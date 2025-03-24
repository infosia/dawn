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

constexpr unsigned int kNumIterations = 20;

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
    static constexpr uint32_t kNumTextures = 5;

    wgpu::Texture msaaTexture[kNumTextures];
    wgpu::TextureView msaaTextureView[kNumTextures];
    wgpu::Texture resolveTexture[kNumTextures];
    wgpu::TextureView resolveTextureView[kNumTextures];

    wgpu::Texture srcTexture1[kNumTextures];
    wgpu::Texture srcTexture2[kNumTextures];
    wgpu::TextureView srcTextureView1[kNumTextures];
    wgpu::TextureView srcTextureView2[kNumTextures];

    wgpu::RenderPipeline msaaPipeline;
    wgpu::RenderPipeline singleSampledPipeline;
    wgpu::BindGroup blitBindGroup1[kNumTextures];
    wgpu::BindGroup blitBindGroup2[kNumTextures];
};

void LoadOpPerfTest::SetUp() {
    DawnPerfTestWithParams<LoadOpParams>::SetUp();

    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float},
                });

    wgpu::PipelineLayout pipelineLayout = utils::MakePipelineLayout(device, {bgl});

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

    pipelineDescriptor.cFragment.targetCount = 1;
    pipelineDescriptor.cTargets[0].writeMask = wgpu::ColorWriteMask::All;
    pipelineDescriptor.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;

    pipelineDescriptor.layout = pipelineLayout;

    pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
    pipelineDescriptor.multisample.count = 4;
    msaaPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    pipelineDescriptor.multisample.count = 1;
    singleSampledPipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    for (uint32_t i = 0; i < kNumTextures; ++i) {
        msaaTexture[i] = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 4);
        resolveTexture[i] = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);
        srcTexture1[i] = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);
        srcTexture2[i] = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);

        msaaTextureView[i] = msaaTexture[i].CreateView();
        resolveTextureView[i] = resolveTexture[i].CreateView();
        srcTextureView1[i] = srcTexture1[i].CreateView();
        srcTextureView2[i] = srcTexture2[i].CreateView();

        blitBindGroup1[i] = utils::MakeBindGroup(device, bgl, {{0, srcTextureView1[i]}});
        blitBindGroup2[i] = utils::MakeBindGroup(device, bgl, {{0, srcTextureView2[i]}});

        // Clear the textures
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        float colorScale = std::max(0.1f, i / float(kNumTextures));
        {
            utils::ComboRenderPassDescriptor renderPass({msaaTextureView[i]});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {0.0f, colorScale, 0.0f, 0.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
        {
            utils::ComboRenderPassDescriptor renderPass({srcTextureView1[i]});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {colorScale, 0.0f, colorScale, 0.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
        {
            utils::ComboRenderPassDescriptor renderPass({srcTextureView2[i]});
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].clearValue = {0.0f, colorScale, colorScale, 0.0f};

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.End();
        }
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }
}

void LoadOpPerfTest::Step() {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    if (SupportsTimestampQuery()) {
        RecordBeginTimestamp(encoder);
    }

    for (unsigned int iteration = 0; iteration < kNumIterations; ++iteration) {
        for (uint32_t i = 0; i < kNumTextures; ++i) {
            // 1st pass: blit the src texture 1 to the MSAA texture.
            {
                utils::ComboRenderPassDescriptor renderPass({msaaTextureView[i]});
                renderPass.cColorAttachments[0].resolveTarget = resolveTextureView[i];
                renderPass.cColorAttachments[0].loadOp = GetParam().mLoadOp;
                renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

                wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
                renderPassEncoder.SetBindGroup(0, blitBindGroup1[i]);
                renderPassEncoder.SetPipeline(msaaPipeline);
                renderPassEncoder.Draw(3);
                renderPassEncoder.End();
            }
            // 2nd pass: blit the src texture 2 to the resolve texture.
            {
                utils::ComboRenderPassDescriptor renderPass({resolveTextureView[i]});
                renderPass.cColorAttachments[0].loadOp = GetParam().mLoadOp;
                renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

                wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
                renderPassEncoder.SetBindGroup(0, blitBindGroup2[i]);
                renderPassEncoder.SetPipeline(singleSampledPipeline);
                renderPassEncoder.Draw(3);
                renderPassEncoder.End();
            }
            // 3rd pass: blit the src texture 2 to the MSAA texture.
            {
                utils::ComboRenderPassDescriptor renderPass({msaaTextureView[i]});
                renderPass.cColorAttachments[0].resolveTarget = resolveTextureView[i];
                renderPass.cColorAttachments[0].loadOp = GetParam().mLoadOp;
                renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

                wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
                renderPassEncoder.SetBindGroup(0, blitBindGroup2[i]);
                renderPassEncoder.SetPipeline(msaaPipeline);
                renderPassEncoder.Draw(3);
                renderPassEncoder.End();
            }
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
