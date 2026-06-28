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

#include "src/tint/lang/core/ir/analysis/minimum_binding_size.h"

#include <algorithm>
#include <utility>

#include "src/tint/lang/core/ir/access.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/user_call.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/i8.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/subgroup_matrix.h"
#include "src/tint/lang/core/type/u8.h"
#include "src/tint/utils/rtti/switch.h"

namespace tint::core::ir::analysis {
namespace {

struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The size info.
    BindingSizeInfo info{};

    /// Process the module.
    /// It is assumed that the IR has been validated so this analysis does not concern itself with
    /// overflows, etc.
    BindingSizeInfo Process() {
        Vector<const Var*, 4> var_worklist;
        for (auto* inst : *ir.root_block) {
            if (!inst->Is<Var>()) {
                continue;
            }

            auto* var = inst->As<Var>();
            if (!var->BindingPoint()) {
                continue;
            }
            auto ptr_ty = var->Result()->Type()->As<type::Pointer>();
            if (!ptr_ty || ptr_ty->AddressSpace() == AddressSpace::kHandle) {
                continue;
            }
            var_worklist.Push(var);
        }

        while (!var_worklist.IsEmpty()) {
            auto* var = var_worklist.Pop();
            auto bp = var->BindingPoint().value();

            auto* store_ty = var->Result()->Type()->UnwrapPtr();
            if (store_ty->HasFixedFootprint()) {
                info.binding_sizes[bp] = store_ty->Size();
            } else {
                info.binding_sizes[bp] = FindMinimumBindingSize(var);
            }
        }

        return std::move(info);
    }

    uint32_t FindMinimumBindingSize(const Var* var) {
        struct Info {
            Usage use;
            uint32_t offset;
        };

        uint32_t size = 0;
        auto* store_ty = var->Result()->Type()->UnwrapPtr();
        if (auto* str = store_ty->As<type::Struct>()) {
            auto* last = str->Members().Back();
            auto* arr_ty = last->Type()->As<type::Array>();
            size = last->Offset() + arr_ty->ImplicitStride();
        } else if (auto* arr = store_ty->As<type::Array>()) {
            size = arr->ImplicitStride();
        }

        Vector<Info, 8> worklist;
        for (auto& u : var->Result()->UsagesUnsorted()) {
            worklist.Push({u, 0});
        }

        while (!worklist.IsEmpty()) {
            auto size_info = worklist.Pop();

            tint::Switch(
                size_info.use.instruction,  //
                [&](const Let* l) {
                    for (auto& u : l->Result()->UsagesUnsorted()) {
                        worklist.Push({u, size_info.offset});
                    }
                },
                [&](const UserCall* user) {
                    auto* target = user->Target();
                    auto* param =
                        target->Params()[size_info.use.operand_index - user->ArgsOperandOffset()];
                    for (auto& u : param->UsagesUnsorted()) {
                        worklist.Push({u, size_info.offset});
                    }
                },
                [&](const Access* access) {
                    auto* obj_ty = access->Object()->Type()->UnwrapPtr();

                    uint32_t offset = 0;
                    for (auto* idx : access->Indices()) {
                        uint32_t idx_value = 0;
                        if (auto* const_idx = idx->As<Constant>()) {
                            idx_value = const_idx->Value()->ValueAs<uint32_t>();
                        }
                        // Matrix and vector can't be hit on the way to a subgroupMatrix and access
                        // won't be hit at all on the way to buffer[Array]View so we only handle
                        // structure and array here.
                        tint::Switch(
                            obj_ty,  //
                            [&](const core::type::Array* ary) {
                                obj_ty = ary->ElemType();
                                offset += idx_value * ary->ImplicitStride();
                            },
                            [&](const core::type::Struct* s) {
                                auto* mem = s->Members()[idx_value];
                                obj_ty = mem->Type();
                                offset += mem->Offset();
                            },
                            [&](Default) {});
                    }

                    for (auto& u : access->Result()->UsagesUnsorted()) {
                        worklist.Push({u, size_info.offset + offset});
                    }
                },
                [&](const CoreBuiltinCall* call) {
                    if (call->Func() == BuiltinFn::kBufferView ||
                        call->Func() == BuiltinFn::kBufferArrayView) {
                        auto [buffer_size, offset] = MinimumBufferSize(call, size_info.offset);
                        size = std::max(buffer_size, size);

                        if (!call->Result()->Type()->UnwrapPtr()->HasFixedFootprint()) {
                            for (auto& u : call->Result()->UsagesUnsorted()) {
                                worklist.Push({u, offset});
                            }
                        }
                        return;
                    }
                    // Only handling variants templated on majorness. Other variants are deprecated.
                    if ((call->Func() == BuiltinFn::kSubgroupMatrixLoad &&
                         call->ExplicitTemplateParams().Length() == 2) ||
                        (call->Func() == BuiltinFn::kSubgroupMatrixStore &&
                         call->ExplicitTemplateParams().Length() == 1)) {
                        uint32_t matrix_size = MinimumMatrixSize(call, size_info.offset);
                        size = std::max(matrix_size, size);
                    }
                },
                [&](Default) {});
        }

        return size;
    }

