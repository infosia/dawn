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

#include <cfloat>
#include <string_view>

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
static constexpr std::string_view kComputeShaderDefault = R"(
        @compute @workgroup_size(1) fn main() {}
    )";

class ImmediateConstantsTrackerTest : public DawnNativeTest {
  protected:
    ComputePipelineBase* MakeTestComputePipeline() {
        wgpu::ComputePipelineDescriptor desc;
        desc.compute.module = utils::CreateShaderModule(device, kComputeShaderDefault.data());
        desc.compute.entryPoint = "main";
        return FromAPI(device.CreateComputePipeline(&desc).Get());
    }
};

class RenderImmediateConstantsTrackerTest : public ImmediateConstantsTrackerTest {};

class ComputeImmediateConstantsTrackerTest : public ImmediateConstantsTrackerTest {};

// Test pipeline change reset dirty bits and update tracked pipeline constants mask.
// Use Compute pipeline to cover this common path.
TEST_F(ImmediateConstantsTrackerTest, OnPipelineChange) {
    ComputeImmediateConstantsTrackerBase tracker;

    // Control Case
    tracker.SetDirtyBitsForTesting({0x00100101});
    EXPECT_TRUE(tracker.GetDirtyBits() == ConstantMask(0x00100101));
    EXPECT_TRUE(tracker.GetPipelineConstantsMask() == ConstantMask(0x0));

    // Pipeline change should reset dirty bits
    ComputePipelineBase* pipeline = MakeTestComputePipeline();
    pipeline->SetConstantMask({0x00100101});
    tracker.OnPipelineChange(pipeline);
    EXPECT_TRUE(tracker.GetDirtyBits() == ConstantMask(0x0));
    EXPECT_TRUE(tracker.GetPipelineConstantsMask() == ConstantMask(0x00100101));
    device.Destroy();
}

// Test immediate setting update dirty bits and contents correctly.
TEST_F(ImmediateConstantsTrackerTest, SetImmediateData) {
    // RenderImmediateConstantsTracker
    {
        RenderImmediateConstantsTrackerBase tracker;
        int32_t immediateData[] = {2, 4, -6, 8};
        tracker.SetImmediateData(1, reinterpret_cast<uint32_t*>(immediateData), 2, 2);

        ConstantMask expected;
        expected |= CommonImmediateConstantsBit::ImmediateDataStart << 1;
        expected |= CommonImmediateConstantsBit::ImmediateDataStart << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();
        uint32_t immediateDataRangeOffset =
            CommonImmediateConstantsBytesLayout::ImmediateDataStart + sizeof(int32_t);
        EXPECT_TRUE(
            memcmp(data + immediateDataRangeOffset, &immediateData[2], sizeof(int32_t) * 2) == 0);
    }

    // ComputeImmediateConstantsTracker
    {
        ComputeImmediateConstantsTrackerBase tracker;
        int32_t immediateData[] = {2, 4, -6, 8};
        tracker.SetImmediateData(1, reinterpret_cast<uint32_t*>(immediateData), 2, 2);

        ConstantMask expected;
        expected |= CommonImmediateConstantsBit::ImmediateDataStart << 1;
        expected |= CommonImmediateConstantsBit::ImmediateDataStart << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();
        uint32_t immediateDataRangeOffset =
            CommonImmediateConstantsBytesLayout::ImmediateDataStart + sizeof(int32_t);
        EXPECT_TRUE(
            memcmp(data + immediateDataRangeOffset, &immediateData[2], sizeof(int32_t) * 2) == 0);
    }

    device.Destroy();
}

// Test setting dynamic storage buffer length update dirty bits and contents correctly.
TEST_F(ImmediateConstantsTrackerTest, SetDynamicStorageBufferLength) {
    // RenderImmediateConstantsTracker
    {
        RenderImmediateConstantsTrackerBase tracker;
        uint32_t dynamicStorageBufferLength[] = {1, 3, 5, 7};
        tracker.SetDynamicStorageBufferLength(0, dynamicStorageBufferLength[0]);
        tracker.SetDynamicStorageBufferLength(2, dynamicStorageBufferLength[2]);

        ConstantMask expected;
        expected |= CommonImmediateConstantsBit::DynamicBufferLengthStart;
        expected |= CommonImmediateConstantsBit::DynamicBufferLengthStart << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();
        EXPECT_TRUE(memcmp(data + CommonImmediateConstantsBytesLayout::DynamicBufferLengthStart,
                           &dynamicStorageBufferLength[0], sizeof(uint32_t)) == 0);
        EXPECT_TRUE(memcmp(data + CommonImmediateConstantsBytesLayout::DynamicBufferLengthStart +
                               2 * sizeof(uint32_t),
                           &dynamicStorageBufferLength[2], sizeof(uint32_t)) == 0);
    }

    // ComputeImmediateConstantsTracker
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t dynamicStorageBufferLength[] = {1, 3, 5, 7};
        tracker.SetDynamicStorageBufferLength(0, dynamicStorageBufferLength[0]);
        tracker.SetDynamicStorageBufferLength(2, dynamicStorageBufferLength[2]);

        ConstantMask expected;
        expected |= CommonImmediateConstantsBit::DynamicBufferLengthStart;
        expected |= CommonImmediateConstantsBit::DynamicBufferLengthStart << 2;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();
        EXPECT_TRUE(memcmp(data + CommonImmediateConstantsBytesLayout::DynamicBufferLengthStart,
                           &dynamicStorageBufferLength[0], sizeof(uint32_t)) == 0);
        EXPECT_TRUE(memcmp(data + CommonImmediateConstantsBytesLayout::DynamicBufferLengthStart +
                               2 * sizeof(uint32_t),
                           &dynamicStorageBufferLength[2], sizeof(uint32_t)) == 0);
    }

    device.Destroy();
}

