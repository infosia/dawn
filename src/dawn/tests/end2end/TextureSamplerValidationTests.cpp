// Copyright 2026 The Dawn & Tint Authors
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

#include <array>
#include <string>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"
#include "webgpu/webgpu_cpp.h"

namespace dawn {
namespace {

class TextureSamplerValidationTests : public DawnTest {
  protected:
    wgpu::PipelineLayout CreatePipelineLayout(wgpu::TextureSampleType textureSampleType,
                                              wgpu::SamplerBindingType samplerBindingType) {
        std::vector<wgpu::BindGroupLayoutEntry> bglEntries = {
            wgpu::BindGroupLayoutEntry{
                .binding = 0,
                .visibility = wgpu::ShaderStage::Compute,
                .texture =
                    {
                        .sampleType = textureSampleType,
                        .viewDimension = wgpu::TextureViewDimension::e2D,
                    },
            },
            wgpu::BindGroupLayoutEntry{
                .binding = 1,
                .visibility = wgpu::ShaderStage::Compute,
                .sampler =
                    {
                        .type = samplerBindingType,
                    },
            },
        };

        wgpu::BindGroupLayoutDescriptor bglDesc;

        bglDesc.entries = bglEntries.data();
        bglDesc.entryCount = bglEntries.size();

        auto bindGroupLayout = device.CreateBindGroupLayout(&bglDesc);

        std::vector<wgpu::BindGroupLayout> bindgroupLayouts = {bindGroupLayout};
        wgpu::PipelineLayoutDescriptor descriptor{
            .bindGroupLayoutCount = bindgroupLayouts.size(),
            .bindGroupLayouts = bindgroupLayouts.data(),
        };
        return device.CreatePipelineLayout(&descriptor);
    }
};

TEST_P(TextureSamplerValidationTests, FilterableBGL_UnFilterableShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32, unfilterable>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, FilterableBGL_FilterableShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32, filterable>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, FilterableBGL_UnknownShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, UnfilterableBGL_UnFilterableShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32, unfilterable>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::UnfilterableFloat,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, UnfilterableBGL_FilterableShader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32, filterable>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::UnfilterableFloat,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("isn't compatible"));
}

TEST_P(TextureSamplerValidationTests, UnfilterableBGL_UnknownShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::UnfilterableFloat,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, FilterableBGL_DepthShader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_depth_2d;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("isn't compatible"));
}

TEST_P(TextureSamplerValidationTests, FilterableBGL_i32Shader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<i32>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("isn't compatible"));
}

TEST_P(TextureSamplerValidationTests, FilteringBGL_FilteringShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, FilteringBGL_NonFilteringShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<non_filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Filtering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, NonFilteringBGL_NonFilteringShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<non_filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::NonFiltering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, NonFilteringBGL_FilteringShader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::NonFiltering),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("doesn't match"));
}

TEST_P(TextureSamplerValidationTests, ComparisonBGL_ComparisonShader_Pass) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler_comparison;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Comparison),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    device.CreateComputePipeline(&pipelineDescriptor);
}

TEST_P(TextureSamplerValidationTests, ComparisonBGL_FilteringShader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Comparison),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("doesn't match"));
}

TEST_P(TextureSamplerValidationTests, ComparisonBGL_NonFilteringShader_Fail) {
    auto shaderSource = R"(
    @group(0) @binding(0) var tex1 : texture_2d<f32>;
    @group(0) @binding(1) var samp : sampler<non_filtering>;

    @compute @workgroup_size(1) fn main() {
      _ = tex1;
      _ = samp;
    })";

    wgpu::ComputePipelineDescriptor pipelineDescriptor{
        .layout = CreatePipelineLayout(wgpu::TextureSampleType::Float,
                                       wgpu::SamplerBindingType::Comparison),
        .compute =
            {
                .module = utils::CreateShaderModule(device, shaderSource),
                .entryPoint = "main",
            },
    };

    ASSERT_DEVICE_ERROR_MSG(device.CreateComputePipeline(&pipelineDescriptor),
                            testing::HasSubstr("doesn't match"));
}

DAWN_INSTANTIATE_TEST(TextureSamplerValidationTests,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend(),
                      WebGPUBackend());

}  // anonymous namespace
}  // namespace dawn
