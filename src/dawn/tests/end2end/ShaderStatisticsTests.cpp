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

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class ShaderStatisticsTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        if (!SupportsFeatures({wgpu::FeatureName::ShaderStatistics})) {
            return {};
        }
        return {wgpu::FeatureName::ShaderStatistics};
    }
};

// Compute pipeline reports at least one stage with Compute shader stage.
TEST_P(ShaderStatisticsTests, ComputePipelineReportsStats) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));

    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var<storage, read_write> buf: array<f32>;
        @compute @workgroup_size(64)
        fn main(@builtin(global_invocation_id) id: vec3u) {
            buf[id.x] = buf[id.x] * 2.0;
        }
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);

    wgpu::ShaderStatistics stats = {};
    ASSERT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Success);
    ASSERT_GE(stats.stageCount, 1u);
    EXPECT_EQ(stats.stages[0].stage, wgpu::ShaderStage::Compute);
}

// Render pipeline reports stats for at least one stage.
TEST_P(ShaderStatisticsTests, RenderPipelineReportsStats) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));

    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, R"(
        @vertex fn main() -> @builtin(position) vec4f {
            return vec4f(0.0, 0.0, 0.0, 1.0);
        }
    )");
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, R"(
        @fragment fn main() -> @location(0) vec4f {
            return vec4f(1.0, 0.0, 0.0, 1.0);
        }
    )");

    utils::ComboRenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.module = vsModule;
    pipelineDesc.cFragment.module = fsModule;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    wgpu::ShaderStatistics stats = {};
    ASSERT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Success);
    EXPECT_GE(stats.stageCount, 1u);
}

// ValidFields bitmask is consistent with non-zero field values.
TEST_P(ShaderStatisticsTests, ValidFieldsAreConsistent) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));

    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var<storage, read_write> buf: array<f32>;
        @compute @workgroup_size(64)
        fn main(@builtin(global_invocation_id) id: vec3u) {
            buf[id.x] = buf[id.x] * 2.0;
        }
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);

    wgpu::ShaderStatistics stats = {};
    ASSERT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Success);

    for (size_t i = 0; i < stats.stageCount; i++) {
        const auto& core = stats.stages[i].core;
        // Every backend should report at least some data.
        EXPECT_TRUE(core.validFields != wgpu::ShaderStatisticValidField::None ||
                    stats.stages[i].extensionEntryCount > 0)
            << "Stage " << i << " has no core fields or extension entries";
        if (core.validFields & wgpu::ShaderStatisticValidField::RegisterCount) {
            EXPECT_GT(core.registerCount, 0u);
        }
        if (core.validFields & wgpu::ShaderStatisticValidField::SimdWidth) {
            EXPECT_GT(core.simdWidth, 0u);
        }
        if (core.validFields & wgpu::ShaderStatisticValidField::MaxThreadsPerGroup) {
            EXPECT_GT(core.maxThreadsPerGroup, 0u);
        }
        if (core.validFields & wgpu::ShaderStatisticValidField::InstructionCount) {
            EXPECT_GT(core.instructionCount, 0u);
        }
    }
}

DAWN_INSTANTIATE_TEST(ShaderStatisticsTests,
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend());

}  // namespace
}  // namespace dawn
