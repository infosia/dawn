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

#ifndef SRC_TINT_LANG_HLSL_WRITER_RAISE_NUM_WORKGROUPS_FROM_UNIFORM_H_
#define SRC_TINT_LANG_HLSL_WRITER_RAISE_NUM_WORKGROUPS_FROM_UNIFORM_H_

#include <optional>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/result/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}  // namespace tint::core::ir

namespace tint::hlsl::writer::raise {

/// Configuration options for the NumWorkgroupsFromUniform transform.
struct NumWorkgroupsFromUniformConfig {
    /// The binding point to use for the generated uniform buffer. If ubo_bp contains no value,
    /// a free binding point will be used. Specifically, binding 0 of the largest used group
    /// plus 1 is used if at least one resource is bound, otherwise group 0 binding 0 is used.
    std::optional<BindingPoint> ubo_binding;

    /// Reflection for this class
    TINT_REFLECT(NumWorkgroupsFromUniformConfig, ubo_binding);
};

/// NumWorkgroupsFromUniform is a transform that implements the `num_workgroups`
/// builtin by loading it from a uniform buffer.
///
/// The generated uniform buffer will have the form:
/// ```
/// struct num_workgroups_struct {
///  num_workgroups : vec3<u32>;
/// };
///
/// @group(0) @binding(0)
/// var<uniform> num_workgroups_ubo : num_workgroups_struct;
/// ```
/// The binding group and number used for this uniform buffer is provided via
/// the `Config` transform input.
///
/// @param module the module to transform
/// @returns success or failure
Result<SuccessType> NumWorkgroupsFromUniform(core::ir::Module& module,
                                             const NumWorkgroupsFromUniformConfig& config);

}  // namespace tint::hlsl::writer::raise

#endif  // SRC_TINT_LANG_HLSL_WRITER_RAISE_NUM_WORKGROUPS_FROM_UNIFORM_H_