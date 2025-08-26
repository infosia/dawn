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

#ifndef SRC_TINT_API_COMMON_RUNTIME_BINDING_ARRAY_CONFIG_H_
#define SRC_TINT_API_COMMON_RUNTIME_BINDING_ARRAY_CONFIG_H_

#include <unordered_map>
#include <vector>

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/reflection.h"

namespace tint {

// The type of texture in the binding slot
enum class RuntimeBindingType {
    kTextureNull = 0,
    kTexture1d_f32,
    kTexture1d_i32,
    kTexture1d_u32,
    kTexture2d_f32,
    kTexture2d_i32,
    kTexture2d_u32,
    kTexture2dArray_f32,
    kTexture2dArray_i32,
    kTexture2dArray_u32,
    kTexture3d_f32,
    kTexture3d_i32,
    kTexture3d_u32,
    kTextureCube_f32,
    kTextureCube_i32,
    kTextureCube_u32,
    kTextureCubeArray_f32,
    kTextureCubeArray_i32,
    kTextureCubeArray_u32,

    kTextureMultisampled2d_f32,
    kTextureMultisampled2d_i32,
    kTextureMultisampled2d_u32,
    kTextureDepthMultisampled2d_f32,
    kTextureDepthMultisampled2d_i32,
    kTextureDepthMultisampled2d_u32,

    kTextureDepth2d,
    kTextureDepth2dArray,
    kTextureDepthCube,
    kTextureDepthCubeArray,
};

/// Binding information for a binding array
struct RuntimeBindingArrayInfo {
    // The binding point for the supporting storage buffer. This is a post-remapping binding point.
    BindingPoint storage_buffer_binding;

    // The ordering of default bindings which are placed after the user bindings. These will be used
    // as the index to lookup the given `RuntimeBindingType` if the user accesses with in incorrect
    // texture type (or out of bounds).
    //
    // Note, a `RuntimeBindingType::kTextureNull` is required to be the first entry in the default
    // binding type order array.
    std::vector<RuntimeBindingType> default_binding_type_order;

    TINT_REFLECT(RuntimeBindingArrayInfo, storage_buffer_binding, default_binding_type_order);
};

// Configuration for the runtime binding array transform.
//
// The runtime binding array transform assumes that for each runtime binding_array there will be an
// entry in the `bindings` hash map. That binding will provide information on the storage buffer
// attached for the runtime binding_array. Specifically, the storage buffer should have a format of:
//
// ```
// struct SB {
//   array_length: u32,
//   bindings: array<u32>,  // Has `array_length` entries (doesn't include the default bindings).
// }
// ```
//
// The values used in the `bindings` are from the `u32` conversion of the `RuntimeBindingType` enum
// entries.
//
struct RuntimeBindingArrayConfig {
    /// Map from a `binding_array` binding point to the supporting storage buffer binding point.
    std::unordered_map<BindingPoint, RuntimeBindingArrayInfo> bindings;

    TINT_REFLECT(RuntimeBindingArrayConfig, bindings);
};

TINT_REFLECT_ENUM_RANGE(tint::RuntimeBindingType, kTextureNull, kTextureDepthCubeArray);

}  // namespace tint

#endif  // SRC_TINT_API_COMMON_RUNTIME_BINDING_ARRAY_CONFIG_H_
