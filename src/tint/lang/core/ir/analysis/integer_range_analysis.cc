// Copyright 2024 The Dawn & Tint Authors
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

#include "src/tint/lang/core/ir/analysis/integer_range_analysis.h"

#include <algorithm>
#include <limits>

#include "src/tint/lang/core/constant/scalar.h"
#include "src/tint/lang/core/ir/analysis/loop_analysis.h"
#include "src/tint/lang/core/ir/binary.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/lang/core/ir/store.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/utils/rtti/switch.h"

namespace tint::core::ir::analysis {

/// Returns an instruction of the given kind if @p val is the result of such an instruction.
/// Otherwise returns nullptr.
template <typename InstClass>
InstClass* As(Value* val) {
    if (auto* instres = val->As<InstructionResult>()) {
        return instres->Instruction()->As<InstClass>();
    }
    return nullptr;
}

IntegerRangeInfo::IntegerRangeInfo(int64_t min_bound, int64_t max_bound) {
    TINT_ASSERT(min_bound <= max_bound);
    range = SignedIntegerRange{min_bound, max_bound};
}

IntegerRangeInfo::IntegerRangeInfo(uint64_t min_bound, uint64_t max_bound) {
    TINT_ASSERT(min_bound <= max_bound);
    range = UnsignedIntegerRange{min_bound, max_bound};
}

struct IntegerRangeAnalysisImpl {
    explicit IntegerRangeAnalysisImpl(Function* func) : function_(func) {
        // Analyze all of the loops in the function.
        Traverse(func->Block(), [&](Loop* l) { AnalyzeLoop(l); });
    }

    const IntegerRangeInfo* GetInfo(const Var* var) {
        auto result = integer_var_range_info_map_.Get(var);
        if (result) {
            return result.value;
        } else {
            return nullptr;
        }
    }

    const IntegerRangeInfo* GetInfo(const FunctionParam* param, uint32_t index) {
        if (!param->Type()->IsIntegerScalarOrVector()) {
            return nullptr;
        }

        const auto& info = integer_function_param_range_info_map_.GetOrAdd(
            param, [&]() -> Vector<IntegerRangeInfo, 3> {
                if (param->Builtin() == core::BuiltinValue::kLocalInvocationIndex) {
                    // We shouldn't be trying to use range analysis on a module that has
                    // non-constant workgroup sizes, since we will always have replaced pipeline
                    // overrides with constant values early in the pipeline.
                    TINT_ASSERT(function_->WorkgroupSizeAsConst().has_value());
                    std::array<uint32_t, 3> workgroup_size =
                        function_->WorkgroupSizeAsConst().value();
                    uint64_t max_bound =
                        workgroup_size[0] * workgroup_size[1] * workgroup_size[2] - 1u;
                    constexpr uint64_t kMinBound = 0;

                    return {IntegerRangeInfo(kMinBound, max_bound)};
                }

                if (param->Builtin() == core::BuiltinValue::kLocalInvocationId) {
                    TINT_ASSERT(function_->WorkgroupSizeAsConst().has_value());
                    std::array<uint32_t, 3> workgroup_size =
                        function_->WorkgroupSizeAsConst().value();

                    constexpr uint64_t kMinBound = 0;
                    Vector<IntegerRangeInfo, 3> integerRanges;
                    for (uint32_t size_x_y_z : workgroup_size) {
                        integerRanges.Push({kMinBound, size_x_y_z - 1u});
                    }
                    return integerRanges;
                }

                if (param->Type()->IsUnsignedIntegerScalar()) {
                    return {IntegerRangeInfo(0, std::numeric_limits<uint64_t>::max())};
                } else {
                    TINT_ASSERT(param->Type()->IsSignedIntegerScalar());
                    return {IntegerRangeInfo(std::numeric_limits<int64_t>::min(),
                                             std::numeric_limits<int64_t>::max())};
                }
            });

        TINT_ASSERT(info.Length() > index);
        return &info[index];
    }

