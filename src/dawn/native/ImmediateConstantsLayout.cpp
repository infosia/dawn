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

#include "dawn/native/ImmediateConstantsLayout.h"

#include <utility>

namespace dawn::native {
uint32_t ImmediateConstant::GetRawStartBit() const {
    return 1u << static_cast<uint32_t>(startIndex);
}

uint32_t ImmediateConstant::GetRawStartIndex() const {
    return static_cast<uint32_t>(startIndex);
}

ImmediateConstantMask ImmediateConstant::GetConstantBlockBits() const {
    return ImmediateConstantMask(constantBlockBits);
}

ImmediateConstantsTable BuildImmediateConstantsTable() {
    ImmediateConstantsTable table;

    auto AddImmediateConstant = [&table](ImmediateConstantType type, uint32_t startIndex,
                                         uint32_t constantBlockBits, uint32_t constantBlockSize,
                                         uint32_t constantBlockElementsCount) {
        ImmediateConstant constant;
        constant.type = type;
        constant.startIndex = static_cast<ImmediateConstantIndex>(startIndex);
        constant.constantBlockBits |= constantBlockBits;
        constant.constantBlockSize = constantBlockSize;
        constant.constantBlockElementsCount = constantBlockElementsCount;

        table.insert({type, std::move(constant)});
    };

    // ImmediateConstants: ImmediateData
    static constexpr uint32_t ImmediateDataStartIndex =
        offsetof(CommonImmediateConstants, immediateData) / kImmediateConstantElementByteSize;
    uint32_t ImmediateDataBits = 0u;
    // Immediate data always start from offset 0.
    for (uint32_t i = 0u; i < kMaxImmediateData32BitCountPerPipeline; i++) {
        ImmediateDataBits |= 1u << i;
    }

    // ImmediateConstants: ClampFragDepth
    static constexpr uint32_t MinClampFragDepthOffset =
        offsetof(RenderImmediateConstants, minClampFragDepth);
    static constexpr uint32_t MaxClampFragDepthOffset =
        offsetof(RenderImmediateConstants, maxClampFragDepth);
    static constexpr uint32_t ClampFragDepthStartIndex =
        MinClampFragDepthOffset / kImmediateConstantElementByteSize;
    static constexpr uint32_t MinClampFragDepthBit =
        1u << (MinClampFragDepthOffset / kImmediateConstantElementByteSize);
    static constexpr uint32_t MaxClampFragDepthBit =
        1u << (MaxClampFragDepthOffset / kImmediateConstantElementByteSize);
    static constexpr uint32_t ClampFragDepthBits = MinClampFragDepthBit | MaxClampFragDepthBit;
    static constexpr uint32_t ClampFragDepthElementCounts = 2u;

    // ImmediateConstants: FirstVertex
    static constexpr uint32_t FirstVertexStartIndex =
        offsetof(RenderImmediateConstants, firstVertex) / kImmediateConstantElementByteSize;

    static constexpr uint32_t FirstVertexBits = 1u << FirstVertexStartIndex;
    static constexpr uint32_t FirstVertexElementCounts = 1u;

    // ImmediateConstants: FirstInstance
    static constexpr uint32_t FirstInstanceStartIndex =
        offsetof(RenderImmediateConstants, firstInstance) / kImmediateConstantElementByteSize;
    static const uint32_t FirstInstanceBits = 1u << FirstInstanceStartIndex;
    static constexpr uint32_t FirstInstanceElementCounts = 1u;

    // ImmediateConstants: Numworkgroups
    static constexpr uint32_t NumWorkgroupsXOffset =
        offsetof(ComputeImmediateConstants, numWorkgroupsX);
    static constexpr uint32_t NumWorkgroupsYOffset =
        offsetof(ComputeImmediateConstants, numWorkgroupsY);
    static constexpr uint32_t NumWorkgroupsZOffset =
        offsetof(ComputeImmediateConstants, numWorkgroupsZ);
    static constexpr uint32_t NumWorkgroupsStartIndex =
        NumWorkgroupsXOffset / kImmediateConstantElementByteSize;
    static const uint32_t NumWorkgroupsXBit =
        1u << (NumWorkgroupsXOffset / kImmediateConstantElementByteSize);
    static const uint32_t NumWorkgroupsYBit =
        1u << (NumWorkgroupsYOffset / kImmediateConstantElementByteSize);
    static const uint32_t NumWorkgroupsZBit =
        1u << (NumWorkgroupsZOffset / kImmediateConstantElementByteSize);
    static const uint32_t NumWorkgroupsBits =
        NumWorkgroupsXBit | NumWorkgroupsYBit | NumWorkgroupsZBit;
    static constexpr uint32_t NumWorkgroupsElementCounts = 3u;

    AddImmediateConstant(ImmediateConstantType::ImmediateData, ImmediateDataStartIndex,
                         ImmediateDataBits,
                         kMaxImmediateData32BitCountPerPipeline * kImmediateConstantElementByteSize,
                         kMaxImmediateData32BitCountPerPipeline);

    AddImmediateConstant(ImmediateConstantType::ClampFragDepth, ClampFragDepthStartIndex,
                         ClampFragDepthBits,
                         ClampFragDepthElementCounts * kImmediateConstantElementByteSize,
                         ClampFragDepthElementCounts);

    AddImmediateConstant(ImmediateConstantType::FirstVertex, FirstVertexStartIndex, FirstVertexBits,
                         FirstVertexElementCounts * kImmediateConstantElementByteSize,
                         FirstVertexElementCounts);

    AddImmediateConstant(
        ImmediateConstantType::FirstInstance, FirstInstanceStartIndex, FirstInstanceBits,
        FirstInstanceElementCounts * kImmediateConstantElementByteSize, FirstInstanceElementCounts);

    AddImmediateConstant(
        ImmediateConstantType::NumWorkgroups, NumWorkgroupsStartIndex, NumWorkgroupsBits,
        NumWorkgroupsElementCounts * kImmediateConstantElementByteSize, NumWorkgroupsElementCounts);
    return table;
}

}  // namespace dawn::native
