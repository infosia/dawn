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

#include "src/tint/api/common/binding_point.h"
#include "src/tint/utils/reflection.h"

namespace tint {

// The binding type information.
enum class RuntimeBindingType {
    kSampledTexture = 0,
};

/// Binding information for a given binding array
struct RuntimeBindingArrayInfo {
    // The binding point for the supporting storage buffer
    BindingPoint storage_buffer_binding;
    // The count of dynamic bindings.
    uint32_t dynamic_binding_count;
    // The count of default bindings
    uint32_t default_binding_count;

    // Map from the `RuntimeBindingType` to the index in the `bindings` of the storage buffer to
    // access the default binding.
    std::unordered_map<RuntimeBindingType, uint32_t> binding_type_to_default_index;

    TINT_REFLECT(RuntimeBindingArrayInfo,
                 storage_buffer_binding,
                 dynamic_binding_count,
                 default_binding_count,
                 binding_type_to_default_index);
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
//   bindings: array<u32, dynamic_binding_count + default_binding_count>,
// }
// ```
//
// The values used in the `bindings` are from the `RuntimeBindingType` enum.
//
struct RuntimeBindingArrayConfig {
    /// Map from a `binding_array` binding point to the supporting storage buffer binding point.
    std::unordered_map<BindingPoint, RuntimeBindingArrayInfo> bindings;

    TINT_REFLECT(RuntimeBindingArrayConfig, bindings);
};

TINT_REFLECT_ENUM_RANGE(tint::RuntimeBindingType, kSampledTexture, kSampledTexture);

}  // namespace tint

#endif  // SRC_TINT_API_COMMON_RUNTIME_BINDING_ARRAY_CONFIG_H_
