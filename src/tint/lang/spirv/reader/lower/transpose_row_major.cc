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

    // A map from a type to it's replacement (which maybe the same as the original).
    Hashmap<const core::type::Type*, const core::type::Type*, 32> type_map{};

    /// A map from rewritten structs to original structs.
    Hashmap<const core::type::Struct*, const core::type::Struct*, 4> struct_to_original{};

    /// Process the module.
    void Process() {
        for (auto* type : ty) {
            auto* strct = type->As<core::type::Struct>();
            if (!strct) {
                continue;
            }

            auto* replacement = RewriteType(strct);
            if (replacement == strct) {
                continue;
            }
        }
    }

    const core::type::Type* RewriteType(const core::type::Type* type,
                                        bool decorated_row_major = false) {
        return type_map.GetOrAdd(type, [&] {
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

        printf("REWRITE %p\n", old_struct);

        Vector<const core::type::StructMember*, 8> new_members;
        new_members.Reserve(old_struct->Members().Length());
        for (auto* member : old_struct->Members()) {
            auto* new_member_type = RewriteType(member->Type(), member->RowMajor());
            printf("RM %d\n", member->RowMajor());
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

        printf("NEW %p\n", new_struct);
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
