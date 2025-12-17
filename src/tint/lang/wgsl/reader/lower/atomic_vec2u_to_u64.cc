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

#include "src/tint/lang/wgsl/reader/lower/atomic_vec2u_to_u64.h"

#include <cstdint>
#include <cstdio>
#include <utility>

#include <iostream>

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/builtin_call.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/atomic.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/wgsl/enums.h"
#include "src/tint/lang/wgsl/ir/builtin_call.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/function_param.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/memory_view.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/utils/containers/hashmap.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/text/string_stream.h"

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

    /// Map from struct member to its new index.
    Hashmap<const core::type::StructMember*, uint32_t, 4> member_index_map{};

    /// Map from a type to a helper function that will convert its rewritten form back to it.
    Hashmap<const core::type::Struct*, tint::core::ir::Function*, 4> convert_helpers{};

    /// Process the module.
    void Process() {
        printf("processing \n");
        fflush(stdout);
        if (ir.root_block->IsEmpty()) {
            return;
        }

        std::cout << "TESTING \n";
        // Find uniform buffers that contain matrices that need to be decomposed.
        Vector<std::pair<core::ir::Var*, const core::type::Type*>, 8> buffer_variables;
        for (auto inst : *ir.root_block) {
            if (auto* var = inst->As<tint::core::ir::Var>()) {
                auto* ptr = var->Result()->Type()->As<core::type::Pointer>();
                // atomic vec2u only supports storage
                if (!ptr || ptr->AddressSpace() != core::AddressSpace::kStorage) {
                    continue;
                }

                 
                std::cout << "tyep infor = \n" << ptr->TypeInfo().name;
               auto* store_type = RewriteType(ptr->StoreType());
                if (store_type != ptr->StoreType()) {
                      buffer_variables.Push(std::make_pair(var, store_type));
                }
            }
        }

        // Now process the buffer variables, replacing them with new variables that have decomposed
        // matrices and updating all usages of the variables.
        for (auto var_and_ty : buffer_variables) {
            // Create a new variable with the modified store type.
            auto* old_var = var_and_ty.first;

            old_var->Result()->SetType(ty.ptr(storage, var_and_ty.second));

            // Transform instructions that accessed the variable to use the decomposed var.
            old_var->Result()->ForEachUseSorted(
                [&](core::ir::Usage use) { Replace(use.instruction); });

        }

    }

     void Replace(core::ir::Instruction* inst) {
        b.InsertBefore(inst, [&] {
            tint::Switch(
                inst,  //
                [&](core::ir::Access* access) {
                    std::cout << "Modifying access \n";
                    auto* object_ty = access->Object()->Type()->As<core::type::MemoryView>();
                    if (!object_ty || object_ty->AddressSpace() != core::AddressSpace::kStorage) {
                        // Access to non-uniform memory views does not require transformation.
                        return;
                    }
                    
                    auto* replacement = RewriteType( access->Result()->Type());
                    std::cout << "Modifying access \n";
                    access->Result()->SetType(replacement);
                    // Replace every instruction that uses the original access instruction.
               
                    access->Result()->ForEachUseSorted(
                        [&](core::ir::Usage use) { Replace(use.instruction); });
                    //access->Destroy();
                },
                [&](wgsl::ir::BuiltinCall* call) {
                 
                     std::cout << "Modifying CoreBuiltinCall \n";
                    if(call->Func() == BuiltinFn::kAtomicStoreMax || call->Func() == BuiltinFn::kAtomicStoreMin)
                    {
                          auto* param0 = call->Args()[0];
                            auto* param1 = call->Args()[1];
                       auto casted_u64 =  b.Bitcast(ty.u64(), param1);
                b.CallWithResult(call->DetachResult(),
                                                         core::BuiltinFn::kAtomicStoreMax, param0, casted_u64);
     
                    
                        call->Destroy();
                    }
                },
                [&](core::ir::Let* let) {
                         std::cout << "Modifying Let \n";
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
                           std::cout << "Is array type \n";
                    // Create a new array with element type potentially rewritten.
                    return ty.array(RewriteType(arr->ElemType()), arr->ConstantCount().value());
                },
                [&](const core::type::Struct* str) -> const core::type::Type* {
                      std::cout << "Is struct type \n";
                    bool needs_rewrite = false;
                    uint32_t member_index = 0;
                    Vector<const core::type::StructMember*, 4> new_members;
                    for (auto* member : str->Members()) {
       
                            // For all other types, recursively rewrite them as necessary.
                            auto* new_member_ty = RewriteType(member->Type());
                            new_members.Push(ty.Get<core::type::StructMember>(
                                member->Name(), new_member_ty, member_index, member->Offset(),
                                member->Align(), member->Size(), core::IOAttributes{}));
                            member_index_map.Add(member, member_index);
                            member_index++;
                            if (new_member_ty != member->Type()) {
                                needs_rewrite = true;
                            }
                      
                    }

                    // If no members needed to be rewritten, just return the original struct.
                    if (!needs_rewrite) {
                        return str;
                    }

                    // Create a new struct with the rewritten members.
                    auto* new_str =
                        ty.Get<core::type::Struct>(sym.New(str->Name().Name() + "_a64"),
                                                   std::move(new_members), str->Size());
                    for (auto flag : str->StructFlags()) {
                        new_str->SetStructFlag(flag);
                    }
                    return new_str;
                },
               [&](const core::type::Pointer* ptr) {
                      std::cout << "Is ptr type \n";
                    // Create a new array with element type potentially rewritten.
                    return ty.ptr(ptr->AddressSpace(),  RewriteType(ptr->StoreType()) );
                },
                [&](const core::type::Atomic* atomic) -> const core::type::Type* {
                    std::cout << "Is atomic type \n";
                    if(atomic->Type()->Is< tint::core::type::Vector>()){
                        return ty.atomic(ty.u64());
                    }
                    // Create a new struct with the rewritten members.
                    return atomic;
                },
                [&](Default) {
                         std::cout << "Unknown type " << type->TypeInfo().name << " \n";
                    return type;
                });
        });
    }

    #if 0
    /// Replace a use of a value that conta
    /// @param inst the instruction to replace
    /// @param replacement the replacement value
    void Replace(tint::core::ir::Instruction* inst, tint::core::ir::Value* replacement) {
        b.InsertBefore(inst, [&] {
            tint::Switch(
                inst,  //
                [&](tint::core::ir::Access* access) {
                    auto* object_ty = access->Object()->Type()->As<core::type::MemoryView>();
                    if (!object_ty || object_ty->AddressSpace() != core::AddressSpace::kUniform) {
                        // Access to non-uniform memory views does not require transformation.
                        return;
                    }

                    if (!replacement->Type()->Is<core::type::MemoryView>()) {
                        // The replacement is a value, in which case the decomposed matrix has
                        // already been reconstructed. In this situation the access only needs its
                        // return type updating, and downstream instructions need updating.
                        access->SetOperand(tint::core::ir::Access::kObjectOperandOffset, replacement);
                        auto* result = access->Result();
                        result->SetType(result->Type()->UnwrapPtrOrRef());
                        result->ForEachUseSorted(
                            [&](tint::core::ir::Usage use) { Replace(use.instruction, result); });
                        return;
                    }

                    // Modify the access indices to take decomposed matrices into account.
                    auto* current_type = object_ty->StoreType();
                    Vector<tint::core::ir::Value*, 4> indices;

  
                    for (size_t i = 0, n = access->Indices().Length(); i < n; i++) {
                        auto* idx = access->Indices()[i];

                        if (auto* mat = NeedsDecomposing(current_type)) {
                            // Access chain passes through decomposed matrix.
                            if (auto* const_idx = idx->As<tint::core::ir::Constant>()) {
                                // Column vector index is a constant.
                                // Instead of loading the whole matrix, fold the access of the
                                // matrix and the constant column index into an single access of
                                // column vector member.
                                auto* base_idx = indices.Back()->As<tint::core::ir::Constant>();
                                indices.Back() =
                                    b.Constant(u32(base_idx->Value()->ValueAs<uint32_t>() +
                                                   const_idx->Value()->ValueAs<uint32_t>()));
                                current_type = mat->ColumnType();
                                i++;  // We've already consumed the column access
                            } else {
                                // Column vector index is dynamic.
                                // Reconstruct the whole matrix and index that.
                                replacement = RebuildMatrix(mat, replacement, std::move(indices));
                                indices.Clear();
                                indices.Push(idx);
                                current_type = mat->ColumnType();
                            }
                        } else if (auto* str = current_type->As<core::type::Struct>()) {
                            // Remap member index
                            uint32_t old_index = idx->As<tint::core::ir::Constant>()->Value()->ValueAs<uint32_t>();
                            uint32_t new_index = *member_index_map.Get(str->Members()[old_index]);
                            current_type = str->Element(old_index);
                            indices.Push(b.Constant(u32(new_index)));
                        } else {
                            indices.Push(idx);
                            current_type = current_type->Elements().type;
                            if (NeedsDecomposing(current_type)) {
                                // Decomposed matrices are indexed using their first column vector
                                indices.Push(b.Constant(0_u));
                            }
                        }
                    }

                    if (auto* mat = NeedsDecomposing(current_type)) {
                        replacement = RebuildMatrix(mat, replacement, std::move(indices));
                        indices.Clear();
                    }

                    if (!indices.IsEmpty()) {
                        // Emit the access with the modified indices.
                        if (replacement->Type()->Is<core::type::Pointer>()) {
                            current_type = ty.ptr(uniform, RewriteType(current_type));
                        }
                        auto* new_access = b.Access(current_type, replacement, std::move(indices));
                        replacement = new_access->Result();
                    }

                    // Replace every instruction that uses the original access instruction.
                    access->Result()->ForEachUseSorted(
                        [&](tint::core::ir::Usage use) { Replace(use.instruction, replacement); });
                    access->Destroy();
                },
                [&](tint::core::ir::Load* load) {
                    if (!replacement->Type()->Is<core::type::Pointer>()) {
                        // We have already loaded to a value type, so this load just folds away.
                        load->Result()->ReplaceAllUsesWith(replacement);
                    } else {
                        // Load the decomposed value and then convert it to the original type.
                        auto* decomposed = b.Load(replacement);
                        auto* converted =decomposed;
                        //load->Result()->ReplaceAllUsesWith(converted);
                    }
                    load->Destroy();
                },
                [&](tint::core::ir::LoadVectorElement* load) {
                    if (!replacement->Type()->Is<core::type::Pointer>()) {
                        // We have loaded a decomposed matrix and reconstructed it, so this is now
                        // extracting from a value type.
                        b.AccessWithResult(load->DetachResult(), replacement, load->Index());
                        load->Destroy();
                    } else {
                        // There was no decomposed matrix on the path to this instruction so just
                        // update the source operand.
                        load->SetOperand(tint::core::ir::LoadVectorElement::kFromOperandOffset, replacement);
                    }
                },
                [&](tint::core::ir::Let* let) {
                    // Let instructions just fold away.
                    let->Result()->ForEachUseSorted(
                        [&](tint::core::ir::Usage use) { Replace(use.instruction, replacement); });
                    let->Destroy();
                });
        });
    }
    #endif
};

}  // namespace

Result<SuccessType> AtomicVec2UToU64(tint::core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "lower.AtomicVec2UToU64",      core::ir::Capabilities{
                                              core::ir::Capability::kAllowMultipleEntryPoints,
                                              core::ir::Capability::kAllowOverrides,
                                              core::ir::Capability::kAllowResourceBinding,
                                              core::ir::Capability::kAllow8BitIntegers,
                                          });
    if (result != Success) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