    std::pair<uint32_t, uint32_t> MinimumBufferSize(const CoreBuiltinCall* call, uint32_t offset) {
        auto* store_ty = call->Result()->Type()->UnwrapPtr();
        uint32_t min_type_size = 0;
        if (store_ty->HasFixedFootprint()) {
            min_type_size = store_ty->Size();
        } else if (auto* str = store_ty->As<type::Struct>()) {
            auto* last = str->Members().Back();
            auto* arr_ty = last->Type()->As<type::Array>();
            min_type_size = last->Offset() + arr_ty->ImplicitStride();
        } else {
            min_type_size = store_ty->As<type::Array>()->ImplicitStride();
        }

        uint32_t offset_value = offset;
        if (auto* const_offset = call->Args()[1]->As<Constant>()) {
            offset_value += const_offset->Value()->ValueAs<uint32_t>();
        }
        uint32_t size_value = min_type_size;
        if (call->Func() == BuiltinFn::kBufferArrayView) {
            if (auto* const_size = call->Args()[2]->As<Constant>()) {
                size_value = const_size->Value()->ValueAs<uint32_t>();
            }
        }

        return std::make_pair(size_value + offset_value, offset_value);
    }

    uint32_t MinimumMatrixSize(const CoreBuiltinCall* call, uint32_t offset) {
        const bool is_load = call->Func() == BuiltinFn::kSubgroupMatrixLoad;
        bool col_major = false;
        auto* offset_arg = call->Args()[1];
        const Value* stride_arg = nullptr;
        const type::Type* ty = nullptr;
        if (is_load) {
            col_major =
                std::get<Majorness>(call->ExplicitTemplateParams()[1]) == Majorness::kColMajor;
            stride_arg = call->Args()[2];
            ty = call->Result()->Type();
        } else {
            col_major =
                std::get<Majorness>(call->ExplicitTemplateParams()[0]) == Majorness::kColMajor;
            stride_arg = call->Args()[3];
            ty = call->Args()[2]->Type();
        }
        auto* mat_ty = ty->As<type::SubgroupMatrix>();
        auto* ele_ty = mat_ty->Type();
        const uint32_t major_size = col_major ? mat_ty->Columns() : mat_ty->Rows();
        const uint32_t minor_size = col_major ? mat_ty->Rows() : mat_ty->Columns();
        uint32_t stride_factor = 1;
        if (ele_ty->IsAnyOf<type::U8, type::I8>()) {
            stride_factor = 4;
        }

        uint32_t offset_value = offset;
        if (auto* const_offset = offset_arg->As<Constant>()) {
            offset_value +=
                stride_factor * ele_ty->Size() * const_offset->Value()->ValueAs<uint32_t>();
        }
        uint32_t min_stride = minor_size * ele_ty->Size();
        uint32_t stride_value = min_stride;
        if (auto* const_stride = stride_arg->As<Constant>()) {
            stride_value =
                stride_factor * ele_ty->Size() * const_stride->Value()->ValueAs<uint32_t>();
        }
        // offset_value and stride_value are in bytes.
        uint32_t mat_required_size = stride_value * (major_size - 1) + minor_size * ele_ty->Size();
        // Round up to shader scalar type size.
        mat_required_size =
            ((mat_required_size + stride_factor - 1) / stride_factor) * stride_factor;

        uint32_t mem_required_size = offset_value + mat_required_size;
        return mem_required_size;
    }
};

}  // namespace

BindingSizeInfo MinimumBindingSize(core::ir::Module& ir) {
    return State{ir}.Process();
}

}  // namespace tint::core::ir::analysis
