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

#include "src/tint/cmd/fuzz/ir/fuzz.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/input_attachment.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/glsl/writer/helpers/generate_bindings.h"
#include "src/tint/lang/glsl/writer/writer.h"

namespace tint::glsl::writer {
namespace {

bool CanRun(const core::ir::Module& module, Options& options) {
    // Excessive values can cause OOM / timeouts in the PadStructs transform.
    static constexpr uint32_t kMaxOffset = 0x1000;

    if (options.first_instance_offset && options.first_instance_offset > kMaxOffset) {
        return false;
    }

    if (options.first_vertex_offset && options.first_vertex_offset > kMaxOffset) {
        return false;
    }

    if (options.depth_range_offsets) {
        if (options.depth_range_offsets->max > kMaxOffset ||
            options.depth_range_offsets->min > kMaxOffset) {
            return false;
        }
    }

    // Make sure that every texture variable is in the texture_builtins_from_uniform binding list,
    // otherwise TextureBuiltinsFromUniform will fail.
    for (auto* inst : *module.root_block) {
        auto* var = inst->As<core::ir::Var>();

        if (!var || !var->Result(0)->Type()->UnwrapPtr()->Is<core::type::Texture>()) {
            continue;
        }

        bool found = false;
        auto binding_point = var->BindingPoint();
        for (auto& bp : options.bindings.texture_builtins_from_uniform.ubo_bindingpoint_ordering) {
            if (bp == binding_point) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    // Make sure that there is at most one entry point.
    bool has_entry_point = false;
    for (auto& func : module.functions) {
        if (func->Stage() != core::ir::Function::PipelineStage::kUndefined) {
            if (has_entry_point) {
                return false;
            }
            has_entry_point = true;
        }
    }

    return true;
}

void IRFuzzer(core::ir::Module& module, Options options) {
    if (!CanRun(module, options)) {
        return;
    }

    options.bindings = GenerateBindings(module);

    [[maybe_unused]] auto output = Generate(module, options, "");
}

}  // namespace
}  // namespace tint::glsl::writer

TINT_IR_MODULE_FUZZER(tint::glsl::writer::IRFuzzer);
