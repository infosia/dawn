// Copyright 2026 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

enum class Type { u32, i32, vec2u, vec3u, vec4u, vec2i, vec3i, vec4i };

uint32_t VectorSize(Type t) {
    switch (t) {
        case Type::u32:
        case Type::i32:
            return 1;
        case Type::vec2u:
        case Type::vec2i:
            return 2;
        case Type::vec3u:
        case Type::vec3i:
            return 3;
        case Type::vec4u:
        case Type::vec4i:
            return 4;
    }
    return 0;
}

Type ScalarType(Type t) {
    switch (t) {
        case Type::u32:
        case Type::vec2u:
        case Type::vec3u:
        case Type::vec4u:
            return Type::u32;
        case Type::i32:
        case Type::vec2i:
        case Type::vec3i:
        case Type::vec4i:
            return Type::i32;
    }
    return Type::u32;
}

std::string WGSLTypeName(Type t) {
    uint32_t size = VectorSize(t);
    std::string base = ScalarType(t) == Type::u32 ? "u32" : "i32";
    if (size == 1) {
        return base;
    }
    return "vec" + std::to_string(size) + (ScalarType(t) == Type::u32 ? "u" : "i");
}

struct Value {
    std::vector<uint32_t> data;
    Type type;

    Value() : type(Type::u32) {}
    Value(uint32_t v, Type t) : data({v}), type(t) {}
    Value(std::vector<uint32_t> v, Type t) : data(v), type(t) {}

    uint32_t Get(uint32_t i) const { return data[i]; }
};

struct Context {
    std::vector<uint32_t> storage_in;
    std::vector<uint32_t> storage_out;
};

struct WGSLState {
    std::vector<std::string> stmts;
    uint32_t varCount = 0;

    std::string NewVar(const std::string& expr) {
        std::string name = "v" + std::to_string(varCount++);
        stmts.push_back("let " + name + " = " + expr + ";");
        return name;
    }
};

class Node {
  public:
    virtual ~Node() = default;
    virtual Type GetType() const = 0;
    virtual std::string ToWGSL(WGSLState& state) const = 0;
    virtual Value Eval(Context& ctx) const = 0;
};

using NodePtr = std::shared_ptr<Node>;

class Literal : public Node {
  public:
    Literal(std::vector<uint32_t> vals, Type type) : mValues(vals), mType(type) {}
    Type GetType() const override { return mType; }
    std::string ToWGSL(WGSLState& state) const override {
        std::string s;
        uint32_t size = VectorSize(mType);
        if (size > 1) {
            s = WGSLTypeName(mType) + "(";
        }
        for (uint32_t i = 0; i < size; ++i) {
            if (ScalarType(mType) == Type::u32) {
                s += std::to_string(mValues[i]) + "u";
            } else {
                s += std::to_string(static_cast<int32_t>(mValues[i]));
            }
            if (i < size - 1) {
                s += ", ";
            }
        }
        if (size > 1) {
            s += ")";
        }
        return state.NewVar(s);
    }
    Value Eval(Context&) const override { return Value(mValues, mType); }

  private:
    std::vector<uint32_t> mValues;
    Type mType;
};

class StorageVar : public Node {
  public:
    StorageVar(uint32_t index, Type type, bool isInput)
        : mIndex(index), mType(type), mIsInput(isInput) {}
    Type GetType() const override { return mType; }
    std::string ToWGSL(WGSLState& state) const override {
        uint32_t size = VectorSize(mType);
        std::string s;
        if (size > 1) {
            s = WGSLTypeName(mType) + "(";
        }
        for (uint32_t i = 0; i < size; ++i) {
            uint32_t idx = mIndex + i;
            if (mIsInput) {
                std::string inVar = "in_" + std::to_string(idx);
                if (ScalarType(mType) == Type::i32) {
                    s += "bitcast<i32>(" + inVar + ")";
                } else {
                    s += inVar;
                }
            } else {
                std::string outVar = "out_" + std::to_string(idx);
                if (ScalarType(mType) == Type::i32) {
                    s += "bitcast<i32>(" + outVar + ")";
                } else {
                    s += outVar;
                }
            }
            if (i < size - 1) {
                s += ", ";
            }
        }
        if (size > 1) {
            s += ")";
        }
        return state.NewVar(s);
    }
    Value Eval(Context& ctx) const override {
        std::vector<uint32_t> vals;
        for (uint32_t i = 0; i < VectorSize(mType); ++i) {
            vals.push_back(mIsInput ? ctx.storage_in[mIndex + i] : ctx.storage_out[mIndex + i]);
        }
        return Value(vals, mType);
    }

