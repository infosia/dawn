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

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "dawn/common/NonMovable.h"
#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class PixelLocalStorageDisabledTest : public ValidationTest {};
class ImmediateDataDisableTest : public ValidationTest {};

// Check that creating a PipelineLayout with non-zero immediateDataRangeByteSize is disallowed
// without the feature enabled.
TEST_F(ImmediateDataDisableTest, ImmediateDataRangeByteSizeNotAllowed) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;
    desc.immediateDataRangeByteSize = 1;

    ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
}

class ImmediateDataTest : public ValidationTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        // Test only the non-coherent version, ad assume that the same validaiton code paths are
        // taken for the coherent path
        return {wgpu::FeatureName::ChromiumExperimentalImmediateData};
    }

    uint32_t GetMaxImmediateDataRangeByteSize() {
        if (maxImmediateDataByteSize != std::numeric_limits<uint32_t>::max()) {
            return maxImmediateDataByteSize;
        }
        wgpu::SupportedLimits supportedLimits = {};
        wgpu::DawnExperimentalImmediateDataLimits immediateDataLimits = {};
        supportedLimits.nextInChain = &immediateDataLimits;
        device.GetLimits(&supportedLimits);
        for (auto* chain = supportedLimits.nextInChain; chain; chain = chain->nextInChain) {
            switch (chain->sType) {
                case (wgpu::SType::DawnExperimentalImmediateDataLimits): {
                    auto* t = static_cast<wgpu::DawnExperimentalImmediateDataLimits*>(
                        supportedLimits.nextInChain);
                    maxImmediateDataByteSize = t->maxImmediateDataRangeByteSize;
                    break;
                }
                default:
                    DAWN_UNREACHABLE();
            }
        }
        return maxImmediateDataByteSize;
    }

    uint32_t maxImmediateDataByteSize = std::numeric_limits<uint32_t>::max();
};

// Check that non-zero immediateDataRangeByteSize is possible with feature enabled and size must
// below max size limits.
TEST_F(ImmediateDataTest, ValidateImmediateDataRangeByteSize) {
    DAWN_SKIP_TEST_IF(!device.HasFeature(wgpu::FeatureName::ChromiumExperimentalImmediateData));

    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;

    uint32_t maxImmediateDataRangeByteSize = GetMaxImmediateDataRangeByteSize();
    // Success case with valid immediateDataRangeByteSize.
    {
        desc.immediateDataRangeByteSize = maxImmediateDataRangeByteSize;
        device.CreatePipelineLayout(&desc);
    }

    // Failed case with invalid immediateDataRangeByteSize that exceed limits.
    {
        desc.immediateDataRangeByteSize = maxImmediateDataRangeByteSize + 1;
        ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
    }
}

class SetImmediateDataTest : public ImmediateDataTest {
  protected:
    void SetUp() override {
        ImmediateDataTest::SetUp();

        vsModule = utils::CreateShaderModule(device, R"(
            @vertex fn main() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 1.0);
            })");

        fsModule = utils::CreateShaderModule(device, R"(
            @fragment fn main() -> @location(0) vec4f {
                return vec4f(0.0, 1.0, 0.0, 1.0);
            })");

        csModule = utils::CreateShaderModule(device, R"(
         @compute @workgroup_size(1, 1, 1)
        fn main() {
        })");
    }

    wgpu::PipelineLayout CreatePipelineLayout() {
        wgpu::PipelineLayoutDescriptor desc;
        desc.bindGroupLayoutCount = 0;
        desc.immediateDataRangeByteSize = GetMaxImmediateDataRangeByteSize();
        return device.CreatePipelineLayout(&desc);
    }

    wgpu::RenderPipeline CreateRenderPipeline() {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = vsModule;
        pipelineDescriptor.cFragment.module = fsModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        pipelineDescriptor.layout = CreatePipelineLayout();

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::ComputePipeline CreateComputePipeline() {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = csModule;
        csDesc.layout = CreatePipelineLayout();

        return device.CreateComputePipeline(&csDesc);
    }

    wgpu::ShaderModule vsModule;
    wgpu::ShaderModule fsModule;
    wgpu::ShaderModule csModule;
};

