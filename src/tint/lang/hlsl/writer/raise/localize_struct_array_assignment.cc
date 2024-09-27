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

#include "src/tint/lang/hlsl/writer/raise/fxc_polyfill.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/result/result.h"

namespace tint::hlsl::writer::raise {
namespace {

using namespace tint::core::fluent_types;  // NOLINT

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    void InlineAccess(core::ir::Access* access,
                      core::ir::Value*& object,
                      Vector<core::ir::Value*, 4>& indices) {
        bool is_access = false;
        if (auto* inst_result = access->Object()->As<core::ir::InstructionResult>()) {
            if (auto* obj_access = inst_result->Instruction()->As<core::ir::Access>()) {
                InlineAccess(obj_access, object, indices);
                is_access = true;
            }
        }
        if (!is_access) {
            object = access->Object();
        }

        for (auto& i : access->Indices()) {
            indices.Push(i);
        }
    }

    void ProcessStore(core::ir::Store* store) {
        auto* to_ptr = store->To()->Type()->As<core::type::Pointer>();
        if (to_ptr->AddressSpace() != core::AddressSpace::kFunction &&
            to_ptr->AddressSpace() != core::AddressSpace::kPrivate) {
            return;
        }
        // Must be storing via an access
        auto* to = store->To()->As<core::ir::InstructionResult>();
        if (!to) {
            return;
        }
        auto* to_access = to->Instruction()->As<core::ir::Access>();
        if (!to_access) {
            return;
        }

        core::ir::Value* object = nullptr;
        Vector<core::ir::Value*, 4> indices;
        InlineAccess(to_access, object, indices);

        // Look for struct.array[dyn_index] triple
        if (indices.Length() < 2) {
            return;
        }

        auto get_type_at = [](const core::type::Type* ty, const core::ir::Value* idx) {
            if (auto* constant = idx->As<core::ir::Constant>()) {
                auto i = constant->Value()->ValueAs<u32>();
                return ty->Element(i);
            }
            return ty->Elements().type;
        };

        auto* ty1 = object->Type()->As<core::type::Pointer>()->UnwrapPtr();
        if (!ty1->Is<core::type::Struct>()) {
            // FXC only fails when the top-level object is a struct. An array of struct of array
            // indexed dynamically is fine.
            return;
        }
        for (size_t i = 0; i < indices.Length() - 1; ++i) {
            auto* ty2 =
                get_type_at(ty1, indices[i]);  // Don't need this lambda, must be constant index
            if (ty1->Is<core::type::Struct>()) {
                if (ty2->Is<core::type::Array>()) {
                    if (!indices[i + 1]->Is<core::ir::Constant>()) {
                        // Found one
                        b.InsertBefore(store, [&] {
                            // Create an access to the array in the struct to copy from
                            auto* array_access =
                                b.Access(ty.ptr(to_ptr->AddressSpace(), ty2), object,
                                         ToVector<4>(indices.Slice().Truncate(i + 1)));

                            // Copy the struct array to a local variable
                            auto* local_array = b.Var("tint_array_copy", b.Load(array_access));
                            // Store the previous store's From to the local array at the same index
                            auto* local_array_value_access =
                                b.Access(ty.ptr<function>(to_ptr->StoreType()), local_array,
                                         ToVector<4>(indices.Slice().Offset(i + 1)));
                            b.Store(local_array_value_access, store->From());
                            // Finally, copy back the data from the local array to the struct array
                            b.Store(array_access, b.Load(local_array));
                            // We can now remove the old store
                            store->Destroy();
                        });
                        return;
                    }
                }
            }
            ty1 = ty2;
        }
    }

