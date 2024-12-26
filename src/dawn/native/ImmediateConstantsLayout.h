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

#ifndef SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_
#define SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_

#include <unordered_map>

#include "dawn/common/ityp_bitset.h"
#include "dawn/native/EnumClassBitmasks.h"
#include "dawn/native/IntegerTypes.h"

namespace dawn::native {

static constexpr uint32_t kKnownImmediateConstantTypeCount = 5;
// Define common immediate data layout. Append members to expand layouts.
// NOTE: 'offsetof' doesn't support non-standard-layout structs. So use
// aggregate instead of inheritance for RenderImmediateConstants and
// ComputeImmediateConstants.
struct UserImmediateConstants {
    uint32_t userImmediateData[kMaxExternalImmediateConstantsPerPipeline];
};

// Define render pipeline immediate data layout. Append members to
// expand the layout.
struct RenderImmediateConstants {
    UserImmediateConstants userConstants;

    // clamp frag depth args
    float minClampFragDepth;
    float maxClampFragDepth;

    // first index offset
    uint32_t firstVertex;
    uint32_t firstInstance;
};

// Define compute pipeline immediate data layout. Append members to
// expand the layout.
struct ComputeImmediateConstants {
    UserImmediateConstants userConstants;
    // numWorkgroups dimensions
    uint32_t numWorkgroupsX;
    uint32_t numWorkgroupsY;
    uint32_t numWorkgroupsZ;
};

enum class ImmediateConstantType : uint32_t {
    Undefined = 0x0,
    UserImmediateData = 0x1,
    ClampFragDepth = 0x2,
    FirstVertex = 0x4,
    FirstInstance = 0x8,
    NumWorkgroups = 0x10
};

struct ImmediateConstant {
    ImmediateConstantType type = ImmediateConstantType::Undefined;
    uint32_t constantBlockElementsCount = 0u;
    uint32_t startByteOffset = 0u;
    ImmediateConstantMask constantBlockBits = ImmediateConstantMask(0u);
};

// Returns the ImmediateConstant corresponding to the ImmediateConstantType.
const ImmediateConstant& GetImmediateConstant(ImmediateConstantType type);

}  // namespace dawn::native

// Enable dawn enum bitmask for Immediate constant types.
template <>
struct wgpu::IsWGPUBitmask<dawn::native::ImmediateConstantType> {
    static constexpr bool enable = true;
};

#endif  // SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_
