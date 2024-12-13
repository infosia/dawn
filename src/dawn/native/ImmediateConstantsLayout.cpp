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

#include "dawn/native/ImmediateConstantsLayout.h"

#include <utility>

namespace dawn::native {

namespace {

struct UserImmediateConstantsBytesLayout {
    static constexpr uint32_t UserImmediateDataStart =
        offsetof(UserImmediateConstants, userImmediateData);
};

struct RenderImmediateConstantsBytesLayout : public UserImmediateConstantsBytesLayout {
    static constexpr uint32_t MinClampFragDepth =
        offsetof(RenderImmediateConstants, minClampFragDepth);
    static constexpr uint32_t MaxClampFragDepth =
        offsetof(RenderImmediateConstants, maxClampFragDepth);
    static constexpr uint32_t FirstVertex = offsetof(RenderImmediateConstants, firstVertex);
    static constexpr uint32_t FirstInstance = offsetof(RenderImmediateConstants, firstInstance);
};

struct ComputeImmediateConstantsBytesLayout : public UserImmediateConstantsBytesLayout {
    static constexpr uint32_t NumWorkgroupsX = offsetof(ComputeImmediateConstants, numWorkgroupsX);
    static constexpr uint32_t NumWorkgroupsY = offsetof(ComputeImmediateConstants, numWorkgroupsY);
    static constexpr uint32_t NumWorkgroupsZ = offsetof(ComputeImmediateConstants, numWorkgroupsZ);
};

struct UserImmediateConstantsBit {
    static constexpr uint32_t UserImmediateDataStart =
        1u << (UserImmediateConstantsBytesLayout::UserImmediateDataStart / sizeof(uint32_t));
};

struct RenderImmediateConstantsBit : public UserImmediateConstantsBit {
    static constexpr uint32_t MinClampFragDepth =
        1u << (RenderImmediateConstantsBytesLayout::MinClampFragDepth / sizeof(uint32_t));
    static constexpr uint32_t MaxClampFragDepth =
        1u << (RenderImmediateConstantsBytesLayout::MaxClampFragDepth / sizeof(uint32_t));
    static constexpr uint32_t FirstVertex =
        1u << (RenderImmediateConstantsBytesLayout::FirstVertex / sizeof(uint32_t));
    static constexpr uint32_t FirstInstance =
        1u << (RenderImmediateConstantsBytesLayout::FirstInstance / sizeof(uint32_t));
};

struct ComputeImmediateConstantsBit : public UserImmediateConstantsBit {
    static constexpr uint32_t NumWorkgroupsX =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsX / sizeof(uint32_t));
    static constexpr uint32_t NumWorkgroupsY =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsY / sizeof(uint32_t));
    static constexpr uint32_t NumWorkgroupsZ =
        1u << (ComputeImmediateConstantsBytesLayout::NumWorkgroupsZ / sizeof(uint32_t));
};

static constexpr uint32_t kClampFragDepthElementCount = 2u;
static constexpr uint32_t kFirstVetexElementCount = 1u;
static constexpr uint32_t kFirstInstanceElementCount = 1u;
static constexpr uint32_t kNumWorkgroupElementCount = 3u;

using ImmediateConstantsTable = std::unordered_map<ImmediateConstantType, ImmediateConstant>;
ImmediateConstantsTable BuildImmediateConstantsTable() {
    ImmediateConstantsTable table;

    auto AddImmediateConstant = [&table](ImmediateConstantType type, uint32_t startByteOffset,
                                         uint32_t constantBlockBits,
                                         uint32_t constantBlockElementsCount) {
        ImmediateConstant constant;
        constant.type = type;
        constant.startByteOffset = startByteOffset;
        constant.constantBlockBits |= constantBlockBits;
        constant.constantBlockElementsCount = constantBlockElementsCount;

        table.insert({type, std::move(constant)});
    };

    // CommonImmediateConstants: ImmediateData
    uint32_t userImmediateDataBits = UserImmediateConstantsBit::UserImmediateDataStart;
    // Immediate data always start from offset 0.
    for (uint32_t i = 0u; i < kMaxExternalImmediateConstantsPerPipeline; i++) {
        userImmediateDataBits |= 1u << i;
    }
    AddImmediateConstant(ImmediateConstantType::UserImmediateData,
                         UserImmediateConstantsBytesLayout::UserImmediateDataStart,
                         userImmediateDataBits, kMaxExternalImmediateConstantsPerPipeline);

    // RenderImmediateConstants: ClampFragDepth, has MinClampFragDepth and MaxClampFragDepth
    AddImmediateConstant(ImmediateConstantType::ClampFragDepth,
                         RenderImmediateConstantsBytesLayout::MinClampFragDepth,
                         RenderImmediateConstantsBit::MinClampFragDepth |
                             RenderImmediateConstantsBit::MaxClampFragDepth,
                         kClampFragDepthElementCount);

    // RenderImmediateConstants: FirstVertex
    AddImmediateConstant(ImmediateConstantType::FirstVertex,
                         RenderImmediateConstantsBytesLayout::FirstVertex,
                         RenderImmediateConstantsBit::FirstVertex, kFirstVetexElementCount);

    // RenderImmediateConstants: FirstInstance
    AddImmediateConstant(ImmediateConstantType::FirstInstance,
                         RenderImmediateConstantsBytesLayout::FirstInstance,
                         RenderImmediateConstantsBit::FirstInstance, kFirstInstanceElementCount);

    // ComputeImmediateConstants: Numworkgroups, has NumWorkgroupsX, NumWorkgroupsY, NumWorkgroupsZ
    AddImmediateConstant(ImmediateConstantType::NumWorkgroups,
                         ComputeImmediateConstantsBytesLayout::NumWorkgroupsX,
                         ComputeImmediateConstantsBit::NumWorkgroupsX |
                             ComputeImmediateConstantsBit::NumWorkgroupsY |
                             ComputeImmediateConstantsBit::NumWorkgroupsZ,
                         kNumWorkgroupElementCount);
    return table;
}

}  // anonymous namespace

const ImmediateConstant& GetImmediateConstant(ImmediateConstantType type) {
    // Only initialize once.
    static ImmediateConstantsTable kImmediateConstantsTable = BuildImmediateConstantsTable();
    return kImmediateConstantsTable[type];
}

}  // namespace dawn::native
