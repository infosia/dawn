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

#include "src/tint/lang/glsl/writer/raise/add_immediate_vars.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/utils/ice/ice.h"

using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The transform config.
    const AddImmediateVarsConfig& config;

    /// The IR module.
    Module& ir;

    /// The IR builder.
    Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    Var* AppendVar(const char* name, const core::type::Type* type, uint32_t location) {
        auto var = b.Var(name, core::AddressSpace::kImmediate, type);
        var->SetAttributes(IOAttributes{.location = location});
        ir.root_block->Append(var);
        return var;
    }

    Result<ImmediateVars> Run() {
        ImmediateVars result;
        if (config.first_vertex_location) {
            result.first_vertex =
                AppendVar("tint_first_vertex", ty.u32(), *config.first_vertex_location);
        }
        if (config.first_instance_location) {
            result.first_instance =
                AppendVar("tint_first_instance", ty.u32(), *config.first_instance_location);
        }
        if (config.min_depth_location) {
            result.min_depth =
                AppendVar("tint_frag_depth_min", ty.f32(), *config.min_depth_location);
        }
        if (config.max_depth_location) {
            result.max_depth =
                AppendVar("tint_frag_depth_max", ty.f32(), *config.max_depth_location);
        }
        return result;
    }
};

}  // namespace

Result<ImmediateVars> AddImmediateVars(Module& ir, const AddImmediateVarsConfig& config) {
    return State{config, ir}.Run();
}

}  // namespace tint::core::ir::transform
