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

#include "src/tint/lang/wgsl/writer/raise/atomic_vec2u_from_u64.h"

#include <utility>

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/bitcast.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/instruction_result.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/memory_view.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/u64.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/wgsl/enums.h"
#include "src/tint/lang/wgsl/ir/builtin_call.h"
#include "src/tint/utils/containers/hashmap.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/rtti/switch.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::wgsl::writer {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    tint::core::ir::Module& ir;

    /// The IR builder.
    tint::core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The symbol table.
    SymbolTable& sym{ir.symbols};

    /// Map from original type to a new type with atomic<vec2<u32>> rewritten to atomic<u64>.
    Hashmap<const core::type::Type*, const core::type::Type*, 4> rewritten_types{};

    /// Process the module.
    void Process() {
        if (ir.root_block->IsEmpty()) {
            return;
        }
        // Find storage buffers that contain atomic<vec2<u32>> that need to be converted to
        // atomic<u64>.
        Vector<std::pair<core::ir::Var*, const core::type::Type*>, 8> buffer_variables;
        for (auto inst : *ir.root_block) {
            if (auto* var = inst->As<tint::core::ir::Var>()) {
                auto* ptr = var->Result()->Type()->As<core::type::Pointer>();
                // atomic vec2u only supports storage
                if (!ptr || ptr->AddressSpace() != core::AddressSpace::kStorage) {
                    continue;
                }
                // Store type could be composite so we attempt to rewrite first to know if we need
                // to change this type.
                auto* store_type = RewriteType(ptr->StoreType());
                if (store_type != ptr->StoreType()) {
                    auto* old_var = var;
                    old_var->Result()->SetType(ty.ptr(storage, store_type));
                    old_var->Result()->ForEachUseSorted(
                        [&](core::ir::Usage use) { Replace(use.instruction); });
                }
            }
        }

        for (auto func : ir.functions) {
            for (auto fn_params : func->Params()) {
                if (auto* val = fn_params->As<tint::core::ir::Value>()) {
                    auto* ptr = val->Type()->As<core::type::Pointer>();
                    // atomic vec2u only supports storage
                    if (!ptr || ptr->AddressSpace() != core::AddressSpace::kStorage) {
                        continue;
                    }
                    // Store type could be composite so we attempt to rewrite first to know if we
                    // need to change this type.
                    auto* store_type = RewriteType(ptr->StoreType());
                    if (store_type != ptr->StoreType()) {
                        auto* old_val = val;
                        old_val->SetType(ty.ptr(storage, store_type));
                        old_val->ForEachUseSorted(
                            [&](core::ir::Usage use) { Replace(use.instruction); });
                    }
                }
            }
        }
    }

    void Replace(core::ir::Instruction* inst) {
        b.InsertBefore(inst, [&] {
            tint::Switch(
                inst,  //
                [&](core::ir::Access* access) {
                    auto* replacement = RewriteType(access->Result()->Type());
                    if (replacement != access->Result()->Type()) {
                        access->Result()->SetType(replacement);
                        // Replace every instruction that uses the original access instruction.
                        access->Result()->ForEachUseSorted(
                            [&](core::ir::Usage use) { Replace(use.instruction); });
                    }
                },
                [&](core::ir::CoreBuiltinCall* call) {
                    if (call->Func() == core::BuiltinFn::kAtomicStoreMax ||
                        call->Func() == core::BuiltinFn::kAtomicStoreMin) {
                        auto* ptr_atomic = call->Args()[0];
                        auto* param_u64 = call->Args()[1];

                        // We are expecting this to have already been type converted to u64.
                        auto as_ptr = ptr_atomic->Type()->As<core::type::Pointer>();
                        TINT_ASSERT(as_ptr);
                        TINT_ASSERT(as_ptr->AddressSpace() == core::AddressSpace::kStorage);
                        auto as_store = as_ptr->StoreType()->As<core::type::Atomic>();
                        TINT_ASSERT(as_store->Type()->Is<core::type::Vector>());

                        auto* as_inst_result = param_u64->As<core::ir::InstructionResult>();
                        TINT_ASSERT(as_inst_result);
                        auto* as_inst = as_inst_result->Instruction();
                        TINT_ASSERT(as_inst);
                        auto* as_bitcast = as_inst->As<core::ir::Bitcast>();
                        TINT_ASSERT(as_bitcast);
                        TINT_ASSERT(as_bitcast->Result()->NumUsages() == 1);
                        param_u64 = as_bitcast->Val();
                        as_bitcast->Destroy();

                        BuiltinFn wgsl_fn = call->Func() == core::BuiltinFn::kAtomicStoreMax
                                                ? BuiltinFn::kAtomicStoreMax
                                                : BuiltinFn::kAtomicStoreMin;

                        b.CallWithResult<wgsl::ir::BuiltinCall>(call->DetachResult(), wgsl_fn,
                                                                ptr_atomic, param_u64);
                        call->Destroy();
                    }
                },
                [&](core::ir::Let* let) {
                    auto* replacement = RewriteType(let->Result()->Type());
                    if (replacement != let->Result()->Type()) {
                        let->Result()->SetType(replacement);
                        // Replace every instruction that uses the original access instruction.
                        let->Result()->ForEachUseSorted(
                            [&](core::ir::Usage use) { Replace(use.instruction); });
                    }
                });
        });
    }

    const core::type::Type* RewriteType(const core::type::Type* type) {
        return rewritten_types.GetOrAdd(type, [&] {
            return tint::Switch(
                type,
                [&](const core::type::Array* arr) {
                    // Create a new array with element type potentially rewritten.
                    if (arr->ConstantCount().has_value()) {
                        return ty.array(RewriteType(arr->ElemType()), arr->ConstantCount().value());
                    } else {
                        return ty.runtime_array(RewriteType(arr->ElemType()));
                    }
                },
                [&](const core::type::Struct* str) -> const core::type::Type* {
                    bool needs_rewrite = false;
                    Vector<const core::type::StructMember*, 4> new_members;
                    for (auto* member : str->Members()) {
                        // Recursively rewrite members.
                        auto* new_member_ty = RewriteType(member->Type());
                        new_members.Push(ty.Get<core::type::StructMember>(
                            member->Name(), new_member_ty, member->Index(), member->Offset(),
                            member->Align(), member->Size(), core::IOAttributes{}));
                        if (new_member_ty != member->Type()) {
                            needs_rewrite = true;
                        }
                    }

                    // If no members needed to be rewritten, just return the original struct.
                    if (!needs_rewrite) {
                        return str;
                    }

                    // Create a new struct with the rewritten members.
                    auto* new_str = ty.Get<core::type::Struct>(sym.New(str->Name().Name() + "_a64"),
                                                               std::move(new_members), str->Size());
                    for (auto flag : str->StructFlags()) {
                        new_str->SetStructFlag(flag);
                    }
                    return new_str;
                },
                [&](const core::type::Pointer* ptr) {
                    return ty.ptr(ptr->AddressSpace(), RewriteType(ptr->StoreType()));
                },
                [&](const core::type::Atomic* atomic) -> const core::type::Type* {
                    // The real target of our replacement. The atomic u64 type.
                    if (atomic->Type()->Is<core::type::U64>()) {
                        return ty.atomic(ty.vec2u());
                    }
                    return atomic;
                },
                [&](Default) { return type; });
        });
    }
};

}  // namespace

Result<SuccessType> AtomicVec2UFromU64(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "lower.AtomicVec2UFromU64",
                                          core::ir::Capabilities{
                                              core::ir::Capability::kAllowMultipleEntryPoints,
                                              core::ir::Capability::kAllowOverrides,
                                              core::ir::Capability::kAllow8BitIntegers,
                                          });
    if (result != Success) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::wgsl::writer
