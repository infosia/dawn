// Copyright 2025 The Dawn & Tint Authors
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
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_TEST_UNSUPPORTED_IF(IsCompatibilityMode());
        DAWN_TEST_UNSUPPORTED_IF(IsSwiftshader());
        DAWN_TEST_UNSUPPORTED_IF(IsANGLESwiftShader());
    }
};

// Test that @builtin(position) in multisampled fragment shaders reports pixel-centered values
// (floor(pos.xy) + 0.5) rather than sample positions, even when per-sample shading is triggered.
TEST_P(MultisampledInterpolationTest, SamplePositions) {
    constexpr uint32_t kWidth = 10;
    constexpr uint32_t kHeight = 10;
    constexpr uint32_t kSampleCount = 4;

    // Create multisampled render targets for encoding position components.
    // We use 4 render targets to encode the 4 components of position as rgba8unorm.
    wgpu::TextureDescriptor msTexDesc;
    msTexDesc.size = {kWidth, kHeight, 1};
    msTexDesc.format = wgpu::TextureFormat::RGBA8Unorm;
    msTexDesc.sampleCount = kSampleCount;
    msTexDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

    wgpu::Texture msTarget0 = device.CreateTexture(&msTexDesc);
    wgpu::Texture msTarget1 = device.CreateTexture(&msTexDesc);
    wgpu::Texture msTarget2 = device.CreateTexture(&msTexDesc);
    wgpu::Texture msTarget3 = device.CreateTexture(&msTexDesc);

    // Vertex shader: full-screen triangle with inter-stage output using linear/center
    // interpolation.
    const char* vsSource = R"(
        struct VSOutput {
            @builtin(position) position : vec4f,
            @location(0) @interpolate(linear, center) interp : vec4f,
        };

        @vertex fn main(@builtin(vertex_index) vertexIndex : u32) -> VSOutput {
            var pos = array<vec2f, 3>(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0),
            );
            var output : VSOutput;
            output.position = vec4f(pos[vertexIndex], 0.0, 1.0);
            output.interp = vec4f(pos[vertexIndex], 0.0, 1.0);
            return output;
        }
    )";

    // Fragment shader: uses @builtin(sample_index) to trigger per-sample shading,
    // then encodes @builtin(position) xy into the first two render targets.
    // Each target stores a normalized float where the value is pos component / 256.
    const char* fsSource = R"(
        struct FSOutput {
            @location(0) target0 : vec4f,
            @location(1) target1 : vec4f,
            @location(2) target2 : vec4f,
            @location(3) target3 : vec4f,
        };

        @fragment fn main(
            @builtin(position) pos : vec4f,
            @builtin(sample_index) sampleIndex : u32,
            @location(0) @interpolate(linear, center) interp : vec4f,
        ) -> FSOutput {
            var output : FSOutput;
            // Encode position.x fractional part
            let fx = fract(pos.x);
            output.target0 = vec4f(fx, fx, fx, 1.0);
            // Encode position.y fractional part
            let fy = fract(pos.y);
            output.target1 = vec4f(fy, fy, fy, 1.0);
            // Encode position.z
            output.target2 = vec4f(pos.z, pos.z, pos.z, 1.0);
            // Encode position.w
            output.target3 = vec4f(pos.w, pos.w, pos.w, 1.0);
            return output;
        }
    )";

    // Create render pipeline
    utils::ComboRenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.module = utils::CreateShaderModule(device, vsSource);
    pipelineDesc.cFragment.module = utils::CreateShaderModule(device, fsSource);
    pipelineDesc.multisample.count = kSampleCount;
    pipelineDesc.cFragment.targetCount = 4;
    pipelineDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
    pipelineDesc.cTargets[1].format = wgpu::TextureFormat::RGBA8Unorm;
    pipelineDesc.cTargets[2].format = wgpu::TextureFormat::RGBA8Unorm;
    pipelineDesc.cTargets[3].format = wgpu::TextureFormat::RGBA8Unorm;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create compute shader to read back multisampled texture and extract position values.
    const char* csSource = R"(
        @group(0) @binding(0) var msTexX : texture_multisampled_2d<f32>;
        @group(0) @binding(1) var msTexY : texture_multisampled_2d<f32>;
        @group(0) @binding(2) var<storage, read_write> result : array<f32>;

        @compute @workgroup_size(1)
        fn main(@builtin(global_invocation_id) id : vec3u) {
            // Read all 4 samples for pixel (0, 0) from x and y targets.
            for (var s = 0u; s < 4u; s = s + 1u) {
                let sampleX = textureLoad(msTexX, vec2u(0, 0), s);
                let sampleY = textureLoad(msTexY, vec2u(0, 0), s);
                result[s * 2u] = sampleX.r;       // fract(position.x)
                result[s * 2u + 1u] = sampleY.r;  // fract(position.y)
            }
        }
    )";

    // Create compute pipeline
    wgpu::ComputePipelineDescriptor computeDesc;
    computeDesc.compute.module = utils::CreateShaderModule(device, csSource);
    wgpu::ComputePipeline computePipeline = device.CreateComputePipeline(&computeDesc);

    // Create output buffer for 4 samples * 2 floats (x, y) = 8 floats
    constexpr uint32_t kResultSize = 4 * 2 * sizeof(float);
    wgpu::BufferDescriptor bufDesc;
    bufDesc.size = kResultSize;
    bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
    wgpu::Buffer resultBuffer = device.CreateBuffer(&bufDesc);

    // Render pass
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    {
        utils::ComboRenderPassDescriptor renderPassDesc(
            {msTarget0.CreateView(), msTarget1.CreateView(), msTarget2.CreateView(),
             msTarget3.CreateView()});
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();
    }

    // Compute pass to read back
    {
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(
            device, computePipeline.GetBindGroupLayout(0),
            {{0, msTarget0.CreateView()}, {1, msTarget1.CreateView()}, {2, resultBuffer}});
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(computePipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.DispatchWorkgroups(1);
        pass.End();
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // For pixel (0, 0), the pixel center is (0.5, 0.5).
    // fract(0.5) = 0.5
    // All 4 samples should report 0.5 for both x and y if the polyfill works correctly.
    // Without the polyfill on Vulkan, the values would vary per-sample.
    std::array<float, 8> expected;
    expected.fill(0.5f);

    EXPECT_BUFFER_FLOAT_RANGE_TOLERANCE_EQ(expected.data(), resultBuffer, 0, 8, 0.01f);
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