  private:
    uint32_t mIndex;
    Type mType;
    bool mIsInput;
};

class BinaryExpr : public Node {
  public:
    enum class Op { Add, Sub, Mul, Div, Mod, Min, Max };
    BinaryExpr(Op op, NodePtr lhs, NodePtr rhs)
        : mOp(op), mLhs(lhs), mRhs(rhs), mType(lhs->GetType()) {}
    Type GetType() const override { return mType; }
    std::string ToWGSL(WGSLState& state) const override {
        std::string lhs = mLhs->ToWGSL(state);
        std::string rhs = mRhs->ToWGSL(state);
        switch (mOp) {
            case Op::Add:
                return state.NewVar("(" + lhs + " + " + rhs + ")");
            case Op::Sub:
                return state.NewVar("(" + lhs + " - " + rhs + ")");
            case Op::Mul:
                return state.NewVar("(" + lhs + " * " + rhs + ")");
            case Op::Div:
                return state.NewVar("(" + lhs + " / " + rhs + ")");
            case Op::Mod:
                return state.NewVar("(" + lhs + " % " + rhs + ")");
            case Op::Min:
                return state.NewVar("min(" + lhs + ", " + rhs + ")");
            case Op::Max:
                return state.NewVar("max(" + lhs + ", " + rhs + ")");
        }
        return "";
    }
    Value Eval(Context& ctx) const override {
        Value l = mLhs->Eval(ctx);
        Value r = mRhs->Eval(ctx);
        uint32_t size = VectorSize(mType);
        std::vector<uint32_t> res(size);
        for (uint32_t i = 0; i < size; ++i) {
            uint32_t lv = l.Get(i);
            uint32_t rv = r.Get(i);
            if (ScalarType(mType) == Type::u32) {
                switch (mOp) {
                    case Op::Add:
                        res[i] = lv + rv;
                        break;
                    case Op::Sub:
                        res[i] = lv - rv;
                        break;
                    case Op::Mul:
                        res[i] = lv * rv;
                        break;
                    case Op::Div:
                        res[i] = rv == 0 ? lv : lv / rv;
                        break;
                    case Op::Mod:
                        res[i] = rv == 0 ? 0 : lv % rv;
                        break;
                    case Op::Min:
                        res[i] = std::min(lv, rv);
                        break;
                    case Op::Max:
                        res[i] = std::max(lv, rv);
                        break;
                }
            } else {
                int32_t il = static_cast<int32_t>(lv);
                int32_t ir = static_cast<int32_t>(rv);
                switch (mOp) {
                    case Op::Add:
                        res[i] = static_cast<uint32_t>(il + ir);
                        break;
                    case Op::Sub:
                        res[i] = static_cast<uint32_t>(il - ir);
                        break;
                    case Op::Mul:
                        res[i] = static_cast<uint32_t>(il * ir);
                        break;
                    case Op::Div:
                        if (ir == 0) {
                            res[i] = static_cast<uint32_t>(il);
                        } else if (il == std::numeric_limits<int32_t>::min() && ir == -1) {
                            res[i] = static_cast<uint32_t>(il);
                        } else {
                            res[i] = static_cast<uint32_t>(il / ir);
                        }
                        break;
                    case Op::Mod:
                        if (ir == 0) {
                            res[i] = 0;
                        } else if (il == std::numeric_limits<int32_t>::min() && ir == -1) {
                            res[i] = 0;
                        } else {
                            res[i] = static_cast<uint32_t>(il % ir);
                        }
                        break;
                    case Op::Min:
                        res[i] = static_cast<uint32_t>(std::min(il, ir));
                        break;
                    case Op::Max:
                        res[i] = static_cast<uint32_t>(std::max(il, ir));
                        break;
                }
            }
        }
        return Value(res, mType);
    }

