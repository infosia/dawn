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

#include "dawn/common/Assert.h"
#include "dawn/native/ComputePipeline.h"
#include "dawn/native/ImmediateConstantsLayout.h"
#include "dawn/native/ImmediateConstantsTracker.h"
#include "dawn/native/RenderPipeline.h"
#include "dawn/tests/DawnNativeTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn::native {
namespace {
class ImmediateConstantOffsetTest : public DawnNativeTest {
  protected:
    wgpu::PipelineLayout CreatePipelineLayout(uint32_t immediateDataByteSize = 0) {
        wgpu::PipelineLayoutDescriptor layoutDesc;
        layoutDesc.immediateDataRangeByteSize = immediateDataByteSize;
        return device.CreatePipelineLayout(&layoutDesc);
    }

    RenderPipelineBase* MakeTestRenderPipelineWithClampingFragDepth(
        uint32_t immediateDataByteSize = 0) {
        utils::ComboRenderPipelineDescriptor desc;
        desc.layout = CreatePipelineLayout(immediateDataByteSize);
        desc.vertex.module = utils::CreateShaderModule(device, R"(
            @vertex fn main() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 0.0);
            }
        )");
        desc.vertex.entryPoint = "main";
        desc.cFragment.module = utils::CreateShaderModule(device, R"(
            @fragment fn main() -> @builtin(frag_depth) f32 {
                return 1.0;
            }
        )");
        desc.cFragment.entryPoint = "main";
        return FromAPI(device.CreateRenderPipeline(&desc).Get());
    }

    ComputePipelineBase* MakeTestComputePipeline(uint32_t immediateDataByteSize = 0) {
        wgpu::ComputePipelineDescriptor desc;
        desc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {}
    )");
        desc.compute.entryPoint = "main";
        desc.layout = CreatePipelineLayout(immediateDataByteSize);
        return FromAPI(device.CreateComputePipeline(&desc).Get());
    }

    const uint32_t kImmediateDataCount = 2;
    const uint32_t kFirstElementBit = 1u;
    const uint32_t kSecondElementBit = 1u << 1;
};

// Test pipeline change reset dirty bits and update tracked pipeline constants mask.
// Use Compute pipeline to cover this common path.
TEST_P(ImmediateConstantOffsetTest, ClampFragDepth) {
    {
        RenderPipelineBase* pipeline = MakeTestRenderPipeline();
        ImmediateConstantMask immediateConstantMask = pipeline->GetImmediateConstantMask();
        const ImmediateConstantOffsetMap& immediateConstantOffsets =
            pipeline->GetInternalImmediateConstantOffsetMap();

        ImmediateConstantMask expectedImmediateConstantMask;
        ImmediateConstantOffsetMap expectedImmediateConstantOffsets;
        uint32_t offset = 0;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MinClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MinClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MaxClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MaxClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        EXPECT_TRUE(pipeline->GetImmediateConstantMask() == expectedImmediateConstantMask);
        EXPECT_TRUE(immediateConstantOffsets.size() == expectedImmediateConstantOffsets.size());
        EXPECT_TRUE(std::equal(immediateConstantOffsets.begin(), immediateConstantOffsets.end(),
                               expectedImmediateConstantOffsets.begin()));
    }

    {
        RenderPipelineBase* pipeline =
            MakeTestRenderPipeline(kImmediateDataCount * kImmediateConstantElementByteSize);
        ImmediateConstantMask immediateConstantMask = pipeline->GetImmediateConstantMask();
        const ImmediateConstantOffsetMap& immediateConstantOffsets =
            pipeline->GetInternalImmediateConstantOffsetMap();

        ImmediateConstantMask expectedImmediateConstantMask;
        ImmediateConstantOffsetMap expectedImmediateConstantOffsets;
        uint32_t offset = 0;

        // 2 Immediate Data

        expectedImmediateConstantMask |= kFirstElementBit;
        expcetedImmediateConstantOffsets.insert({kFirstElementBit, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= kSecondElementBit;
        expcetedImmediateConstantOffsets.insert({kSecondElementBit, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MinClampFragDepth;
        expcetedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MinClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MaxClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MaxClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        EXPECT_TRUE(pipeline->GetImmediateConstantMask() == expectedImmediateConstantMask);
        EXPECT_TRUE(immediateConstantOffsets.size() == expectedImmediateConstantOffsets.size());
        EXPECT_TRUE(std::equal(immediateConstantOffsets.begin(), immediateConstantOffsets.end(),
                               expectedImmediateConstantOffsets.begin()));
    }
}

TEST_P(ImmediateConstantOffsetTest, ComputePipelineImmediateData) {
    {
        ComputePipelineBase* pipeline = MakeTestComputePipeline();
        ImmediateConstantMask immediateConstantMask = pipeline->GetImmediateConstantMask();
        const ImmediateConstantOffsetMap& immediateConstantOffsets =
            pipeline->GetInternalImmediateConstantOffsetMap();

        ImmediateConstantMask expectedImmediateConstantMask;
        ImmediateConstantOffsetMap expectedImmediateConstantOffsets;
        uint32_t offset = 0;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MinClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MinClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MaxClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MaxClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        EXPECT_TRUE(pipeline->GetImmediateConstantMask() == expectedImmediateConstantMask);
        EXPECT_TRUE(immediateConstantOffsets.size() == expectedImmediateConstantOffsets.size());
        EXPECT_TRUE(std::equal(immediateConstantOffsets.begin(), immediateConstantOffsets.end(),
                               expectedImmediateConstantOffsets.begin()));
    }

    {
        ComputePipelineBase* pipeline =
            MakeTestComputePipeline(kImmediateDataCount * kImmediateConstantElementByteSize);
        ImmediateConstantMask immediateConstantMask = pipeline->GetImmediateConstantMask();
        const ImmediateConstantOffsetMap& immediateConstantOffsets =
            pipeline->GetInternalImmediateConstantOffsetMap();

        ImmediateConstantMask expectedImmediateConstantMask;
        ImmediateConstantOffsetMap expectedImmediateConstantOffsets;
        uint32_t offset = 0;

        // 2 Immediate Data

        expectedImmediateConstantMask |= kFirstElementBit;
        expcetedImmediateConstantOffsets.insert({kFirstElementBit, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= kSecondElementBit;
        expcetedImmediateConstantOffsets.insert({kSecondElementBit, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MinClampFragDepth;
        expcetedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MinClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        expectedImmediateConstantMask |= RenderImmediateConstantsBit::MaxClampFragDepth;
        expectedImmediateConstantOffsets.insert(
            {RenderImmediateConstantsBit::MaxClampFragDepth, offset});
        offset += kImmediateConstantElementByteSize;

        EXPECT_TRUE(pipeline->GetImmediateConstantMask() == expectedImmediateConstantMask);
        EXPECT_TRUE(immediateConstantOffsets.size() == expectedImmediateConstantOffsets.size());
        EXPECT_TRUE(std::equal(immediateConstantOffsets.begin(), immediateConstantOffsets.end(),
                               expectedImmediateConstantOffsets.begin()));
    }
}
DAWN_INSTANTIATE_TEST_P(ImmediateConstantOffsetTest, {VulkanBackend()});

}  // anonymous namespace
}  // namespace dawn::native
