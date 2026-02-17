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

#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class MultisampledInterpolationTest : public DawnTest {
  protected:
    static constexpr uint32_t kSampleCount = 4;
    static constexpr uint32_t kRTSize = 1;
};

// Test that @builtin(position) reports pixel-centered coordinates in a multisampled fragment shader
// that also uses @builtin(sample_index). On Vulkan without polyfill, position.xy might report the
// sample position instead of the pixel center. The polyfill ensures position.xy is always
// floor(pos.xy) + 0.5.
//
// This test renders a full-screen triangle to a 1x1 multisampled target. The fragment shader writes
// position.xy into a storage buffer for each sample. After rendering, we verify that all samples
// report (0.5, 0.5) as the position (pixel-centered).
TEST_P(MultisampledInterpolationTest, SamplePositions) {
    // This test uses textureLoad on multisampled textures which isn't supported in compat mode.
    DAWN_TEST_UNSUPPORTED_IF(IsCompatibilityMode());

    // Create a 1x1 multisampled render target.
    wgpu::TextureDescriptor msDesc;
    msDesc.size = {kRTSize, kRTSize, 1};
    msDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    msDesc.sampleCount = kSampleCount;
    msDesc.usage = wgpu::TextureUsage::RenderAttachment;
    wgpu::Texture msTexture = device.CreateTexture(&msDesc);

    // Storage buffer to hold position values from each sample invocation.
    // We store 2 floats (x, y) per sample.
    static constexpr uint64_t kBufferSize = kSampleCount * 2 * sizeof(float);
    wgpu::BufferDescriptor bufDesc;
    bufDesc.size = kBufferSize;
    bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer resultBuffer = device.CreateBuffer(&bufDesc);

    // Vertex shader outputs a full-screen triangle.
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, R"(
        @vertex
        fn main(@builtin(vertex_index) vertexIndex : u32) -> @builtin(position) vec4f {
            var pos = array<vec2f, 3>(
                vec2f(-1.0, -3.0),
                vec2f(3.0, 1.0),
                vec2f(-1.0, 1.0)
            );
            return vec4f(pos[vertexIndex], 0.0, 1.0);
        })");

    // Fragment shader: writes position.xy into the storage buffer at the offset given by
    // sample_index. Using sample_index triggers per-sample shading.
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, R"(
        struct Results {
            pos : array<vec2f, 4>,
        }
        @group(0) @binding(0) var<storage, read_write> results : Results;

        @fragment
        fn main(@builtin(position) position : vec4f,
                @builtin(sample_index) sampleIndex : u32) -> @location(0) vec4f {
            results.pos[sampleIndex] = position.xy;
            return vec4f(1.0, 0.0, 0.0, 1.0);
        })");

    utils::ComboRenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.module = vsModule;
    pipelineDesc.cFragment.module = fsModule;
    pipelineDesc.multisample.count = kSampleCount;
    pipelineDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                     {{0, resultBuffer, 0, kBufferSize}});

    // Render pass
    utils::ComboRenderPassDescriptor renderPassDesc({msTexture.CreateView()});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.Draw(3);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Check that all samples report pixel-centered position (0.5, 0.5).
    std::array<float, 8> expected = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    EXPECT_BUFFER_FLOAT_RANGE_EQ(expected.data(), resultBuffer, 0, 8);
}

DAWN_INSTANTIATE_TEST(MultisampledInterpolationTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