  private:
    Op mOp;
    NodePtr mLhs, mRhs;
    Type mType;
};

class UnaryExpr : public Node {
  public:
    enum class Op { Abs, CountLeadingZeros, CountOneBits, CountTrailingZeros };
    UnaryExpr(Op op, NodePtr expr) : mOp(op), mExpr(expr) {}
    Type GetType() const override { return mExpr->GetType(); }
    std::string ToWGSL(WGSLState& state) const override {
        std::string e = mExpr->ToWGSL(state);
        std::string func;
        switch (mOp) {
            case Op::Abs:
                func = "abs";
                break;
            case Op::CountLeadingZeros:
                func = "countLeadingZeros";
                break;
            case Op::CountOneBits:
                func = "countOneBits";
                break;
            case Op::CountTrailingZeros:
                func = "countTrailingZeros";
                break;
        }
        return state.NewVar(func + "(" + e + ")");
    }
    Value Eval(Context& ctx) const override {
        Value v = mExpr->Eval(ctx);
        uint32_t size = VectorSize(GetType());
        std::vector<uint32_t> res(size);
        for (uint32_t i = 0; i < size; ++i) {
            uint32_t val = v.Get(i);
            switch (mOp) {
                case Op::Abs:
                    if (ScalarType(GetType()) == Type::u32) {
                        res[i] = val;
                    } else {
                        int32_t iv = static_cast<int32_t>(val);
                        res[i] = static_cast<uint32_t>(iv < 0 ? -iv : iv);
                    }
                    break;
                case Op::CountLeadingZeros: {
                    uint32_t x = val;
                    if (x == 0) {
                        res[i] = 32;
                    } else {
                        uint32_t n = 0;
                        if (x <= 0x0000FFFF) {
                            n += 16;
                            x <<= 16;
                        }
                        if (x <= 0x00FFFFFF) {
                            n += 8;
                            x <<= 8;
                        }
                        if (x <= 0x0FFFFFFF) {
                            n += 4;
                            x <<= 4;
                        }
                        if (x <= 0x3FFFFFFF) {
                            n += 2;
                            x <<= 2;
                        }
                        if (x <= 0x7FFFFFFF) {
                            n += 1;
                        }
                        res[i] = n;
                    }
                    break;
                }
                case Op::CountOneBits: {
                    uint32_t x = val;
                    x = x - ((x >> 1) & 0x55555555);
                    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
                    res[i] = (((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
                    break;
                }
                case Op::CountTrailingZeros: {
                    uint32_t x = val;
                    if (x == 0) {
                        res[i] = 32;
                    } else {
                        uint32_t n = 0;
                        if (!(x & 0x0000FFFF)) {
                            n += 16;
                            x >>= 16;
                        }
                        if (!(x & 0x000000FF)) {
                            n += 8;
                            x >>= 8;
                        }
                        if (!(x & 0x0000000F)) {
                            n += 4;
                            x >>= 4;
                        }
                        if (!(x & 0x00000003)) {
                            n += 2;
                            x >>= 2;
                        }
                        if (!(x & 0x00000001)) {
                            n += 1;
                        }
                        res[i] = n;
                    }
                    break;
                }
            }
        }
        return Value(res, GetType());
    }

  private:
    Op mOp;
    NodePtr mExpr;
};

class DotExpr : public Node {
  public:
    DotExpr(NodePtr lhs, NodePtr rhs) : mLhs(lhs), mRhs(rhs) {}
    Type GetType() const override { return ScalarType(mLhs->GetType()); }
    std::string ToWGSL(WGSLState& state) const override {
        return state.NewVar("dot(" + mLhs->ToWGSL(state) + ", " + mRhs->ToWGSL(state) + ")");
    }
    Value Eval(Context& ctx) const override {
        Value l = mLhs->Eval(ctx);
        Value r = mRhs->Eval(ctx);
        uint32_t size = VectorSize(mLhs->GetType());
        uint32_t res = 0;
        if (ScalarType(mLhs->GetType()) == Type::u32) {
            for (uint32_t i = 0; i < size; ++i) {
                res += l.Get(i) * r.Get(i);
            }
        } else {
            int32_t ires = 0;
            for (uint32_t i = 0; i < size; ++i) {
                ires += static_cast<int32_t>(l.Get(i)) * static_cast<int32_t>(r.Get(i));
            }
            res = static_cast<uint32_t>(ires);
        }
        return Value(res, GetType());
    }

  private:
    NodePtr mLhs, mRhs;
};

class ClampExpr : public Node {
  public:
    ClampExpr(NodePtr val, NodePtr min, NodePtr max) : mVal(val), mMin(min), mMax(max) {}
    Type GetType() const override { return mVal->GetType(); }
    std::string ToWGSL(WGSLState& state) const override {
        std::string v = mVal->ToWGSL(state);
        std::string lowStr = mMin->ToWGSL(state);
        std::string highStr = mMax->ToWGSL(state);
        // WGSL clamp works component-wise on vectors too.
        // We use min/max to ensure low <= high.
        return state.NewVar("clamp(" + v + ", min(" + lowStr + ", " + highStr + "), max(" + lowStr +
                            ", " + highStr + "))");
    }
    Value Eval(Context& ctx) const override {
        Value v = mVal->Eval(ctx);
        Value low = mMin->Eval(ctx);
        Value high = mMax->Eval(ctx);
        uint32_t size = VectorSize(GetType());
        std::vector<uint32_t> res(size);
        for (uint32_t i = 0; i < size; ++i) {
            uint32_t vv = v.Get(i);
            uint32_t lv = low.Get(i);
            uint32_t hv = high.Get(i);
            if (ScalarType(GetType()) == Type::u32) {
                uint32_t actual_low = std::min(lv, hv);
                uint32_t actual_high = std::max(lv, hv);
                res[i] = std::max(actual_low, std::min(vv, actual_high));
            } else {
                int32_t ilv = static_cast<int32_t>(lv);
                int32_t ihv = static_cast<int32_t>(hv);
                int32_t ivv = static_cast<int32_t>(vv);
                int32_t actual_low = std::min(ilv, ihv);
                int32_t actual_high = std::max(ilv, ihv);
                res[i] = static_cast<uint32_t>(std::max(actual_low, std::min(ivv, actual_high)));
            }
        }
        return Value(res, GetType());
    }

  private:
    NodePtr mVal, mMin, mMax;
};

class Stmt {
  public:
    virtual ~Stmt() = default;
    virtual void ToWGSL(WGSLState& state) const = 0;
    virtual void Eval(Context& ctx) const = 0;
};

using StmtPtr = std::shared_ptr<Stmt>;

class Assignment : public Stmt {
  public:
    Assignment(uint32_t index, NodePtr expr) : mOutputIndex(index), mExpr(expr) {}
    void ToWGSL(WGSLState& state) const override {
        std::string exprVar = mExpr->ToWGSL(state);
        uint32_t size = VectorSize(mExpr->GetType());
        if (size == 1) {
            std::string val = exprVar;
            if (ScalarType(mExpr->GetType()) == Type::i32) {
                val = "bitcast<u32>(" + val + ")";
            }
            state.stmts.push_back("out_" + std::to_string(mOutputIndex) + " = " + val + ";");
        } else {
            for (uint32_t i = 0; i < size; ++i) {
                std::string comp = std::string(1, "xyzw"[i]);
                std::string val = exprVar + "." + comp;
                if (ScalarType(mExpr->GetType()) == Type::i32) {
                    val = "bitcast<u32>(" + val + ")";
                }
                state.stmts.push_back("out_" + std::to_string(mOutputIndex + i) + " = " + val +
                                      ";");
            }
        }
    }
    void Eval(Context& ctx) const override {
        Value v = mExpr->Eval(ctx);
        for (uint32_t i = 0; i < VectorSize(mExpr->GetType()); ++i) {
            ctx.storage_out[mOutputIndex + i] = v.Get(i);
        }
    }

  private:
    uint32_t mOutputIndex;
    NodePtr mExpr;
};

class IfStmt : public Stmt {
  public:
    IfStmt(NodePtr lhs, NodePtr rhs, StmtPtr thenStmt, StmtPtr elseStmt)
        : mCondLhs(lhs), mCondRhs(rhs), mThen(thenStmt), mElse(elseStmt) {}
    void ToWGSL(WGSLState& state) const override {
        std::string lhsVar = mCondLhs->ToWGSL(state);
        std::string rhsVar = mCondRhs->ToWGSL(state);

        std::string lhs = lhsVar;
        std::string rhs = rhsVar;
        if (VectorSize(mCondLhs->GetType()) > 1) {
            lhs += ".x";
        }
        if (VectorSize(mCondRhs->GetType()) > 1) {
            rhs += ".x";
        }

        state.stmts.push_back("if (" + lhs + " > " + rhs + ") {");
        state.stmts.push_back("{BLOCK_START}");
        mThen->ToWGSL(state);
        state.stmts.push_back("{BLOCK_END}");
        state.stmts.push_back("} else {");
        state.stmts.push_back("{BLOCK_START}");
        if (mElse) {
            mElse->ToWGSL(state);
        }
        state.stmts.push_back("{BLOCK_END}");
        state.stmts.push_back("}");
    }
    void Eval(Context& ctx) const override {
        Value l = mCondLhs->Eval(ctx);
        Value r = mCondRhs->Eval(ctx);
        bool cond = false;
        if (ScalarType(mCondLhs->GetType()) == Type::u32) {
            cond = l.Get(0) > r.Get(0);
        } else {
            cond = static_cast<int32_t>(l.Get(0)) > static_cast<int32_t>(r.Get(0));
        }
        if (cond) {
            mThen->Eval(ctx);
        } else if (mElse) {
            mElse->Eval(ctx);
        }
    }

  private:
    NodePtr mCondLhs, mCondRhs;
    StmtPtr mThen, mElse;
};

class LoopStmt : public Stmt {
  public:
    LoopStmt(uint32_t inputIdx, StmtPtr body) : mInputIndex(inputIdx), mBody(body) {}
    void ToWGSL(WGSLState& state) const override {
        // Use a modulo to keep loop count reasonable.
        std::string limit = "(in_" + std::to_string(mInputIndex) + " % 8u)";
        state.stmts.push_back("for (var i = 0u; i < " + limit + "; i = i + 1u) {");
        state.stmts.push_back("{BLOCK_START}");
        mBody->ToWGSL(state);
        state.stmts.push_back("{BLOCK_END}");
        state.stmts.push_back("}");
    }
    void Eval(Context& ctx) const override {
        uint32_t limit = ctx.storage_in[mInputIndex] % 8;
        for (uint32_t i = 0; i < limit; ++i) {
            mBody->Eval(ctx);
        }
    }

  private:
    uint32_t mInputIndex;
    StmtPtr mBody;
};

class Program {
  public:
    void AddStmt(StmtPtr stmt) { mStmts.push_back(stmt); }
    std::string ToWGSL() const {
        std::string s = R"(
@group(0) @binding(0) var<storage, read> input_data : array<u32>;
@group(0) @binding(1) var<storage, read_write> output_data : array<u32>;

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>) {
    let thread_idx = GlobalInvocationID.x;
    let input_offset = thread_idx * 16u;
    let output_offset = thread_idx * 16u;
)";
        for (uint32_t i = 0; i < 16; ++i) {
            s += "    var in_" + std::to_string(i) + " = input_data[input_offset + " +
                 std::to_string(i) + "u];\n";
        }
        for (uint32_t i = 0; i < 16; ++i) {
            s += "    var out_" + std::to_string(i) + " = 0u;\n";
        }

        WGSLState state;
        for (const auto& stmt : mStmts) {
            stmt->ToWGSL(state);
        }

        uint32_t indent = 1;
        for (const auto& line : state.stmts) {
            if (line == "{BLOCK_START}") {
                indent++;
                continue;
            }
            if (line == "{BLOCK_END}") {
                indent--;
                continue;
            }
            for (uint32_t i = 0; i < indent; ++i) {
                s += "    ";
            }
            s += line + "\n";
        }

        for (uint32_t i = 0; i < 16; ++i) {
            s += "    output_data[output_offset + " + std::to_string(i) + "u] = out_" +
                 std::to_string(i) + ";\n";
        }
        s += "}\n";

        return s;
    }
    void Eval(Context& ctx) const {
        for (const auto& stmt : mStmts) {
            stmt->Eval(ctx);
        }
    }

  private:
    std::vector<StmtPtr> mStmts;
};

class Generator {
  public:
    Generator(uint32_t seed, uint32_t inputSize, uint32_t outputSize)
        : mGen(seed), mInputSize(inputSize), mOutputSize(outputSize) {}

    Type GenType() {
        static const std::vector<Type> types = {Type::u32,   Type::i32,   Type::vec2u, Type::vec3u,
                                                Type::vec4u, Type::vec2i, Type::vec3i, Type::vec4i};
        std::uniform_int_distribution<> dist(0, types.size() - 1);
        return types[dist(mGen)];
    }

    NodePtr GenExpr(uint32_t depth, Type type) {
        std::uniform_int_distribution<> dist(0, depth == 0 ? 1 : 5);
        int choice = dist(mGen);
        if (choice == 0) {
            uint32_t size = VectorSize(type);
            std::vector<uint32_t> vals;
            for (uint32_t i = 0; i < size; ++i) {
                vals.push_back(std::uniform_int_distribution<uint32_t>(0, 1000)(mGen));
            }
            return std::make_shared<Literal>(vals, type);
        } else if (choice == 1) {
            uint32_t size = VectorSize(type);
            uint32_t idx = std::uniform_int_distribution<>(0, mInputSize - size)(mGen);
            return std::make_shared<StorageVar>(idx, type, true);
        } else if (choice == 2) {
            BinaryExpr::Op op =
                static_cast<BinaryExpr::Op>(std::uniform_int_distribution<>(0, 6)(mGen));
            return std::make_shared<BinaryExpr>(op, GenExpr(depth - 1, type),
                                                GenExpr(depth - 1, type));
        } else if (choice == 3) {
            return std::make_shared<ClampExpr>(GenExpr(depth - 1, type), GenExpr(depth - 1, type),
                                               GenExpr(depth - 1, type));
        } else if (choice == 4) {
            UnaryExpr::Op op =
                static_cast<UnaryExpr::Op>(std::uniform_int_distribution<>(0, 3)(mGen));
            return std::make_shared<UnaryExpr>(op, GenExpr(depth - 1, type));
        } else {
            // Dot product: only for vectors.
            if (VectorSize(type) == 1) {
                // If we need a scalar, we can dot two vectors of the same scalar type.
                uint32_t vecSize = std::uniform_int_distribution<>(2, 4)(mGen);
                Type vecType;
                if (ScalarType(type) == Type::u32) {
                    vecType =
                        (vecSize == 2) ? Type::vec2u : (vecSize == 3 ? Type::vec3u : Type::vec4u);
                } else {
                    vecType =
                        (vecSize == 2) ? Type::vec2i : (vecSize == 3 ? Type::vec3i : Type::vec4i);
                }
                return std::make_shared<DotExpr>(GenExpr(depth - 1, vecType),
                                                 GenExpr(depth - 1, vecType));
            } else {
                // If we need a vector, just fallback to something else for now.
                return GenExpr(depth, type);
            }
        }
    }

    StmtPtr GenStmt(uint32_t depth) {
        std::uniform_int_distribution<> dist(0, depth == 0 ? 0 : 2);
        int choice = dist(mGen);
        if (choice == 0) {
            Type type = GenType();
            uint32_t size = VectorSize(type);
            uint32_t idx = std::uniform_int_distribution<>(0, mOutputSize - size)(mGen);
            return std::make_shared<Assignment>(idx, GenExpr(2, type));
        } else if (choice == 1) {
            Type type = GenType();
            return std::make_shared<IfStmt>(GenExpr(1, type), GenExpr(1, type), GenStmt(depth - 1),
                                            GenStmt(depth - 1));
        } else {
            uint32_t idx = std::uniform_int_distribution<>(0, mInputSize - 1)(mGen);
            return std::make_shared<LoopStmt>(idx, GenStmt(depth - 1));
        }
    }

    Program GenProgram(uint32_t numStmts) {
        Program p;
        for (uint32_t i = 0; i < numStmts; ++i) {
            p.AddStmt(GenStmt(2));
        }
        return p;
    }

  private:
    std::mt19937 mGen;
    uint32_t mInputSize;
    uint32_t mOutputSize;
};

struct WGSLFuzzerParams : public AdapterTestParam {
    uint32_t iterations;
};

std::ostream& operator<<(std::ostream& os, const WGSLFuzzerParams& p) {
    os << static_cast<const AdapterTestParam&>(p) << ", iterations: " << p.iterations;
    return os;
}

class WGSLFuzzerTests : public DawnTestWithParams<WGSLFuzzerParams> {
  protected:
    wgpu::Buffer CreateBuffer(const std::vector<uint32_t>& data,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        uint64_t bufferSize = static_cast<uint64_t>(data.size() * sizeof(uint32_t));
        return utils::CreateBufferFromData(device, data.data(), bufferSize, usage);
    }

    wgpu::Buffer CreateBuffer(const uint32_t count,
                              const uint32_t default_val = 0,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        std::vector<uint32_t> data(count, default_val);
        return CreateBuffer(data, usage);
    }

    wgpu::ComputePipeline CreateComputePipeline(const std::string& shader) {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = utils::CreateShaderModule(device, shader.c_str());
        csDesc.compute.entryPoint = "main";
        return device.CreateComputePipeline(&csDesc);
    }
};

TEST_P(WGSLFuzzerTests, RandomFuzz) {
    // Skip slow tests in Debug mode on CI/farm.
#if _DEBUG
    DAWN_SUPPRESS_TEST_IF(IsTestLauncherBotMode());
#endif

    const uint32_t kInputSizePerThread = 16;
    const uint32_t kOutputSizePerThread = 16;
    const uint32_t kWorkgroupSize = 256;
    const uint32_t kNumWorkgroups = 8;
    const uint32_t kNumThreads = kWorkgroupSize * kNumWorkgroups;
    const uint32_t kNumIterations = GetParam().iterations;

    std::random_device rd;
    uint32_t baseSeed = rd();
    SCOPED_TRACE("Base Seed: " + std::to_string(baseSeed));

    for (uint32_t i = 0; i < kNumIterations; ++i) {
        uint32_t seed = baseSeed + i;
        SCOPED_TRACE("Iteration: " + std::to_string(i) + ", Seed: " + std::to_string(seed));

        Generator gen(seed, kInputSizePerThread, kOutputSizePerThread);
        Program prog = gen.GenProgram(20);
        std::string wgsl = prog.ToWGSL();
        SCOPED_TRACE("WGSL:\n" + wgsl);

        std::vector<uint32_t> total_input(kNumThreads * kInputSizePerThread);
        std::vector<uint32_t> total_expected_output(kNumThreads * kOutputSizePerThread, 0);

        for (uint32_t t = 0; t < kNumThreads; ++t) {
            Context ctx;
            ctx.storage_in.resize(kInputSizePerThread);
            for (uint32_t j = 0; j < kInputSizePerThread; ++j) {
                ctx.storage_in[j] = (i + 1) * (t + 1) * (j + 1) * 1103515245 + 12345;
                total_input[t * kInputSizePerThread + j] = ctx.storage_in[j];
            }
            ctx.storage_out.assign(kOutputSizePerThread, 0);

            prog.Eval(ctx);

            for (uint32_t j = 0; j < kOutputSizePerThread; ++j) {
                total_expected_output[t * kOutputSizePerThread + j] = ctx.storage_out[j];
            }
        }

        wgpu::ComputePipeline pipeline = CreateComputePipeline(wgsl);
        wgpu::Buffer input = CreateBuffer(total_input);
        wgpu::Buffer output = CreateBuffer(total_expected_output.size(), 0);
        wgpu::BindGroup bindGroup =
            utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, input}, {1, output}});

