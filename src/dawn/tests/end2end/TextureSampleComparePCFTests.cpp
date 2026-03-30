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

#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class TextureSampleComparePCFTest : public DawnTest {
  protected:
    void SetUp() override { DawnTest::SetUp(); }
};

// Test that textureSampleCompare with PCF polyfill returns the same result as the native implementation.
TEST_P(TextureSampleComparePCFTest, ComparePCF) {
    // Only run on Vulkan for this specific polyfill test.
    DAWN_TEST_UNSUPPORTED_IF(!IsVulkan());

    static constexpr uint32_t kSize = 4;
    wgpu::Extent3D textureSize = {kSize, kSize, 1};

    // Create a depth texture with a gradient.
    wgpu::TextureDescriptor depthDesc;
    depthDesc.format = wgpu::TextureFormat::Depth32Float;
    depthDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    depthDesc.size = textureSize;
    wgpu::Texture depthTexture = device.CreateTexture(&depthDesc);

    std::vector<float> depthData(kSize * kSize);
    for (uint32_t y = 0; y < kSize; ++y) {
        for (uint32_t x = 0; x < kSize; ++x) {
            depthData[y * kSize + x] = static_cast<float>(x + y) / static_cast<float>(2 * kSize);
        }
    }
    wgpu::TexelCopyTextureInfo texelCopyTextureInfo = utils::CreateTexelCopyTextureInfo(depthTexture, 0, {0, 0, 0});
    wgpu::TexelCopyBufferLayout texelCopyBufferLayout = utils::CreateTexelCopyBufferLayout(0, kSize * sizeof(float));
    queue.WriteTexture(&texelCopyTextureInfo, depthData.data(), depthData.size() * sizeof(float), &texelCopyBufferLayout, &textureSize);

    // Create a comparison sampler.
    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.compare = wgpu::CompareFunction::Less;
    wgpu::Sampler sampler = device.CreateSampler(&samplerDesc);

    // Create a render pipeline that outputs the result of textureSampleCompare.
    utils::ComboRenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.module = utils::CreateShaderModule(device, R"(
        @vertex fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
            var pos = array(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0));
            return vec4f(pos[VertexIndex], 0.0, 1.0);
        }
    )");
    pipelineDesc.cFragment.module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var t : texture_depth_2d;
        @group(0) @binding(1) var s : sampler_comparison;

        @fragment fn main() -> @location(0) f32 {
            // Sample in the middle of the 4x4 texture.
            // UV (0.5, 0.5) corresponds to the boundary between texels.
            return textureSampleCompare(t, s, vec2f(0.5, 0.5), 0.5);
        }
    )");
    pipelineDesc.cTargets[0].format = wgpu::TextureFormat::R32Float;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create an output texture to read the result.
    wgpu::TextureDescriptor outputDesc;
    outputDesc.format = wgpu::TextureFormat::R32Float;
    outputDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    outputDesc.size = {1, 1, 1};
    wgpu::Texture outputTexture = device.CreateTexture(&outputDesc);
    wgpu::TextureView outputView = outputTexture.CreateView();

    // Calculate expected PCF value manually.
    // depthData[y * 4 + x] = (x + y) / 8
    // Texels near center: (1,1), (2,1), (1,2), (2,2)
    // Values: 2/8, 3/8, 3/8, 4/8  => 0.25, 0.375, 0.375, 0.5
    // Compare with dref = 0.5 (Less):
    // v00 = 0.25 < 0.5 ? 1.0 : 0.0 -> 1.0
    // v10 = 0.375 < 0.5 ? 1.0 : 0.0 -> 1.0
    // v01 = 0.375 < 0.5 ? 1.0 : 0.0 -> 1.0
    // v11 = 0.5 < 0.5 ? 1.0 : 0.0 -> 0.0
    //
    // Bilinear interpolation at UV (0.5, 0.5):
    // On a 4x4 texture, UV (0.5, 0.5) is exactly the corner between the 4 middle texels.
    // Polyfill math:
    // texelPos = 0.5 * 4 - 0.5 = 1.5
    // i_j = floor(1.5) = 1.0
    // f = 1.5 - 1.0 = 0.5
    // top = mix(v00, v10, 0.5) = 1.0
    // bottom = mix(v01, v11, 0.5) = 0.5
    // final = mix(top, bottom, 0.5) = 0.75
    
    float expectedPCF = 0.75f;
    
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPass({outputView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, depthTexture.CreateView()}, {1, sampler}}));
    pass.Draw(3);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_FLOAT_EQ(expectedPCF, outputTexture, 0, 0);
}

DAWN_INSTANTIATE_TEST(TextureSampleComparePCFTest,
                      VulkanBackend(),
                      VulkanBackend({"vulkan_sample_compare_pcf_workaround"}));

}  // anonymous namespace
}  // namespace dawn
