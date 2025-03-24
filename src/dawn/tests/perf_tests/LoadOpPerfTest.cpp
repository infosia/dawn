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

#include "dawn/tests/perf_tests/DawnPerfTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

constexpr unsigned int kNumIterations = 50;

using LoadOp = wgpu::LoadOp;
DAWN_TEST_PARAM_STRUCT(LoadOpParams, LoadOp);

class LoadOpPerfTest : public DawnPerfTestWithParams<LoadOpParams> {
  public:
    LoadOpPerfTest() : DawnPerfTestWithParams(kNumIterations, 1) {}
    ~LoadOpPerfTest() override = default;

    void SetUp() override;

  private:
    void Step() override;

    wgpu::Texture CreateTexture(wgpu::TextureFormat format, uint32_t sampleCount) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = kWidth;
        descriptor.size.height = kHeight;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = sampleCount;
        descriptor.format = format;
        descriptor.mipLevelCount = 1;

        descriptor.usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

        return device.CreateTexture(&descriptor);
    }

    static constexpr uint32_t kWidth = 4096;
    static constexpr uint32_t kHeight = 4096;

    wgpu::Texture msaaTexture;
    wgpu::TextureView msaaTextureView;
    wgpu::Texture singleSampledTexture;
    wgpu::TextureView singleSampledTextureView;
    wgpu::Texture resolveTexture;
    wgpu::TextureView resolveTextureView;

    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup blitBindGroup;
};

void LoadOpPerfTest::SetUp() {
    DawnPerfTestWithParams<LoadOpParams>::SetUp();

    msaaTexture = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 4);
    singleSampledTexture = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);
    resolveTexture = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);

    msaaTextureView = msaaTexture.CreateView();
    singleSampledTextureView = singleSampledTexture.CreateView();
    resolveTextureView = resolveTexture.CreateView();

    const char* vs = R"(
        @vertex
        fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
            const pos = array(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0));
            return vec4f(pos[VertexIndex], 0.0, 1.0);
        })";
    constexpr char fs[] = R"(
        @group(0) @binding(0) var colorMap: texture_2d<f32>;
        @fragment 
        fn main(@builtin(position) fragPosition : vec4<f32>) -> @location(0) vec4<f32> {
            let coords = vec2<i32>(i32(fragPosition.x), i32(fragPosition.y));
            return textureLoad(colorMap, coords, 0);
        })";
    utils::ComboRenderPipelineDescriptor pipelineDescriptor;

    pipelineDescriptor.vertex.module = utils::CreateShaderModule(device, vs);
    pipelineDescriptor.cFragment.module = utils::CreateShaderModule(device, fs);

    pipelineDescriptor.multisample.count = 4;
    pipelineDescriptor.multisample.mask = 0xFFFFFFFF;

    pipelineDescriptor.cFragment.targetCount = 1;
    pipelineDescriptor.cTargets[0].writeMask = wgpu::ColorWriteMask::All;
    pipelineDescriptor.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;

    pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    blitBindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                         {{0, singleSampledTextureView}});
}

void LoadOpPerfTest::Step() {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    if (SupportsTimestampQuery()) {
        RecordBeginTimestamp(encoder);
    }

    for (unsigned int i = 0; i < kNumIterations; ++i) {
        // First pass: clear the MSAA texture
        {
            utils::ComboRenderPassDescriptor renderPass({msaaTextureView});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {0.0f, 1.0f, 0.0f, 0.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
        // 2nd pass: clear the single sampled texture
        {
            utils::ComboRenderPassDescriptor renderPass({singleSampledTextureView});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {1.0f, 0.0f, 1.0f, 0.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
        // 3rd pass: blit the single sampled texture to the MSAA texture.
        {
            utils::ComboRenderPassDescriptor renderPass({msaaTextureView});
            renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
            renderPass.cColorAttachments[0].loadOp = GetParam().mLoadOp;
            renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
            renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Discard;

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.SetBindGroup(0, blitBindGroup);
            renderPassEncoder.SetPipeline(pipeline);
            renderPassEncoder.Draw(3);
            renderPassEncoder.End();
        }
        // 4th pass: clear the single sampled texture
        {
            utils::ComboRenderPassDescriptor renderPass({singleSampledTextureView});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {0.0f, 1.0f, 1.0f, 1.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
    }

    if (SupportsTimestampQuery()) {
        RecordEndTimestampAndResolveQuerySet(encoder);
    }

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    if (SupportsTimestampQuery()) {
        ComputeGPUElapsedTime();
    }
}

TEST_P(LoadOpPerfTest, Run) {
    RunTest();
}

DAWN_INSTANTIATE_TEST_P(LoadOpPerfTest,
                        {D3D11Backend(), D3D12Backend(), MetalBackend(), OpenGLBackend(),
                         VulkanBackend()},
                        {wgpu::LoadOp::Load, wgpu::LoadOp::Clear});

}  // anonymous namespace
}  // namespace dawn
