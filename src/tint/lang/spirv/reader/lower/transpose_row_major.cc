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

#include "src/tint/lang/spirv/reader/lower/transpose_row_major.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/spirv/type/explicit_layout_array.h"

namespace tint::spirv::reader::lower {
namespace {

using namespace tint::core::fluent_types;  // NOLINT

constexpr std::string_view kTintLoadRowMajor = "tint_load_row_major_column";
constexpr std::string_view kTintStoreRowMajor = "tint_store_row_major_column";

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The symbol manager.
    SymbolTable& sym{ir.symbols};

    // A map from a (type, is_row_major) pair to it's replacement (which maybe the same as the
    // original).
    struct TypeAndRowMajor {
        const core::type::Type* type;
        bool row_major;

        bool operator==(const TypeAndRowMajor& other) const {
            return type == other.type && row_major == other.row_major;
        }

        tint::HashCode HashCode() const { return Hash(type, row_major); }
    };
    Hashmap<TypeAndRowMajor, const core::type::Type*, 32> original_to_transposed{};

    /// A map from rewritten structs to original structs.
    Hashmap<const core::type::Struct*, const core::type::Struct*, 4> struct_to_original{};

    /// List of instruction results where the usages need to be updated
    Vector<core::ir::InstructionResult*, 32> results_to_update{};

    /// A hash of access instructions to the vector index they were accessing
    Hashmap<core::ir::Value*, core::ir::Value*, 8> access_to_vector_index{};

    /// A list of instructions to remove
    Vector<core::ir::Instruction*, 8> instructions_to_remove_if_unused{};

    /// A map of type to the load helper
    Hashmap<const core::type::Type*, core::ir::Function*, 4> load_functions{};

    /// A map of type to the store helper
    Hashmap<const core::type::Type*, core::ir::Function*, 4> store_functions{};

    /// Process the module.
    void Process() {
        for (auto* type : ty) {
            auto* strct = type->As<core::type::Struct>();
            if (!strct) {
                continue;
            }

            auto* replacement = RewriteType(strct, false);
            if (replacement == strct) {
                continue;
            }
        }

        for (auto* inst : ir.Instructions()) {
            for (auto* res : inst->Results()) {
                auto* new_ty = RewriteType(res->Type(), false);
                if (new_ty != res->Type()) {
                    res->SetType(new_ty);
                    results_to_update.Push(res);
                }
            }
        }

        while (!results_to_update.IsEmpty()) {
            auto* result = results_to_update.Pop();

            // Use sorted usages to force a copy, the replacements may create new usages of this
            // result.
            for (auto& usage : result->UsagesSorted()) {
                tint::Switch(
                    usage.instruction,  //
                    [&](core::ir::Load* ld) { ReplaceLoad(ld); },
                    [&](core::ir::Store* store) { ReplaceStore(store); },
                    [&](core::ir::Let* let) { ReplaceLet(let); },
                    [&](core::ir::Access* access) { ReplaceAccess(access); },
                    [&](core::ir::Call*) {
                        // TODO(dsinclair): Remove
                        TINT_UNREACHABLE() << " call";
                    },
                    [&](core::ir::Construct*) {
                        // TODO(dsinclair): Remove
                        TINT_UNREACHABLE() << " construct";
                    },
                    [&](core::ir::LoadVectorElement* lve) { ReplaceLoadVectorElement(lve); },
                    [&](core::ir::StoreVectorElement* sve) { ReplaceStoreVectorElement(sve); },
                    TINT_ICE_ON_NO_MATCH);
            }
        }

        for (auto* inst : instructions_to_remove_if_unused) {
            // If we've detached, just remove the instruction
            if (inst->Results().IsEmpty()) {
                inst->Destroy();
                continue;
            }

            TINT_ASSERT(inst->Results().Length() == 1);
            if (!inst->Result()->IsUsed()) {
                inst->Destroy();
            }
        }
    }

    void ReplaceLet(core::ir::Let* let) {
        let->Result()->SetType(let->Value()->Type());
        results_to_update.Push(let->Result());
    }

