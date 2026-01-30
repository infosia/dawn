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

#include "src/tint/lang/wgsl/reader/lower/atomic_vec2u_to_u64.h"

#include <iostream>
#include <utility>

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/memory_view.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/wgsl/ir/builtin_call.h"
#include "src/tint/utils/containers/hashmap.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/rtti/switch.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::wgsl::reader {

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

    /// Map from original type to a new type with decomposed matrices.
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
                auto* store_type = RewriteType(ptr->StoreType());
                if (store_type != ptr->StoreType()) {
                    buffer_variables.Push(std::make_pair(var, store_type));
                }
            }
        }

        // Now process the buffer variables
        for (auto var_and_ty : buffer_variables) {
            // Update the variable type.
            auto* old_var = var_and_ty.first;
            old_var->Result()->SetType(ty.ptr(storage, var_and_ty.second));

            // Transform instructions that accessed the variable.
            old_var->Result()->ForEachUseSorted(
                [&](core::ir::Usage use) { Replace(use.instruction); });
        }
    }

    void Replace(core::ir::Instruction* inst) {
        b.InsertBefore(inst, [&] {
            tint::Switch(
                inst,  //
                [&](core::ir::Access* access) {
                    auto* object_ty = access->Object()->Type()->As<core::type::MemoryView>();
                    if (!object_ty || object_ty->AddressSpace() != core::AddressSpace::kStorage) {
                        return;
                    }

                    auto* replacement = RewriteType(access->Result()->Type());
                    if (replacement != access->Result()->Type()) {
                        access->Result()->SetType(replacement);
                        // Replace every instruction that uses the original access instruction.
                        access->Result()->ForEachUseSorted(
                            [&](core::ir::Usage use) { Replace(use.instruction); });
                    }
                },
                [&](wgsl::ir::BuiltinCall* call) {
                    if (call->Func() == BuiltinFn::kAtomicStoreMax ||
                        call->Func() == BuiltinFn::kAtomicStoreMin) {
                        auto* param0 = call->Args()[0];
                        auto* param1 = call->Args()[1];
                        // Here we go from the wgsl vec2u to the internal 64 bit representation as
                        // WGSL does not have 64 bit types.
                        auto casted_u64 = b.Bitcast(ty.u64(), param1);

                        core::BuiltinFn core_fn = call->Func() == BuiltinFn::kAtomicStoreMax
                                                      ? core::BuiltinFn::kAtomicStoreMax
                                                      : core::BuiltinFn::kAtomicStoreMin;

                        b.CallWithResult(call->DetachResult(), core_fn, param0, casted_u64);
                        call->Destroy();
                    }
                },
                [&](core::ir::Let* let) {
                    // Let instructions just fold away.
                    let->Result()->ForEachUseSorted(
                        [&](core::ir::Usage use) { Replace(use.instruction); });
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
                        return ty.runtime_array(arr->ElemType());
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
                    if (auto* vec = atomic->Type()->As<tint::core::type::Vector>()) {
                        if (vec->Width() == 2 && vec->Type()->Is<core::type::U32>()) {
                            return ty.atomic(ty.u64());
                        }
                    }
                    return atomic;
                },
                [&](Default) { return type; });
        });
    }
};

}  // namespace

Result<SuccessType> AtomicVec2UToU64(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "lower.AtomicVec2UToU64",
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

}  // namespace tint::wgsl::reader
