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

#ifndef SRC_TINT_LANG_HLSL_WRITER_RAISE_CHANGE_PUSH_CONSTANT_TO_UNIFORM_H_
#define SRC_TINT_LANG_HLSL_WRITER_RAISE_CHANGE_PUSH_CONSTANT_TO_UNIFORM_H_

#include <algorithm>
#include <optional>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}  // namespace tint::core::ir

namespace tint::hlsl::writer::raise {

/// The set of polyfills that should be applied.
struct ChangePushConstantToUniformConfig {
    /// The binding point info.
    std::optional<BindingPoint> push_constant_binding_point;
};

/// ChangePushConstantToUniform is a transform used to replace push_constant global var accesses to
/// uniform address space. Doing this transform to leverage DecomposeUniformAccess to generate
/// correct
// load of the HLSL primitive objects. This transform should apply after all push_constant related
// transforms and beforem DecomposeUniformAccess transform.
///
/// @param module the module to transform
/// @returns success or failure
Result<SuccessType> ChangePushConstantToUniform(core::ir::Module& module,
                                                const ChangePushConstantToUniformConfig& config);

}  // namespace tint::hlsl::writer::raise

#endif  // SRC_TINT_LANG_HLSL_WRITER_RAISE_CHANGE_PUSH_CONSTANT_TO_UNIFORM_H_