// Check that SetImmediateData must be called after pipeline been set.
TEST_F(SetImmediateDataTest, ValidateSetImmediateDataWithoutPipelineNotAllowed) {
    DAWN_SKIP_TEST_IF(!device.HasFeature(wgpu::FeatureName::ChromiumExperimentalImmediateData));

    // Success case with render pipeline set.
    {
        wgpu::RenderPipeline pipeline = CreateRenderPipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetPipeline(pipeline);
        renderPass.SetImmediateData(0, 0, nullptr);
        renderPass.End();
        encoder.Finish();
    }

    // Success case with compute pipeline set.
    {
        wgpu::ComputePipeline pipeline = CreateComputePipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(pipeline);
        computePass.SetImmediateData(0, 0, nullptr);
        computePass.End();
        encoder.Finish();
    }

    // Failed case without render pipeline set
    {
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetImmediateData(0, 0, nullptr);
        renderPass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    // Failed case without compute pipeline set
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetImmediateData(0, 0, nullptr);
        computePass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Check that SetImmediateData data array must be valid when immediate count is not zero.
TEST_F(SetImmediateDataTest, ValidateSetImmediateDataSource) {
    DAWN_SKIP_TEST_IF(!device.HasFeature(wgpu::FeatureName::ChromiumExperimentalImmediateData));

    // Success cases with non-zero immediate data count and valid data array
    {
        wgpu::RenderPipeline pipeline = CreateRenderPipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t data[] = {0};
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetPipeline(pipeline);
        renderPass.SetImmediateData(0, 1, data);
        renderPass.End();
        encoder.Finish();
    }

    // Success case with zero immediate data count and nullptr data array
    {
        wgpu::ComputePipeline pipeline = CreateComputePipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t data[] = {0};
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(pipeline);
        computePass.SetImmediateData(0, 1, data);
        computePass.End();
        encoder.Finish();
    }

    // Failed cases with non-zero immediate data count but nullptr data array.
    {
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetImmediateData(0, 1, nullptr);
        renderPass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetImmediateData(0, 1, nullptr);
        computePass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

// Check that SetImmediateData offset + length must be in bound.
TEST_F(SetImmediateDataTest, ValidateSetImmediateDataOOB) {
    DAWN_SKIP_TEST_IF(!device.HasFeature(wgpu::FeatureName::ChromiumExperimentalImmediateData));

    uint32_t maxImmediateDataRangeByteSize = GetMaxImmediateDataRangeByteSize();
    DAWN_SKIP_TEST_IF(maxImmediateDataRangeByteSize < sizeof(uint32_t));

    // Success cases
    {
        wgpu::RenderPipeline pipeline = CreateRenderPipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t data[] = {0};
        uint32_t offset = maxImmediateDataRangeByteSize / sizeof(uint32_t) - 1;
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetPipeline(pipeline);
        renderPass.SetImmediateData(offset, 1, data);
        renderPass.End();
        encoder.Finish();
    }

    // Success case with zero immediate data count and nullptr data array
    {
        wgpu::ComputePipeline pipeline = CreateComputePipeline();
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t offset = maxImmediateDataRangeByteSize / sizeof(uint32_t) - 1;
        uint32_t data[] = {0};
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetPipeline(pipeline);
        computePass.SetImmediateData(offset, 1, data);
        computePass.End();
        encoder.Finish();
    }

    // Failed cases with non-zero immediate data count but nullptr data array.
    {
        PlaceholderRenderPass placeholderRenderPass(device);
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t offset = maxImmediateDataRangeByteSize / sizeof(uint32_t);
        uint32_t data[] = {0};
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&placeholderRenderPass);
        renderPass.SetImmediateData(offset, 1, data);
        renderPass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }

    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        uint32_t offset = maxImmediateDataRangeByteSize / sizeof(uint32_t);
        uint32_t data[] = {0};
        wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
        computePass.SetImmediateData(offset, 1, data);
        computePass.End();
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

}  // anonymous namespace
}  // namespace dawn