    // This is a store vector element that is going to a matrix which we've transposed the size of.
    // So, we need to swap the index on this store with the last index of the source access.
    void ReplaceStoreVectorElement(core::ir::StoreVectorElement* sve) {
        auto* src_to = sve->To()->As<core::ir::InstructionResult>();
        TINT_ASSERT(src_to);

        auto* src_access = src_to->Instruction()->As<core::ir::Access>();
        TINT_ASSERT(src_access);

        auto access_idx = access_to_vector_index.Get(sve->To());
        TINT_ASSERT(access_idx);

        core::ir::Access* new_access = nullptr;
        b.InsertAfter(src_access, [&] {
            Vector<core::ir::Value*, 4> indices;
            auto old_indices = src_access->Indices();

            // We push all old indices here because we've already removed the vector index,
            for (size_t i = 0; i < old_indices.Length(); ++i) {
                indices.Push(old_indices[i]);
            }
            indices.Push(sve->Index());

            auto* src_ty = src_to->Type()->As<core::type::Pointer>();
            TINT_ASSERT(src_ty);

            auto* src_mat = src_ty->StoreType()->As<core::type::Matrix>();
            TINT_ASSERT(src_mat);

            auto* new_ptr =
                ty.ptr(src_ty->AddressSpace(), ty.vec(src_mat->Type(), src_mat->Rows()));
            new_access = b.Access(new_ptr, src_access->Object(), indices);

            b.InsertAfter(sve,
                          [&] { b.StoreVectorElement(new_access, *access_idx, sve->Value()); });
            sve->Destroy();
        });

        instructions_to_remove_if_unused.Push(src_access);
    }

    // This is a load vector element that is coming from a matrix which we've transposed the size
    // of. So, we need to swap the index on this load with the last index of the source access.
    void ReplaceLoadVectorElement(core::ir::LoadVectorElement* lve) {
        auto* src_result = lve->From()->As<core::ir::InstructionResult>();
        TINT_ASSERT(src_result);

        auto* src_access = src_result->Instruction()->As<core::ir::Access>();
        TINT_ASSERT(src_access);

        auto access_idx = access_to_vector_index.Get(lve->From());
        TINT_ASSERT(access_idx);

        core::ir::Access* new_access = nullptr;
        b.InsertAfter(src_access, [&] {
            Vector<core::ir::Value*, 4> indices;
            auto old_indices = src_access->Indices();

            // We push all old indices here because we've already removed the vector index,
            for (size_t i = 0; i < old_indices.Length(); ++i) {
                indices.Push(old_indices[i]);
            }
            indices.Push(lve->Index());

            auto* src_ty = src_result->Type()->As<core::type::Pointer>();
            TINT_ASSERT(src_ty);

            auto* src_mat = src_ty->StoreType()->As<core::type::Matrix>();
            TINT_ASSERT(src_mat);

            auto* new_ptr =
                ty.ptr(src_ty->AddressSpace(), ty.vec(src_mat->Type(), src_mat->Rows()));
            new_access = b.Access(new_ptr, src_access->Object(), indices);

            b.InsertAfter(lve, [&] {
                b.LoadVectorElementWithResult(lve->DetachResult(), new_access, *access_idx);
            });
            lve->Destroy();
        });

        instructions_to_remove_if_unused.Push(src_access);
    }

    void ReplaceAccess(core::ir::Access* access) {
        bool indexed_through_row_major = false;

        auto* cur_ty = access->Object()->Type()->UnwrapPtr();
        const core::type::Type* parent_ty = nullptr;
        for (auto* idx : access->Indices()) {
            if (auto* struct_ty = cur_ty->As<core::type::Struct>()) {
                auto const_idx = idx->As<core::ir::Constant>()->Value()->ValueAs<uint32_t>();
                parent_ty = cur_ty;
                cur_ty = cur_ty->Element(const_idx);

                auto* orig_struct = struct_to_original.GetOr(struct_ty, nullptr);
                if (!orig_struct) {
                    // Structure didn't change, so doesn't contain a row-major member
                    continue;
                }

                auto* mem = orig_struct->Members()[const_idx];
                if (mem->RowMajor() && cur_ty->Is<core::type::Matrix>()) {
                    indexed_through_row_major = true;
                }

            } else {
                parent_ty = cur_ty;
                cur_ty = cur_ty->Elements().type;
            }
        }

        // Not accessing through a matrix, nothing to do.
        if (!indexed_through_row_major) {
            return;
        }

        // We've accessed the matrix itself, we need to update the access result type, and if
        // changed, update the uses of this access
        if (cur_ty->Is<core::type::Matrix>()) {
            auto* new_access_ty = RewriteType(access->Result()->Type(), true);
            if (new_access_ty != access->Result()->Type()) {
                access->Result()->SetType(new_access_ty);
                results_to_update.Push(access->Result());
            }
            return;
        }

        // We're accessing a row of the vector. We need to replace this access with an access of the
        // parent matrix and then store away which row we're accessing so we can rebuild any needed
        // accesses later.
        if (cur_ty->Is<core::type::Vector>()) {
            TINT_ASSERT(parent_ty != nullptr);
            auto* idx = access->PopLastIndex();

            /// The access now returns the transposed matrix
            auto* new_access_ty = access->Result()->Type();
            if (auto* access_ptr = access->Result()->Type()->As<core::type::Pointer>()) {
                new_access_ty = ty.ptr(access_ptr->AddressSpace(), parent_ty, access_ptr->Access());
            }
            access->Result()->SetType(new_access_ty);

            access_to_vector_index.Add(access->Result(), idx);
            results_to_update.Push(access->Result());
            return;
        }

        TINT_UNREACHABLE() << " access";
    }