    /// Process the module.
    void Process() {
        Vector<core::ir::Store*, 16> stores;
        for (auto* inst : ir.Instructions()) {
            if (auto* store = inst->As<core::ir::Store>()) {
                stores.Push(store);
            }
        }
        for (auto* store : stores) {
            ProcessStore(store);
        }
        // for (auto* inst : ir.Instructions()) {
        // if (auto* store = inst->As<core::ir::Store>()) {
        //     ProcessStore(store);
        // }
        //     if (auto* store = inst->As<core::ir::Store>()) {
        //         // Must be storing to a function or private variable
        //         auto* to_ptr = store->To()->Type()->As<core::type::Pointer>();
        //         if (to_ptr->AddressSpace() != core::AddressSpace::kFunction &&
        //             to_ptr->AddressSpace() != core::AddressSpace::kPrivate) {
        //             continue;
        //         }
        //         // Must be storing to a struct member, so expect an access
        //         auto* to = store->To()->As<core::ir::InstructionResult>();
        //         if (!to) {
        //             continue;
        //         }
        //         auto* to_access = to->Instruction()->As<core::ir::Access>();
        //         if (!to_access) {
        //             continue;
        //         }
        //         // Must be indexing an array in a struct, so the access should have at least two
        //         // indices
        //         if (to_access->Indices().Length() < 2) {
        //             continue;
        //         }

        //         // We need to follow the access chain, looking for a triple access chain of
        //         // struct.array[dyn_index]. We only need to stop on the very first one we find,
        //         // and create a copy of that to a local variable.
        //         //
        //         // Possibilities:
        //         //
        //         // >> struct.array[dyn_index]
        //         // %acc:ptr<function, InnerS, read_write> = access %s1, 0u, %i
        //         //
        //         // >> struct.struct.array[dyn_index]
        //         // %acc:ptr<function, InnerS, read_write> = access %s1, 0u, 0u, %i
        //         //
        //         // >> struct.array[0].struct.array[dyn_index]
        //         // >> struct.array[dyn_indx].struct.array[0]
        //         //
        //         // >> struct.array[dyn_indx].struct.array[dyn_index]
        //         // %acc:ptr<function, InnerS, read_write> = access %s, 0u, %i, 0u, %i

        //         // ...
        //         // >> Split accesses:
        //         // >> struct.array[dyn_index]
        //         // %acc1:ptr<function, array<InnerS, 8>, read_write> = access %s1, 0u
        //         // %acc2:ptr<function, InnerS, read_write> = access %acc1, %i
        //         //
        //         // >> struct.struct.array[dyn_index]
        //         // %acc1:ptr<function, S1, read_write> = access %s1, 0u
        //         // %acc2:ptr<function, array<InnerS, 8>, read_write> = access %acc1, 0u
        //         // %acc3:ptr<function, InnerS, read_write> = access %acc2, %i
        //         //
        //         // >> struct.array[0].struct.array[dyn_index]
        //         // >> struct.array[dyn_indx].struct.array[0]
        //         //
        //         // >> struct.array[dyn_indx].struct.array[dyn_index]
        //         // %acc1:ptr<function, array<S1, 8>, read_write> = access %s, 0u
        //         // %acc2:ptr<function, S1, read_write> = access %acc1, %i
        //         // %acc3:ptr<function, array<InnerS, 8>, read_write> = access %acc2, 0u
        //         // %acc4:ptr<function, InnerS, read_write> = access %acc3, %i
        //         //
        //         //
        //         // Strategy: inline access chain into a single access, and look for
        //         // struct.array[dyn_index].

        //         // The array (second) index must be dynamic (not constant)
        //         if (to_access->Indices()[1]->Is<core::ir::Constant>()) {
        //             continue;
        //         }
        //         // Must be a struct
        //         auto* to_access_ptr = to_access->Object()->Type()->As<core::type::Pointer>();
        //         auto* str = to_access_ptr->StoreType()->As<core::type::Struct>();
        //         if (!str) {
        //             continue;
        //         }
        //         // Index into struct must be constant (this is always true)
        //         auto* idx0 = (to_access->Indices()[0])->As<core::ir::Constant>();
        //         if (!idx0) {
        //             continue;
        //         }
        //         // Must be an array in the struct
        //         auto* str_member = str->Members()[idx0->Value()->ValueAs<uint32_t>()];
        //         auto* array_ty = str_member->Type()->As<core::type::Array>();
        //         if (!array_ty) {
        //             continue;
        //         }

        //         // We got one! Replace the direct store with a store to a local copy of the
        //         array,
        //         // and copy it back.
        //         b.InsertBefore(store, [&] {
        //             // Create an access to the array in the struct to copy from
        //             auto* array_access = b.Access(ty.ptr(to_ptr->AddressSpace(), array_ty),
        //                                           to_access->Object(), idx0);
        //             // Copy the struct array to a local variable
        //             auto* local_array = b.Var("tint_array_copy", b.Load(array_access));
        //             // Store the previous store's From to the local array at the same index
        //             auto* local_array_value_access =
        //                 b.Access(ty.ptr<function>(to_ptr->StoreType()), local_array,
        //                          ToVector<4>(to_access->Indices().Offset(1)));
        //             b.Store(local_array_value_access, store->From());
        //             // Finally, copy back the data from the local array to the struct array
        //             b.Store(array_access, b.Load(local_array));
        //             // We can now remove the old store
        //             store->Destroy();
        //         });
        //     }
        // }
    }
};

}  // namespace

Result<SuccessType> LocalizeStructArrayAssignment(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "LocalizeStructArrayAssignment transform");
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::hlsl::writer::raise
