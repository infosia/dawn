// Copyright 2025 The Dawn & Tint Authors
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

#include "src/tint/lang/spirv/reader/lower/atomics.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/spirv/ir/builtin_call.h"

namespace tint::spirv::reader::lower {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The `ir::Value`s to be converted to atomics
    std::vector<core::ir::Value*> values_to_convert_{};

    /// The `ir::Value`s which have been converted
    std::unordered_set<core::ir::Value*> converted_{};

    struct ForkedStruct {
        const core::type::Struct* src_strct = nullptr;
        core::type::Struct* dst_strct = nullptr;
        std::unordered_set<size_t> atomic_members;
    };

    /// Map of original structure to forked structure information
    std::unordered_map<const core::type::Struct*, ForkedStruct> forked_structs_{};

    /// Process the module.
    void Process() {
        Vector<spirv::ir::BuiltinCall*, 4> builtin_worklist;
        for (auto* inst : ir.Instructions()) {
            if (auto* builtin = inst->As<spirv::ir::BuiltinCall>()) {
                builtin_worklist.Push(builtin);
            }
        }

        for (auto* builtin : builtin_worklist) {
            switch (builtin->Func()) {
                case spirv::BuiltinFn::kAtomicLoad:
                    break;
                case spirv::BuiltinFn::kAtomicStore:
                    AtomicStore(builtin);
                    break;
                case spirv::BuiltinFn::kAtomicExchange:
                case spirv::BuiltinFn::kAtomicCompareExchange:
                case spirv::BuiltinFn::kAtomicIAdd:
                case spirv::BuiltinFn::kAtomicISub:
                case spirv::BuiltinFn::kAtomicSMax:
                case spirv::BuiltinFn::kAtomicSMin:
                case spirv::BuiltinFn::kAtomicUMax:
                case spirv::BuiltinFn::kAtomicUMin:
                case spirv::BuiltinFn::kAtomicAnd:
                case spirv::BuiltinFn::kAtomicOr:
                case spirv::BuiltinFn::kAtomicXor:
                case spirv::BuiltinFn::kAtomicIIncrement:
                case spirv::BuiltinFn::kAtomicIDecrement:
                    break;
                default:
                    TINT_UNREACHABLE() << "unknown spirv builtin: " << builtin->Func();
            }
        }

        while (!values_to_convert_.empty()) {
            auto* val = values_to_convert_.back();
            values_to_convert_.pop_back();

            if (converted_.count(val) > 0) {
                continue;
            }

            converted_.insert(val);
            ConvertAtomicValue(val);
        }

        ProcessForkedStructs();
    }

    void AtomicStore(spirv::ir::BuiltinCall* call) {
        auto args = call->Args();

        b.InsertBefore(call, [&] {
            auto* var = args[0];
            values_to_convert_.push_back(var);

            auto* val = args[3];
            b.CallWithResult(call->DetachResult(), core::BuiltinFn::kAtomicStore, var, val);
        });
        call->Destroy();
    }

    void ProcessForkedStructs() {
        for (auto iter : forked_structs_) {
            auto& forked = iter.second;
        }
    }

    void ConvertAtomicValue(core::ir::Value* val) {
        auto* res = val->As<core::ir::InstructionResult>();
        TINT_ASSERT(res);

        auto* orig_ty = res->Type();
        auto* atomic_ty = AtomicTypeFor(orig_ty);
        res->SetType(atomic_ty);

        tint::Switch(            //
            res->Instruction(),  //
            [&](core::ir::Access* a) {
                CheckForStructForking(a);
                values_to_convert_.push_back(a->Object());
            },                                                                    //
            [&](core::ir::Let* l) { values_to_convert_.push_back(l->Value()); },  //
            [&](core::ir::Var*) {},                                               //
            TINT_ICE_ON_NO_MATCH);
    }

    void CheckForStructForking(core::ir::Access* access) {
        auto* cur_ty = access->Object()->Type();
        for (auto* idx : access->Indices()) {
            tint::Switch(
                cur_ty,  //
                [&](const core::type::Struct* str) {
                    auto& forked = Fork(str);

                    auto* const_val = idx->As<core::ir::Constant>();
                    TINT_ASSERT(const_val);

                    auto const_idx = const_val->Value()->ValueAs<uint32_t>();
                    forked.atomic_members.insert(const_idx);

                    cur_ty = str->Members()[const_idx]->Type();
                },                                                                   //
                [&](const core::type::Array* ary) { cur_ty = ary->ElemType(); },     //
                [&](const core::type::Pointer* ptr) { cur_ty = ptr->StoreType(); },  //
                TINT_ICE_ON_NO_MATCH);
        }
    }

    const core::type::Type* AtomicTypeFor(const core::type::Type* orig_ty) {
        return tint::Switch(
            orig_ty,  //
            [&](const core::type::I32*) { return ty.atomic(orig_ty); },
            [&](const core::type::U32*) { return ty.atomic(orig_ty); },
            [&](const core::type::Struct* str) { return Fork(str).dst_strct; },
            [&](const core::type::Array* arr) {
                if (arr->Count()->Is<core::type::RuntimeArrayCount>()) {
                    return ty.runtime_array(AtomicTypeFor(arr->ElemType()));
                }
                auto count = arr->ConstantCount();
                TINT_ASSERT(count);

                return ty.array(AtomicTypeFor(arr->ElemType()), u32(count.value()));
            },
            [&](const core::type::Pointer* ptr) {
                return ty.ptr(ptr->AddressSpace(), AtomicTypeFor(ptr->StoreType()), ptr->Access());
            },
            TINT_ICE_ON_NO_MATCH);
    }

    ForkedStruct& Fork(const core::type::Struct* str) {
        auto& forked = forked_structs_[str];
        if (!forked.dst_strct) {
            forked.src_strct = str;
            forked.dst_strct = const_cast<core::type::Struct*>(
                ty.Struct(ir.symbols.New(str->Name().Name() + "_atomic"), {}));
        }
        return forked;
    }
};

}  // namespace

Result<SuccessType> Atomics(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "spirv.Atomics",
                                          core::ir::Capabilities{
                                              core::ir::Capability::kAllowOverrides,
                                          });
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::reader::lower
