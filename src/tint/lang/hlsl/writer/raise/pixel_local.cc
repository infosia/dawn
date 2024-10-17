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

#include "src/tint/lang/hlsl/writer/raise/pixel_local.h"

#include "src/tint/lang/core/ir/block.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/exit.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/switch.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/hlsl/builtin_fn.h"
#include "src/tint/lang/hlsl/ir/builtin_call.h"
#include "src/tint/lang/hlsl/ir/member_builtin_call.h"
#include "src/tint/lang/hlsl/type/rasterizer_ordered_texture_2d.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/result/result.h"

namespace tint::hlsl::writer::raise {
namespace {

using namespace tint::core::fluent_types;  // NOLINT

/// PIMPL state for the transform.
struct State {
    // Config options
    const PixelLocalOptions& options;

    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// Process the module.
    void Process() {
        TINT_ASSERT(options.attachments.size() == options.attachment_formats.size());
        if (options.attachments.size() == 0) {
            return;
        }

        // Inline pointers
        for (auto* inst : ir.Instructions()) {
            if (auto* l = inst->As<core::ir::Let>()) {
                if (l->Result(0)->Type()->Is<core::type::Pointer>()) {
                    l->Result(0)->ReplaceAllUsesWith(l->Value());
                    l->Destroy();
                }
            }
        }

        // Find the pixel_local module var, if any
        if (!ir.root_block) {
            return;
        }
        core::ir::Var* pixel_local_var = nullptr;
        const core::type::Struct* pixel_local_struct = nullptr;
        for (auto* inst : *ir.root_block) {
            if (auto* var = inst->As<core::ir::Var>()) {
                auto* ptr = var->Result(0)->Type()->As<core::type::Pointer>();
                if (ptr->AddressSpace() == core::AddressSpace::kPixelLocal) {
                    pixel_local_var = var;
                    pixel_local_struct = ptr->StoreType()->As<core::type::Struct>();
                    break;
                }
            }
        }
        if (!pixel_local_var || !pixel_local_var->Result(0)->IsUsed()) {
            return;
        }
        if (!pixel_local_struct) {
            TINT_ICE() << "pixel_local var must be of struct type";
        }
        if (pixel_local_struct->Members().Length() != options.attachments.size()) {
            TINT_ICE() << "missing options for each member of the pixel_local struct";
        }

        // One of the entry points must have a @position input as the last parameter, which should
        // have been inserted by ShaderIO if needed.
        core::ir::Function* entry_point = nullptr;
        core::ir::FunctionParam* entry_point_param = nullptr;
        for (auto f : ir.functions) {
            if (f->Stage() == core::ir::Function::PipelineStage::kFragment ||
                f->Stage() == core::ir::Function::PipelineStage::kVertex) {
                for (auto* p : f->Params()) {
                    if (auto* str = p->Type()->As<core::type::Struct>()) {
                        if (str->Members().Back()->Attributes().builtin ==
                            core::BuiltinValue::kPosition) {
                            entry_point = f;
                            entry_point_param = p;
                            break;
                        }
                    }
                }
            }
        }
        if (!entry_point) {
            TINT_ICE() << "expected a fragment or vertex entry point with a @position input";
        }

        // Get the entry point's single return instruction. We expect only one as this transform
        // should run after ShaderIO.
        core::ir::Return* entry_point_ret = nullptr;
        if (entry_point->UsagesUnsorted().Count() == 1) {
            entry_point_ret =
                entry_point->UsagesUnsorted().begin()->instruction->As<core::ir::Return>();
        }
        if (!entry_point_ret) {
            TINT_ICE() << "expected entry point with a single return";
        }

        // Create ROVs for each member of the struct
        Vector<core::ir::Var*, 4> rovs;
        Vector<core::type::Type*, 4> rov_subtypes;
        for (size_t i = 0; i < pixel_local_struct->Members().Length(); ++i) {
            auto options_texel_format = options.attachment_formats.find(static_cast<uint32_t>(i));
            auto options_binding = options.attachments.find(static_cast<uint32_t>(i));
            if (options_texel_format == options.attachment_formats.end() ||
                options_binding == options.attachments.end()) {
                TINT_ICE() << "missing options for member at index " << i;
            }
            const core::type::StructMember* mem = pixel_local_struct->Members()[i];

            core::TexelFormat texel_format;
            switch (options_texel_format->second) {
                case PixelLocalOptions::TexelFormat::kR32Sint:
                    texel_format = core::TexelFormat::kR32Sint;
                    break;
                case PixelLocalOptions::TexelFormat::kR32Uint:
                    texel_format = core::TexelFormat::kR32Uint;
                    break;
                case PixelLocalOptions::TexelFormat::kR32Float:
                    texel_format = core::TexelFormat::kR32Float;
                    break;
                default:
                    TINT_ICE() << "missing texel format for pixel local storage attachment";
            }
            auto* subtype = hlsl::type::RasterizerOrderedTexture2D::SubtypeFor(texel_format, ty);

            auto* rov_ty = ty.Get<hlsl::type::RasterizerOrderedTexture2D>(texel_format, subtype);
            auto* rov = b.Var("pixel_local_" + mem->Name().Name(), ty.ptr<handle>(rov_ty));
            rov->SetBindingPoint(options.group_index, options_binding->second);

            ir.root_block->Append(rov);
            rovs.Push(rov);
            rov_subtypes.Push(subtype);
        }

        // Change the address space of the var from 'pixel_local' to 'private'
        pixel_local_var->Result(0)->SetType(ty.ptr<private_>(pixel_local_struct));
        // As well as the usages
        for (auto& usage : pixel_local_var->Result(0)->UsagesUnsorted()) {
            if (auto* ptr = usage->instruction->Result(0)->Type()->As<core::type::Pointer>()) {
                if (ptr->AddressSpace() == core::AddressSpace::kPixelLocal) {
                    usage->instruction->Result(0)->SetType(ty.ptr<private_>(ptr->StoreType()));
                }
            }
        }

        auto build_coord = [&]() {
            auto* ep_struct = entry_point_param->Type()->As<core::type::Struct>();
            auto last_member_index = ep_struct->Members().Length() - 1;
            auto* coord_ty = ep_struct->Members().Back()->Type();
            core::ir::Instruction* coord =
                b.Access(coord_ty, entry_point_param, u32(last_member_index));
            coord = b.Swizzle(ty.vec2(coord_ty), coord, {0, 1});
            coord = b.Convert<vec2<u32>>(coord);  // Input type to .Load
            return coord;
        };

        // Insert copy from ROVs to the struct at the entry point start
        b.InsertBefore(entry_point->Block()->Front(), [&] {
            auto* coord = build_coord();
            for (size_t i = 0; i < pixel_local_struct->Members().Length(); ++i) {
                const core::type::StructMember* mem = pixel_local_struct->Members()[i];
                core::ir::Var* rov = rovs[i];
                // Note that the ROV type and the struct member type may not match, in which case we
                // need to convert
                auto* mem_ty = mem->Type();
                TINT_ASSERT(mem_ty->Is<core::type::Scalar>());
                auto* rov_ty = rov_subtypes[i];

                core::ir::Instruction* from = b.Load(rov);
                // Load returns a vec4, so we need to swizzle the first element
                from = b.MemberCall<hlsl::ir::MemberBuiltinCall>(
                    ty.vec4(rov_ty), tint::hlsl::BuiltinFn::kLoad, from, coord);
                from = b.Swizzle(rov_ty, from, {0});
                if (mem_ty != rov_ty) {
                    from = b.Convert(mem_ty, from);
                }
                auto* to = b.Access(ty.ptr<private_>(mem_ty), pixel_local_var, u32(i));
                b.Store(to, from);
            }
        });

        // Similarly, insert a copy from the struct back to ROVs at the return point
        b.InsertBefore(entry_point_ret, [&] {
            auto* coord = build_coord();
            for (size_t i = 0; i < pixel_local_struct->Members().Length(); ++i) {
                const core::type::StructMember* mem = pixel_local_struct->Members()[i];
                core::ir::Var* rov = rovs[i];
                // Note that the ROV type and the struct member type may not match, in which case we
                // need to convert
                auto* mem_ty = mem->Type();
                TINT_ASSERT(mem_ty->Is<core::type::Scalar>());
                auto* rov_ty = rov_subtypes[i];

                core::ir::Instruction* from =
                    b.Access(ty.ptr<private_>(mem_ty), pixel_local_var, u32(i));
                if (mem_ty != rov_ty) {
                    from = b.Convert(rov_ty, from);
                }
                // Store requires a vec4
                from = b.Swizzle(ty.vec4(rov_ty), from, {0, 0, 0, 0});
                core::ir::Instruction* to = b.Load(rov);
                b.Call<hlsl::ir::BuiltinCall>(  //
                    ty.void_(), hlsl::BuiltinFn::kTextureStore, to, coord, from);
            }
        });
    }
};

}  // namespace

Result<SuccessType> PixelLocal(core::ir::Module& ir, const PixelLocalConfig& config) {
    auto result = ValidateAndDumpIfNeeded(ir, "PixelLocal transform");
    if (result != Success) {
        return result.Failure();
    }

    State{config.options, ir}.Process();

    return Success;
}

}  // namespace tint::hlsl::writer::raise
