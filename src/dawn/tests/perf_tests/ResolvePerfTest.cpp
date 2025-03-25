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

constexpr unsigned int kNumIterations = 100;

enum ResolveMode {
    Draw,
    StoreOp,
};

std::ostream& operator<<(std::ostream& ostream, const ResolveMode& resolveMode) {
    switch (resolveMode) {
        case ResolveMode::Draw:
            ostream << "Draw";
            break;
        case ResolveMode::StoreOp:
            ostream << "StoreOp";
            break;
    }
    return ostream;
}

DAWN_TEST_PARAM_STRUCT(Params, ResolveMode);

class ResolvePerfTest : public DawnPerfTestWithParams<Params> {
  public:
    ResolvePerfTest() : DawnPerfTestWithParams(kNumIterations, 1) {}
    ~ResolvePerfTest() override = default;

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
    wgpu::Texture resolveTexture;
    wgpu::TextureView resolveTextureView;

    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup msaaSrcBindGroup;
};

void ResolvePerfTest::SetUp() {
    DawnPerfTestWithParams<Params>::SetUp();

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
        @group(0) @binding(0) var colorMap: texture_multisampled_2d<f32>;
        @fragment 
        fn main(@builtin(position) fragPosition : vec4<f32>) -> @location(0) vec4<f32> {
            let coords = vec2<i32>(i32(fragPosition.x), i32(fragPosition.y));
            var sum = textureLoad(colorMap, coords, 0);
            sum += textureLoad(colorMap, coords, 1);
            sum += textureLoad(colorMap, coords, 2);
            sum += textureLoad(colorMap, coords, 3);
            return sum * 0.25;
        })";
    utils::ComboRenderPipelineDescriptor pipelineDescriptor;

    pipelineDescriptor.vertex.module = utils::CreateShaderModule(device, vs);
    pipelineDescriptor.cFragment.module = utils::CreateShaderModule(device, fs);

    pipelineDescriptor.cFragment.targetCount = 1;
    pipelineDescriptor.cTargets[0].writeMask = wgpu::ColorWriteMask::All;
    pipelineDescriptor.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;

    pipelineDescriptor.multisample.mask = 0xFFFFFFFF;
    pipelineDescriptor.multisample.count = 1;
    pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    msaaTexture = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 4);
    resolveTexture = CreateTexture(wgpu::TextureFormat::RGBA8Unorm, 1);

    msaaTextureView = msaaTexture.CreateView();
    resolveTextureView = resolveTexture.CreateView();

    msaaSrcBindGroup =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, msaaTextureView}});

    // Clear the resolve texture
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        utils::ComboRenderPassDescriptor renderPass({resolveTextureView});
        renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass.cColorAttachments[0].clearValue = {0.f, 0.f, 0.f, 0.f};

        wgpu::RenderPassEncoder renderPassEncoder =
            encoder.BeginRenderPass(&renderPass);
        renderPassEncoder.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }
}

void ResolvePerfTest::Step() {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    if (SupportsTimestampQuery()) {
        RecordBeginTimestamp(encoder);
    }

    for (unsigned int iteration = 0; iteration < kNumIterations; ++iteration) {
        // 1st pass: blit the src texture 1 to the MSAA texture.
        float colorScale = std::max(0.1f, float(iteration) / kNumIterations);
        switch (GetParam().mResolveMode) {
            case ResolveMode::StoreOp: {
                utils::ComboRenderPassDescriptor renderPass({msaaTextureView});
                renderPass.cColorAttachments[0].resolveTarget = resolveTextureView;
                renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
                renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Discard;
                renderPass.cColorAttachments[0].clearValue = {colorScale, colorScale, colorScale,
                                                              colorScale};

                wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
                renderPassEncoder.End();
            } break;
            case ResolveMode::Draw: {
                {
                    utils::ComboRenderPassDescriptor renderPass({msaaTextureView});
                    renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
                    renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
                    renderPass.cColorAttachments[0].clearValue = {colorScale, colorScale,
                                                                  colorScale, colorScale};

                    wgpu::RenderPassEncoder renderPassEncoder =
                        encoder.BeginRenderPass(&renderPass);
                    renderPassEncoder.End();
                }

                {
                    utils::ComboRenderPassDescriptor renderPass({resolveTextureView});
                    renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Load;
                    renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;

                    wgpu::RenderPassEncoder renderPassEncoder =
                        encoder.BeginRenderPass(&renderPass);
                    renderPassEncoder.SetBindGroup(0, msaaSrcBindGroup);
                    renderPassEncoder.SetPipeline(pipeline);
                    renderPassEncoder.Draw(3);
                    renderPassEncoder.End();
                }
            } break;
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

TEST_P(ResolvePerfTest, Run) {
    RunTest();
}

DAWN_INSTANTIATE_TEST_P(ResolvePerfTest,
                        {D3D11Backend(), D3D12Backend(), MetalBackend(), OpenGLBackend(),
                         VulkanBackend()},
                        {ResolveMode::Draw, ResolveMode::StoreOp});

}  // anonymous namespace
}  // namespace dawn