    void ReplaceLoad(core::ir::Load* ld) {
        if (ld->Result()->Type()->Is<core::type::Matrix>()) {
            b.InsertAfter(ld, [&] {
                // We're replacing the load, which means the source must have been a transposed
                // matrix, so we need to get the load result as if it was row-major decorated.
                auto* new_res = b.InstructionResult(RewriteType(ld->Result()->Type(), true));
                b.CallWithResult(ld->DetachResult(), core::BuiltinFn::kTranspose, new_res);
                ld->SetResult(new_res);
            });
        } else if (ld->Result()->Type()->Is<core::type::Vector>()) {
            auto idx = access_to_vector_index.Get(ld->From());
            TINT_ASSERT(idx);

            if (idx) {
                // We're loading a vector from an access chain, we need to determine if this
                // vector came from a row-major matrix

                auto* load_fn = LoadColumnHelper(ld->From()->Type()->As<core::type::Pointer>());
                b.InsertAfter(ld, [&] {
                    auto* v = *idx;
                    if (v->Type()->Is<core::type::I32>()) {
                        v = b.Convert(ty.u32(), v)->Result();
                    }
                    b.CallWithResult(ld->DetachResult(), load_fn, ld->From(), v);
                });

                // Do this after we're done so we don't end up modifying the usage list of the
                // result we're iterating over.
                instructions_to_remove_if_unused.Push(ld);

            } else {
                // TODO(dsinclair): Remove
                TINT_UNREACHABLE() << " load vec not access";
            }
        }
    }

    void ReplaceStore(core::ir::Store* store) {
        auto vec_idx = access_to_vector_index.Get(store->To());

        auto* to_ty = store->To()->Type()->UnwrapPtr();
        if (!vec_idx) {
            // Storing the full matrix
            TINT_ASSERT(to_ty->Is<core::type::Matrix>());
            b.InsertBefore(store, [&] {
                auto* from = b.Call(to_ty, core::BuiltinFn::kTranspose, store->From());
                store->SetFrom(from->Result());
            });
        } else {
            // Storing a vector
            auto* store_fn = StoreColumnHelper(store->To()->Type()->As<core::type::Pointer>());
            b.InsertAfter(store, [&] {
                auto* v = *vec_idx;
                if (v->Type()->Is<core::type::I32>()) {
                    v = b.Convert(ty.u32(), v)->Result();
                }

                b.Call(ty.void_(), store_fn, store->To(), v, store->From());
            });
            instructions_to_remove_if_unused.Push(store);
        }
    }

    // Note, the type provided in the pointer has _already_ been transposed.
    core::ir::Function* StoreColumnHelper(const core::type::Pointer* ptr) {
        return store_functions.GetOrAdd(ptr, [&] {
            TINT_ASSERT(ptr);

            auto* row_major_ty = ptr->UnwrapPtr()->As<core::type::Matrix>();
            TINT_ASSERT(row_major_ty);

            auto* vec_ty = ty.vec(row_major_ty->Type(), row_major_ty->Columns());
            auto* col_ptr_ty =
                ty.ptr(ptr->AddressSpace(), ty.vec(row_major_ty->Type(), row_major_ty->Rows()),
                       ptr->Access());

            auto* fn = b.Function(kTintStoreRowMajor, ty.void_());
            auto* mat = b.FunctionParam(ptr);
            auto* row = b.FunctionParam(ty.u32());
            auto* col = b.FunctionParam(vec_ty);
            fn->SetParams({mat, row, col});

            b.Append(fn->Block(), [&] {
                for (uint32_t i = 0; i < row_major_ty->Columns(); ++i) {
                    auto* col_access = b.Access(vec_ty->DeepestElement(), col, u32(i));
                    b.StoreVectorElement(b.Access(col_ptr_ty, mat, u32(i)), row, col_access);
                }
                b.Return(fn);
            });
            return fn;
        });
    }

