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

class FragmentShadingRateTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures;
        if (SupportsFeatures({wgpu::FeatureName::DawnRenderPipelineFragmentShadingRate})) {
            requiredFeatures.push_back(wgpu::FeatureName::DawnRenderPipelineFragmentShadingRate);
        }
        return requiredFeatures;
    }
};

TEST_P(FragmentShadingRateTests, SampleShadingRunsFewerTimes) {
    DAWN_TEST_UNSUPPORTED_IF(
        !SupportsFeatures({wgpu::FeatureName::DawnRenderPipelineFragmentShadingRate}));

    wgpu::TextureDescriptor renderTargetDesc;
    renderTargetDesc.dimension = wgpu::TextureDimension::e2D;
    renderTargetDesc.size = {4, 4, 1};
    renderTargetDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    renderTargetDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    wgpu::Texture renderTarget = device.CreateTexture(&renderTargetDesc);

    wgpu::BufferDescriptor bufDesc;
    bufDesc.size = 4;
    bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer invocationBuffer = device.CreateBuffer(&bufDesc);

    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
    wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{0, invocationBuffer}});

    wgpu::DawnRenderPipelineFragmentShadingRate shadingRate;
    shadingRate.fragmentSize = {2, 2};

    utils::ComboRenderPipelineDescriptor pDesc;
    pDesc.nextInChain = &shadingRate;
    pDesc.layout = utils::MakePipelineLayout(device, {bgl});
    pDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
    pDesc.vertex.module = utils::CreateShaderModule(device, R"(
        @vertex fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
            var pos = array<vec2f, 3>(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0)
            );
            return vec4f(pos[VertexIndex], 0.0, 1.0);
        }
    )");
    pDesc.cFragment.module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var<storage, read_write> invocationCount : atomic<u32>;

        @fragment fn main() -> @location(0) vec4f {
            atomicAdd(&invocationCount, 1u);
            return vec4f(1.0, 0.0, 0.0, 1.0);
        }
    )");

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pDesc);

    utils::ComboRenderPassDescriptor passDesc({renderTarget.CreateView()});
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.Draw(3);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // 4x4 texture = 16 pixels. Fragment size 2x2 means 4 fragments total.
    EXPECT_BUFFER_U32_EQ(4, invocationBuffer, 0);
}

DAWN_INSTANTIATE_TEST(FragmentShadingRateTests, VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
