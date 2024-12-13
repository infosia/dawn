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

#ifndef SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_
#define SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_

namespace dawn::native {
class PipelineBase;

// Define common immediate data layout. Append members to expand layouts.
struct CommonImmediateConstants {
    uint32_t immediateData[kMaxImmediateData32BitCountPerPipeline];

    // Calculating the length from the total size of the storage buffer and store
    // in uniform buffer for arrayLength() implementation in WGSL.
    uint32_t dynamicStorageBufferLength[kMaxDynamicStorageBuffersPerPipelineLayout];
};

// Define render pipeline immediate data layout. Append members to
// expand the layout.
struct RenderImmediateConstants : public CommonImmediateConstants {
    // clamp frag depth args
    float minClampFragDepth;
    float maxClampFragDepth;

    // first index offset
    uint32_t firstVertex;
    uint32_t firstInstance;
};

// Define compute pipeline immediate data layout. Append members to
// expand the layout.
struct ComputeImmediateConstants : public CommonImmediateConstants {
    // numWorkgroups dimensions
    uint32_t numWorkgroupsX;
    uint32_t numWorkgroupsY;
    uint32_t numWorkgroupsZ;
};

struct CommonImmediateConstantsBytesLayout {
    static constexpr uint32_t ImmediateDataStart =
        offsetof(RenderImmediateConstants, immediateData);
    static constexpr uint32_t DynamicBufferLengthStart =
        offsetof(RenderImmediateConstants, dynamicStorageBufferLength);
};

struct RenderImmediateConstantsBytesLayout : public CommonImmediateConstantsBytesLayout {
    static constexpr uint32_t MinClampFragDepth =
        offsetof(RenderImmediateConstants, minClampFragDepth);
    static constexpr uint32_t MaxClampFragDepth =
        offsetof(RenderImmediateConstants, maxClampFragDepth);
    static constexpr uint32_t FirstVertex = offsetof(RenderImmediateConstants, firstVertex);
    static constexpr uint32_t FirstInstance = offsetof(RenderImmediateConstants, firstInstance);
};

struct ComputeImmediateConstantsBytesLayout : public CommonImmediateConstantsBytesLayout {
    static constexpr uint32_t NumWorkgroupsX = offsetof(ComputeImmediateConstants, numWorkgroupsX);
    static constexpr uint32_t NumWorkgroupsY = offsetof(ComputeImmediateConstants, numWorkgroupsY);
    static constexpr uint32_t NumWorkgroupsZ = offsetof(ComputeImmediateConstants, numWorkgroupsZ);
};

struct CommonImmediateConstantsBit {
    static constexpr uint32_t ImmediateDataStart =
        1u << (CommonImmediateConstantsBytesLayout::ImmediateDataStart / sizeof(uint32_t));
    static constexpr uint32_t DynamicBufferLengthStart =
        1u << (CommonImmediateConstantsBytesLayout::DynamicBufferLengthStart / sizeof(uint32_t));
};

struct RenderImmediateConstantsBit : public CommonImmediateConstantsBit {
    static constexpr uint32_t MinClampFragDepth =
        1u << (RenderImmediateConstantsBytesLayout::MinClampFragDepth / sizeof(uint32_t));
    static constexpr uint32_t MaxClampFragDepth =
        1u << (RenderImmediateConstantsBytesLayout::MaxClampFragDepth / sizeof(uint32_t));
    static constexpr uint32_t FirstVertex =
        1u << (RenderImmediateConstantsBytesLayout::FirstVertex / sizeof(uint32_t));
    static constexpr uint32_t FirstInstance =
        1u << (RenderImmediateConstantsBytesLayout::FirstInstance / sizeof(uint32_t));
};

struct ComputeImmediateConstantsBit : public CommonImmediateConstantsBit {
    static constexpr uint32_t NumWorkgroupsX =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsX / sizeof(uint32_t));
    static constexpr uint32_t NumWorkgroupsY =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsY / sizeof(uint32_t));
    static constexpr uint32_t NumWorkgroupsZ =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsZ / sizeof(uint32_t));
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_IMMEDIATECONSTANTSLAYOUT_H_