        wgpu::BufferDescriptor readbackDesc;
        readbackDesc.size = total_expected_output.size() * sizeof(uint32_t);
        readbackDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        wgpu::Buffer readbackBuffer = device.CreateBuffer(&readbackDesc);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.DispatchWorkgroups(kNumWorkgroups);
        pass.End();
        encoder.CopyBufferToBuffer(output, 0, readbackBuffer, 0, readbackDesc.size);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        MapAsyncAndWait(readbackBuffer, wgpu::MapMode::Read, 0, readbackDesc.size);
        const uint32_t* result_data =
            static_cast<const uint32_t*>(readbackBuffer.GetConstMappedRange());

        for (uint32_t t = 0; t < kNumThreads; ++t) {
            bool threadMatch = true;
            for (uint32_t j = 0; j < kOutputSizePerThread; ++j) {
                if (result_data[t * kOutputSizePerThread + j] !=
                    total_expected_output[t * kOutputSizePerThread + j]) {
                    threadMatch = false;
                    break;
                }
            }

            if (!threadMatch) {
                std::cout << "Mismatch found!" << std::endl;
                std::cout << "Iteration: " << i << ", Seed: " << seed << ", Thread: " << t
                          << std::endl;
                std::cout << "Input data for thread " << t << ":" << std::endl;
                for (uint32_t j = 0; j < kInputSizePerThread; ++j) {
                    std::cout << "  in_" << j << ": " << total_input[t * kInputSizePerThread + j]
                              << " (0x" << std::hex << total_input[t * kInputSizePerThread + j]
                              << std::dec << ")" << std::endl;
                }
                std::cout << "Expected vs Actual output for thread " << t << ":" << std::endl;
                for (uint32_t j = 0; j < kOutputSizePerThread; ++j) {
                    uint32_t expected = total_expected_output[t * kOutputSizePerThread + j];
                    uint32_t actual = result_data[t * kOutputSizePerThread + j];
                    std::cout << "  out_" << j << ": Expected " << expected << " (0x" << std::hex
                              << expected << std::dec << "), Actual " << actual << " (0x"
                              << std::hex << actual << std::dec << ")";
                    if (expected != actual) {
                        std::cout << " <--- MISMATCH";
                    }
                    std::cout << std::endl;
                }
                std::cout << "WGSL:" << std::endl << wgsl << std::endl;
                readbackBuffer.Unmap();
                FAIL() << "Data mismatch in WGSL fuzzer";
            }
        }
        readbackBuffer.Unmap();
    }
}

DAWN_INSTANTIATE_TEST_P(WGSLFuzzerTests,
                        {D3D12Backend(), MetalBackend(), VulkanBackend(), OpenGLESBackend()},
                        {1, 2, 3, 4, 5, 6, 7, 8});

}  // anonymous namespace
}  // namespace dawn