    /// Analyze a loop.
    void AnalyzeLoop(Loop* loop) {
        if (auto* init_block = loop->Initializer()) {
            // Look for variables that could be used as iteration indices.
            for (auto* inst : *init_block) {
                auto* var = inst->As<Var>();
                if (!var) {
                    continue;
                }

                const auto* pty = var->Result(0)->Type()->As<core::type::Pointer>();
                if (!pty->StoreType()->IsIntegerScalar()) {
                    break;
                }

                std::variant<std::monostate, int64_t, uint64_t> bound0 =
                    GetBoundFromConstInitializer(var);
                if (std::holds_alternative<std::monostate>(bound0)) {
                    break;
                }

                std::variant<std::monostate, int64_t, uint64_t> bound1 =
                    GetBoundFromLoopExit(loop, var);
                if (std::holds_alternative<std::monostate>(bound1)) {
                    break;
                }

                if (std::holds_alternative<uint64_t>(bound0)) {
                    TINT_ASSERT(std::holds_alternative<uint64_t>(bound1));
                    uint64_t uintBound0 = std::get<uint64_t>(bound0);
                    uint64_t uintBound1 = std::get<uint64_t>(bound1);
                    integer_var_range_info_map_.Add(
                        var, IntegerRangeInfo(std::min(uintBound0, uintBound1),
                                              std::max(uintBound0, uintBound1)));
                } else {
                    TINT_ASSERT(std::holds_alternative<int64_t>(bound1));
                    int64_t intBound0 = std::get<int64_t>(bound0);
                    int64_t intBound1 = std::get<int64_t>(bound1);
                    integer_var_range_info_map_.Add(
                        var, IntegerRangeInfo(std::min(intBound0, intBound1),
                                              std::max(intBound0, intBound1)));
                }
                return;
            }
        }
    }

    std::variant<std::monostate, int64_t, uint64_t> GetBoundFromConstInitializer(Var* var) {
        std::variant<std::monostate, int64_t, uint64_t> output;
        const auto* initializer = var->Initializer();
        if (!initializer) {
            return output;
        }
        const auto* constInitializer = initializer->As<Constant>();
        if (!constInitializer) {
            return output;
        }
        if (constInitializer->Value()->Type()->IsSignedIntegerScalar()) {
            const auto* i32Initializer =
                constInitializer->Value()->As<core::constant::Scalar<i32>>();
            TINT_ASSERT(i32Initializer);
            return static_cast<int64_t>(i32Initializer->ValueAs<int32_t>());
        } else {
            const auto* u32Initializer =
                constInitializer->Value()->As<core::constant::Scalar<u32>>();
            TINT_ASSERT(u32Initializer);
            return static_cast<uint64_t>(u32Initializer->ValueAs<uint32_t>());
        }
    }

    std::variant<std::monostate, int64_t, uint64_t> GetBoundFromLoopExit(Loop* loop, Var* index) {
        std::variant<std::monostate, int64_t, uint64_t> output;

        // Look for a store to the index in the continuing block.
        // Make sure there is only one, and make sure that the only other uses are loads.
        Store* single_store_in_continue_block = nullptr;
        const auto& uses = index->Result(0)->UsagesUnsorted();
        for (auto& use : uses) {
            if (auto* store = use->instruction->As<Store>()) {
                if (store->Block() != loop->Continuing()) {
                    // Not in the continuing block, so we cannot easily prove that it will be
                    // executed.
                    return output;
                }
                if (single_store_in_continue_block) {
                    // Found more than one store, so we cannot easily analyze this candidate.
                    return output;
                }
                single_store_in_continue_block = store;
            } else if (!use->instruction->Is<Load>()) {
                // Not a store or a load, so reject this candidate.
                return output;
            }
        }
        if (!single_store_in_continue_block) {
            return output;
        }

        // Check that the store we found is either incrementing or decrementing the index by `1`.
        if (!IsIncrementOrDecrementOfVar(*index, single_store_in_continue_block->From())) {
            return output;
        }

        // Now check if the loop body begins with a break-if construct that is guaranteed to exit
        // the loop if the index reaches the loop bound and return the loop bound.
        for (auto* inst : *loop->Body()) {
            auto* if_inst = inst->As<If>();
            if (!if_inst) {
                continue;
            }

            // Only look at the first 'if' we find.
            output = GetBoundFromBreakIfOnIndex(loop, if_inst, index);
            break;
        }
        return output;
    }