    // Note, the type provided in the pointer has _already_ been transposed.
    core::ir::Function* LoadColumnHelper(const core::type::Pointer* ptr) {
        return load_functions.GetOrAdd(ptr, [&] {
            TINT_ASSERT(ptr);

            auto* row_major_ty = ptr->UnwrapPtr()->As<core::type::Matrix>();
            TINT_ASSERT(row_major_ty);

            auto* vec_ty = ty.vec(row_major_ty->Type(), row_major_ty->Columns());
            auto* col_ptr_ty =
                ty.ptr(ptr->AddressSpace(), ty.vec(row_major_ty->Type(), row_major_ty->Rows()),
                       ptr->Access());

            auto* fn = b.Function(kTintLoadRowMajor, vec_ty);
            auto* mat = b.FunctionParam(ptr);
            auto* row = b.FunctionParam(ty.u32());
            fn->SetParams({mat, row});

            b.Append(fn->Block(), [&] {
                Vector<core::ir::Value*, 4> values;
                for (uint32_t i = 0; i < row_major_ty->Columns(); ++i) {
                    values.Push(
                        b.LoadVectorElement(b.Access(col_ptr_ty, mat, u32(i)), row)->Result());
                }
                b.Return(fn, b.Construct(vec_ty, values));
            });
            return fn;
        });
    }

    const core::type::Type* RewriteType(const core::type::Type* type, bool decorated_row_major) {
        return original_to_transposed.GetOrAdd(TypeAndRowMajor{type, decorated_row_major}, [&] {
            return tint::Switch(
                type,
                [&](const core::type::Array* arr) {
                    return RewriteArray(arr, decorated_row_major);
                },
                [&](const core::type::Struct* str) { return RewriteStruct(str); },
                [&](const core::type::Matrix* mat) {
                    if (!decorated_row_major) {
                        return mat;
                    }
                    return ty.mat(mat->Type(), mat->Rows(), mat->Columns());
                },
                [&](const core::type::Pointer* ptr) {
                    return ty.ptr(ptr->AddressSpace(),
                                  RewriteType(ptr->StoreType(), decorated_row_major),
                                  ptr->Access());
                },
                [&](Default) { return type; });
        });
    }

    const core::type::Type* RewriteArray(const core::type::Array* arr, bool decorated_row_major) {
        auto* elem_ty = RewriteType(arr->ElemType(), decorated_row_major);
        if (elem_ty == arr->ElemType()) {
            return arr;
        }

        // The element type is the only thing that will change. That does not affect the stride of
        // the array itself, which may either be the natural stride or an larger stride in the case
        // of an explicitly laid out array.
        if (arr->Is<spirv::type::ExplicitLayoutArray>()) {
            return ty.Get<spirv::type::ExplicitLayoutArray>(elem_ty, arr->Count(), arr->Align(),
                                                            arr->Size(), arr->Stride());
        }
        return ty.Get<core::type::Array>(elem_ty, arr->Count(), arr->Align(), arr->Size(),
                                         arr->Stride(), arr->Stride());
    }

    const core::type::Type* RewriteStruct(const core::type::Struct* old_struct) {
        bool made_changes = false;

        Vector<const core::type::StructMember*, 8> new_members;
        new_members.Reserve(old_struct->Members().Length());
        for (auto* member : old_struct->Members()) {
            auto* new_member_type = RewriteType(member->Type(), member->RowMajor());
            if (member->RowMajor() || new_member_type != member->Type()) {
                // Recreate the struct member without the row major attribute, using the new type.
                new_members.Push(ty.Get<core::type::StructMember>(
                    member->Name(), new_member_type, member->Index(), member->Offset(),
                    member->Align(), member->Size(), member->Attributes()));
                made_changes = true;
            } else {
                new_members.Push(member);
            }
        }
        if (!made_changes) {
            return old_struct;
        }

        // Create the new struct and record the mapping to the old struct.
        auto* new_struct = ty.Struct(sym.New(old_struct->Name().Name()), std::move(new_members));
        struct_to_original.Add(new_struct, old_struct);

        return new_struct;
    }
};

}  // namespace

Result<SuccessType> TransposeRowMajor(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "spirv.TransposeRowMajor",
                                          core::ir::Capabilities{
                                              core::ir::Capability::kAllowStructMatrixDecorations,
                                              core::ir::Capability::kAllowNonCoreTypes,
                                              core::ir::Capability::kAllowOverrides,
                                          });
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::reader::lower
