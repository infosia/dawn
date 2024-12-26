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
class ImmediateConstantsTrackerTest : public DawnNativeTest {
  protected:
    RenderPipelineBase* MakeTestRenderPipeline() {
        utils::ComboRenderPipelineDescriptor desc;
        desc.vertex.module = utils::CreateShaderModule(device, R"(
            @vertex fn main() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 0.0);
            }
        )");
        desc.vertex.entryPoint = "main";
        desc.cFragment.module = utils::CreateShaderModule(device, R"(
            @fragment fn main() -> @location(0) vec4f {
                return vec4f(0.1, 0.2, 0.3, 0.4);
            }
        )");
        desc.cFragment.entryPoint = "main";
        return FromAPI(device.CreateRenderPipeline(&desc).Get());
    }

    ComputePipelineBase* MakeTestComputePipeline() {
        wgpu::ComputePipelineDescriptor desc;
        desc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {}
    )");
        desc.compute.entryPoint = "main";
        return FromAPI(device.CreateComputePipeline(&desc).Get());
    }
};

class RenderImmediateConstantsTrackerTest : public ImmediateConstantsTrackerTest {};

class ComputeImmediateConstantsTrackerTest : public ImmediateConstantsTrackerTest {};

// Test pipeline change reset dirty bits and update tracked pipeline constants mask.
TEST_F(ImmediateConstantsTrackerTest, OnPipelineChange) {
    {
        RenderImmediateConstantsTrackerBase tracker;

        // Control Case
        tracker.SetDirtyBitsForTesting({0x00100101});
        EXPECT_TRUE(tracker.GetDirtyBits() == ImmediateConstantMask(0x00100101));
        EXPECT_TRUE(tracker.GetPipelineImmediateConstantMask() == ImmediateConstantMask(0x0));

        // Pipeline change should reset dirty bits
        RenderPipelineBase* pipeline = MakeTestRenderPipeline();
        pipeline->SetImmediateConstantMask({0x00100101});
        tracker.OnPipelineChange(pipeline);
        EXPECT_TRUE(tracker.GetDirtyBits() == ImmediateConstantMask(0x0));
        EXPECT_TRUE(tracker.GetPipelineImmediateConstantMask() ==
                    ImmediateConstantMask(0x00100101));
    }

    {
        ComputeImmediateConstantsTrackerBase tracker;

        // Control Case
        tracker.SetDirtyBitsForTesting({0x00100101});
        EXPECT_TRUE(tracker.GetDirtyBits() == ImmediateConstantMask(0x00100101));
        EXPECT_TRUE(tracker.GetPipelineImmediateConstantMask() == ImmediateConstantMask(0x0));

        // Pipeline change should reset dirty bits
        ComputePipelineBase* pipeline = MakeTestComputePipeline();
        pipeline->SetImmediateConstantMask({0x00100101});
        tracker.OnPipelineChange(pipeline);
        EXPECT_TRUE(tracker.GetDirtyBits() == ImmediateConstantMask(0x0));
        EXPECT_TRUE(tracker.GetPipelineImmediateConstantMask() ==
                    ImmediateConstantMask(0x00100101));
    }

    device.Destroy();
}

// Test immediate setting update dirty bits and contents correctly.
TEST_F(ImmediateConstantsTrackerTest, SetImmediateData) {
    // RenderImmediateConstantsTracker
    {
        RenderImmediateConstantsTrackerBase tracker;
        int32_t immediateData[] = {2, 4, -6, 8};
        tracker.SetImmediateData(1, reinterpret_cast<uint32_t*>(immediateData), 2, 2);

        ImmediateConstantMask expected;
        expected |= GetRawBits(CommonImmediateConstantsBit::ImmediateDataStart) << 1;
        expected |= GetRawBits(CommonImmediateConstantsBit::ImmediateDataStart) << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        uint32_t immediateDataRangeOffset =
            CommonImmediateConstantsBytesLayout::ImmediateDataStart + sizeof(int32_t);
        EXPECT_TRUE(memcmp(tracker.GetContent().Get<int32_t>(immediateDataRangeOffset),
                           &immediateData[2], sizeof(int32_t) * 2) == 0);
    }

    // ComputeImmediateConstantsTracker
    {
        ComputeImmediateConstantsTrackerBase tracker;
        int32_t immediateData[] = {2, 4, -6, 8};
        tracker.SetImmediateData(1, reinterpret_cast<uint32_t*>(immediateData), 2, 2);

        ImmediateConstantMask expected;
        expected |= GetRawBits(CommonImmediateConstantsBit::ImmediateDataStart) << 1;
        expected |= GetRawBits(CommonImmediateConstantsBit::ImmediateDataStart) << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        uint32_t immediateDataRangeOffset =
            CommonImmediateConstantsBytesLayout::ImmediateDataStart + sizeof(int32_t);
        EXPECT_TRUE(memcmp(tracker.GetContent().Get<int32_t>(immediateDataRangeOffset),
                           &immediateData[2], sizeof(int32_t) * 2) == 0);
    }

    device.Destroy();
}

