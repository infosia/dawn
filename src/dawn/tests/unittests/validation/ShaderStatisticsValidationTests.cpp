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

#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class ShaderStatisticsValidationTest : public ValidationTest {};

// Without the ShaderStatistics feature, GetShaderStatistics returns error.
TEST_F(ShaderStatisticsValidationTest, ReturnsErrorWithoutFeature) {
    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {}
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);
    wgpu::ShaderStatistics stats = {};
    EXPECT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Error);
}

class ShaderStatisticsWithFeatureTest : public ValidationTest {
  protected:
    void SetUp() override {
        DAWN_SKIP_TEST_IF(UsesWire());
        ValidationTest::SetUp();
        DAWN_SKIP_TEST_IF(!adapter.HasFeature(wgpu::FeatureName::ShaderStatistics));
    }
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ShaderStatistics};
    }
};

// With the feature enabled, GetShaderStatistics succeeds on compute pipelines.
TEST_F(ShaderStatisticsWithFeatureTest, ComputePipelineSucceeds) {
    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {}
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);
    wgpu::ShaderStatistics stats = {};
    EXPECT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Success);
}

// With the feature enabled, GetShaderStatistics succeeds on render pipelines.
TEST_F(ShaderStatisticsWithFeatureTest, RenderPipelineSucceeds) {
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

    utils::ComboRenderPipelineDescriptor desc;
    desc.vertex.module = vsModule;
    desc.cFragment.module = fsModule;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&desc);

    wgpu::ShaderStatistics stats = {};
    EXPECT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Success);
}

}  // namespace
}  // namespace dawn