// Test setting clamp frag depth args with float value updates dirty bits and contents correctly.
TEST_F(RenderImmediateConstantsTrackerTest, SetClampFragDepth) {
    RenderImmediateConstantsTrackerBase tracker;
    float minClampFragDepth = 0.1;
    float maxClampFragDepth = 0.95;
    tracker.SetClampFragDepth(minClampFragDepth, maxClampFragDepth);

    ConstantMask expected;
    expected |= RenderImmediateConstantsBit::MinClampFragDepth;
    expected |= RenderImmediateConstantsBit::MaxClampFragDepth;
    EXPECT_TRUE(tracker.GetDirtyBits() == expected);

    const unsigned char* data = tracker.GetContent().data();
    EXPECT_TRUE(memcmp(data + RenderImmediateConstantsBytesLayout::MinClampFragDepth,
                       &minClampFragDepth, sizeof(float)) == 0);
    EXPECT_TRUE(memcmp(data + RenderImmediateConstantsBytesLayout::MaxClampFragDepth,
                       &maxClampFragDepth, sizeof(float)) == 0);

    device.Destroy();
}

// Test setting first index offset args updates dirty bits and contents correctly.
TEST_F(RenderImmediateConstantsTrackerTest, SetFirstIndexOffset) {
    RenderImmediateConstantsTrackerBase tracker;
    uint32_t firstVertex = 1;
    uint32_t firstInstance = 2;
    tracker.SetFirstIndexOffset(firstVertex, firstInstance);

    ConstantMask expected;
    expected |= RenderImmediateConstantsBit::FirstVertex;
    expected |= RenderImmediateConstantsBit::FirstInstance;
    EXPECT_TRUE(tracker.GetDirtyBits() == expected);

    const unsigned char* data = tracker.GetContent().data();
    EXPECT_TRUE(memcmp(data + RenderImmediateConstantsBytesLayout::FirstVertex, &firstVertex,
                       sizeof(uint32_t)) == 0);
    EXPECT_TRUE(memcmp(data + RenderImmediateConstantsBytesLayout::FirstInstance, &firstInstance,
                       sizeof(uint32_t)) == 0);

    device.Destroy();
}

// Test setting num workgroups dimensions update dirty bits and contents correctly.
TEST_F(ComputeImmediateConstantsTrackerTest, SetNumWorkgroupDimensions) {
    // SetNumWorkgropusX
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsX = 256;
        tracker.SetNumWorkgroupsX(numWorkgroupsX);

        ConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsX;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();

        EXPECT_TRUE(memcmp(data + ComputeImmediateConstantsBytesLayout::NumWorkgroupsX,
                           &numWorkgroupsX, sizeof(uint32_t)) == 0);
    }

    // SetNumworkgroupsY
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsY = 128;
        tracker.SetNumWorkgroupsY(numWorkgroupsY);

        ConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsY;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();

        EXPECT_TRUE(memcmp(data + ComputeImmediateConstantsBytesLayout::NumWorkgroupsY,
                           &numWorkgroupsY, sizeof(uint32_t)) == 0);
    }

    // SetNumworkgroupsZ
    {
        ComputeImmediateConstantsTrackerBase tracker;
        uint32_t numWorkgroupsZ = 64;
        tracker.SetNumWorkgroupsZ(numWorkgroupsZ);

        ConstantMask expected;
        expected |= ComputeImmediateConstantsBit::NumWorkgroupsZ;
        EXPECT_TRUE(tracker.GetDirtyBits() == expected);

        const unsigned char* data = tracker.GetContent().data();

        EXPECT_TRUE(memcmp(data + ComputeImmediateConstantsBytesLayout::NumWorkgroupsZ,
                           &numWorkgroupsZ, sizeof(uint32_t)) == 0);
    }

    device.Destroy();
}

}  // anonymous namespace
}  // namespace dawn::native