    /// @returns the loop bound if @p is a break-if construct that exits the loop based on @p index.
    /// Otherwise returns a variant that holds std::monostate.
    std::variant<std::monostate, int64_t, uint64_t> GetBoundFromBreakIfOnIndex(const Loop* loop,
                                                                               If* i,
                                                                               Var* index) {
        std::variant<std::monostate, int64_t, uint64_t> output;

        // Returns `true` if the given value is a load of the index variable.
        auto is_index = [&index](Value* v) {
            if (auto* load = As<Load>(v)) {
                return load->From() == index->Result(0);
            }
            return false;
        };
        // Returns `true` if the given value a constant value declared before the loop body.
        auto is_const_before_body = [](Value* v) { return v->Is<Constant>(); };
        // Make sure that one of the branches will exit from the
        // loop and do nothing else.
        auto is_simple_loop_exit = [](Block* b) {
            return b && !b->IsEmpty() && (b->Front()->Is<ExitLoop>());
        };

        // Check if the condition matches (%idx < %bound) or (%idx > %bound).
        // The value %bound can be any immutable value that was declared before the body.
        auto* binary = As<Binary>(i->Condition());
        if (!binary) {
            return output;
        }
        if (binary->Op() != BinaryOp::kLessThan && binary->Op() != BinaryOp::kGreaterThan &&
            binary->Op() != BinaryOp::kLessThanEqual &&
            binary->Op() != BinaryOp::kGreaterThanEqual) {
            return output;
        }

        Constant* bound1ConstantValue = nullptr;
        if (is_simple_loop_exit(i->True()) || is_simple_loop_exit(i->False())) {
            if (is_index(binary->LHS()) && is_const_before_body(binary->RHS())) {
                bound1ConstantValue = binary->RHS()->As<Constant>();
            } else if (is_index(binary->RHS()) && is_const_before_body(binary->LHS())) {
                bound1ConstantValue = binary->LHS()->As<Constant>();
            }
        }
        if (!bound1ConstantValue) {
            return output;
        }

        if (bound1ConstantValue->Value()->Type()->IsSignedIntegerScalar()) {
            const auto* i32Value = bound1ConstantValue->Value()->As<core::constant::Scalar<i32>>();
            TINT_ASSERT(i32Value);
            int64_t bound1 = static_cast<int64_t>(i32Value->ValueAs<int32_t>());
            if (binary->Op() == BinaryOp::kLessThan || binary->Op() == BinaryOp::kGreaterThan) {
                bound1--;
            }
            return bound1;
        } else {
            const auto* u32Value = bound1ConstantValue->Value()->As<core::constant::Scalar<u32>>();
            TINT_ASSERT(u32Value);
            uint64_t bound1 = static_cast<uint64_t>(u32Value->ValueAs<uint32_t>());
            if (binary->Op() == BinaryOp::kLessThan || binary->Op() == BinaryOp::kGreaterThan) {
                bound1--;
            }
            return bound1;
        }
    }

  private:
    Function* function_;
    Hashmap<const FunctionParam*, Vector<IntegerRangeInfo, 3>, 4>
        integer_function_param_range_info_map_;

    Hashmap<const Var*, IntegerRangeInfo, 8> integer_var_range_info_map_;
};

IntegerRangeAnalysis::IntegerRangeAnalysis(Function* func)
    : impl_(new IntegerRangeAnalysisImpl(func)) {}
IntegerRangeAnalysis::~IntegerRangeAnalysis() = default;

const IntegerRangeInfo* IntegerRangeAnalysis::GetInfo(const FunctionParam* param, uint32_t index) {
    return impl_->GetInfo(param, index);
}

const IntegerRangeInfo* IntegerRangeAnalysis::GetInfo(const Var* var) {
    return impl_->GetInfo(var);
}
}  // namespace tint::core::ir::analysis
