// Copyright 2023 The Dawn & Tint Authors
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

#include "src/tint/lang/spirv/writer/raise/ycbcr.h"

#include <iostream>
#include <utility>

#include "spirv/unified1/spirv.h"
#include "src/dawn/common/Log.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/function_param.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/builtin_structs.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/input_attachment.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/texture.h"
#include "src/tint/lang/spirv/ir/builtin_call.h"
#include "src/tint/lang/spirv/ir/literal_operand.h"
#include "src/tint/lang/spirv/type/sampled_image.h"
#include "src/tint/utils/ice/ice.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::spirv::writer::raise {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    // Erases all usages of `value`. Expected usages are as a parameter to loads
    // and as a parameter in function calls.
    void RecursivelyErase(core::ir::Value* value) {
        // We will be mutating `value->Usages()` within the loop below, so it
        // is necessary to iterate over a copy of it.
        auto usages = value->Usages();
        for (auto& usage : usages) {
            auto* inst = usage.Value().instruction;
            if (inst->As<core::ir::Load>()) {
                // `value` was loaded: Erase all the usages of the load, followed by
                // erasing the load itself.
                for (auto* result : inst->Results()) {
                    RecursivelyErase(result);
                }
                inst->Destroy();
            } else if (auto* call = inst->As<core::ir::Call>()) {
                // `value` was passed in a function call: Erase it from the
                // operands.
                Vector<core::ir::Value*, 6> operands;
                for (auto* operand : call->Operands()) {
                    if (operand != value) {
                        operands.Push(operand);
                    }
                }
                call->SetOperands(operands);
            } else {
                // TODO(blundell): Assert that there are no other usages.
            }
        }
    }

    // Process the module.
    void Process() {
        // First find the sampler var so that we can save its binding point and
        // then recursively erase it.
        BindingPoint sampler_binding_point;
        for (auto* inst : ir.Instructions()) {
            if (auto* var = inst->As<core::ir::Var>()) {
                for (auto* result : inst->Results()) {
                    auto* type = result->Type()->UnwrapPtr();

                    if (type->As<core::type::Sampler>()) {
                        sampler_binding_point = var->BindingPoint().value();
                        RecursivelyErase(result);
                        inst->Destroy();
                    }
                }
            }
        }

        // Now find the texture var so that we can adjust its type and
        // binding point (Dawn inserts the texture into the bind group at the
        // binding for the static sampler).
        for (auto* inst : ir.Instructions()) {
            if (auto* var = inst->As<core::ir::Var>()) {
                for (auto* result : inst->Results()) {
                    auto* type = result->Type()->UnwrapPtr();
                    auto* texture_ty = type->As<core::type::Texture>();
                    if (texture_ty) {
                        auto* pointer = result->Type()->As<core::type::Pointer>();
                        result->SetType(ty.Get<core::type::Pointer>(
                            pointer->AddressSpace(), ty.Get<type::SampledImage>(texture_ty),
                            pointer->Access()));
                        var->SetBindingPoint(sampler_binding_point.group,
                                             sampler_binding_point.binding);

                        // Find any loads of the texture variable and update their
                        // result types as well.
                        for (auto& usage : result->Usages()) {
                            auto* usage_inst = usage->instruction;
                            if (usage_inst->As<core::ir::Load>()) {
                                for (auto* usage_result : usage_inst->Results()) {
                                    usage_result->SetType(ty.Get<type::SampledImage>(texture_ty));
                                }
                            } else {
                                // TODO(blundell): Assert that there are no other usages.
                            }
                        }
                    }
                }
            }
        }

        // Find and adjust usage of the sampler and texture within functions.
        for (auto* func : ir.DependencyOrderedFunctions()) {
            Vector<core::ir::FunctionParam*, 6> params;
            core::ir::FunctionParam* texture_param;

            // Adjust the function signature as necessary.
            for (auto* param : func->Params()) {
                // Drop the sampler param and modify the type of the texture
                // param.
                if (param->Type()->As<core::type::Sampler>()) {
                    continue;
                } else if (auto* texture_ty = param->Type()->As<core::type::Texture>()) {
                    // TODO(blundell): I presumably need to set the index of
                    // this param to the index of the sampler param. Right now
                    // it works because the texture param comes before the index
                    // param.
                    param->SetType(ty.Get<type::SampledImage>(texture_ty));
                    texture_param = param;
                }
                params.Push(param);
            }
            func->SetParams(params);

            // Find the instruction creating the sampled image via the
            // SampledImage() builtin call and replace usage of its results with
            // usage of the sampled image param before erasing it.
            for (auto* inst : *func->Block()) {
                bool destroy_inst = false;
                if (inst->As<core::ir::BuiltinCall>()) {
                    for (auto* result : inst->Results()) {
                        if (result->Type()->As<spirv::type::SampledImage>()) {
                            result->ReplaceAllUsesWith(texture_param);
                            destroy_inst = true;
                        }
                    }
                }
                if (destroy_inst) {
                    inst->Destroy();
                }
            }
        }
    }
};

}  // namespace

Result<SuccessType> YCbCr(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "YCbCr transform");
    std::cout << "Running YCbCr transform" << std::endl;
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::writer::raise
