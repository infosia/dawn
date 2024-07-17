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

#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/sampler.h"
#include "src/tint/lang/core/type/texture.h"

namespace tint::spirv::writer::raise {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    // Process the module.
    void Process() {
        // First find the sampler var so that we can save its binding point.
        BindingPoint sampler_binding_point;
        for (auto* inst : ir.Instructions()) {
            if (auto* var = inst->As<core::ir::Var>()) {
                for (auto* result : inst->Results()) {
                    auto* type = result->Type()->UnwrapPtr();

                    if (type->As<core::type::Sampler>()) {
                        sampler_binding_point = var->BindingPoint().value();
                    }
                }
            }
        }

        // Now find the texture var so that we can adjust its binding point
        // (Dawn inserts the texture into the bind group at the binding for the
        // YCbCr sampler).
        for (auto* inst : ir.Instructions()) {
            if (auto* var = inst->As<core::ir::Var>()) {
                for (auto* result : inst->Results()) {
                    auto* type = result->Type()->UnwrapPtr();
                    auto* texture_ty = type->As<core::type::Texture>();
                    if (texture_ty) {
                        var->SetBindingPoint(sampler_binding_point.group,
                                             sampler_binding_point.binding);
                    }
                }
            }
        }
    }
};

}  // namespace

Result<SuccessType> YCbCr(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "YCbCr transform");
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::writer::raise