// Test setting clamp frag depth args with float value updates dirty bits and contents correctly.
TEST_F(RenderImmediateConstantsTrackerTest, SetClampFragDepth) {
    RenderImmediateConstantsTrackerBase tracker;
    float minClampFragDepth = 0.1;
    float maxClampFragDepth = 0.95;
    tracker.SetClampFragDepth(minClampFragDepth, maxClampFragDepth);

    ImmediateConstantMask expected;
    expected |= RenderImmediateConstantsBit::MinClampFragDepth;
    expected |= RenderImmediateConstantsBit::MaxClampFragDepth;
    EXPECT_TRUE(tracker.GetDirtyBits() == expected);

    // Compare bits instead of values here to ensure bits level equality.
    EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                           RenderImmediateConstantsBytesLayout::MinClampFragDepth),
                       reinterpret_cast<uint32_t*>(&minClampFragDepth), sizeof(uint32_t)) == 0);
    EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                           RenderImmediateConstantsBytesLayout::MaxClampFragDepth),
                       reinterpret_cast<uint32_t*>(&maxClampFragDepth), sizeof(float)) == 0);

    device.Destroy();
}

// Test setting first index offset args updates dirty bits and contents correctly.
TEST_F(RenderImmediateConstantsTrackerTest, SetFirstIndexOffset) {
    RenderImmediateConstantsTrackerBase tracker;
    uint32_t firstVertex = 1;
    uint32_t firstInstance = 2;
    tracker.SetFirstIndexOffset(firstVertex, firstInstance);

    ImmediateConstantMask expected;
    expected |= RenderImmediateConstantsBit::FirstVertex;
    expected |= RenderImmediateConstantsBit::FirstInstance;
    EXPECT_TRUE(tracker.GetDirtyBits() == expected);

    EXPECT_TRUE(
        memcmp(tracker.GetContent().Get<uint32_t>(RenderImmediateConstantsBytesLayout::FirstVertex),
               &firstVertex, sizeof(uint32_t)) == 0);
    EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                           RenderImmediateConstantsBytesLayout::FirstInstance),
                       &firstInstance, sizeof(uint32_t)) == 0);

    device.Destroy();
}

// Test setting num workgroups dimensions update dirty bits and contents correctly.
TEST_F(ComputeImmediateConstantsTrackerTest, SetNumWorkgroupDimensions) {
    // SetNumWorkgropusX
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsX = 256;
        tracker.SetNumWorkgroupsX(numWorkgroupsX);

        ImmediateConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsX;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                               ComputeImmediateConstantsBytesLayout::NumWorkgroupsX),
                           &numWorkgroupsX, sizeof(uint32_t)) == 0);
    }

    // SetNumworkgroupsY
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsY = 128;
        tracker.SetNumWorkgroupsY(numWorkgroupsY);

        ImmediateConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsY;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                               ComputeImmediateConstantsBytesLayout::NumWorkgroupsY),
                           &numWorkgroupsY, sizeof(uint32_t)) == 0);
    }

    // SetNumworkgroupsZ
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsZ = 64;
        tracker.SetNumWorkgroupsZ(numWorkgroupsZ);

        ImmediateConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsZ;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        EXPECT_TRUE(memcmp(tracker.GetContent().Get<uint32_t>(
                               ComputeImmediateConstantsBytesLayout::NumWorkgroupsZ),
                           &numWorkgroupsZ, sizeof(uint32_t)) == 0);
    }

    device.Destroy();
}

}  // anonymous namespace
}  // namespace dawn::native
