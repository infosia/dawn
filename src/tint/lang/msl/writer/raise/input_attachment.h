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

// tint-tbdr: begin input_attachment-msl
//
// This file is part of the tile-based deferred rendering (TBDR) support for the MSL backend. It is
// a fork-only file: it has no upstream counterpart, which keeps the divergence from upstream Dawn
// concentrated here rather than spread across shared files. See TILED.md.

#ifndef SRC_TINT_LANG_MSL_WRITER_RAISE_INPUT_ATTACHMENT_H_
#define SRC_TINT_LANG_MSL_WRITER_RAISE_INPUT_ATTACHMENT_H_

#include <cstdint>
#include <unordered_map>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}

namespace tint::msl::writer::raise {

/// Configuration for the InputAttachment transform.
struct InputAttachmentConfig {
    /// Map from the WGSL binding point of an `input_attachment` variable to the Metal `[[color(N)]]`
    /// slot it is lowered to.
    const std::unordered_map<BindingPoint, uint32_t>& binding_to_color;
};

/// InputAttachment lowers `input_attachment` variables and `inputAttachmentLoad()` builtins, which
/// have no native Metal representation, into `[[color(N)]]` fragment inputs. Each input attachment
/// is replaced by a fragment entry-point parameter carrying the already-resolved `vec4<T>` value,
/// reusing the same `@color` representation that the framebuffer-fetch extension already uses.
///
/// This transform must run before ShaderIO (which turns the `@color` parameter into a
/// `[[color(N)]]` input) and before ModuleScopeVars (which threads the introduced private variable
/// to any helper functions that read the attachment).
///
/// @param module the module to transform
/// @param config the configuration
/// @returns success or failure
Result<SuccessType> InputAttachment(core::ir::Module& module, const InputAttachmentConfig& config);

}  // namespace tint::msl::writer::raise

#endif  // SRC_TINT_LANG_MSL_WRITER_RAISE_INPUT_ATTACHMENT_H_

// tint-tbdr: end input_attachment-msl
