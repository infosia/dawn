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

#include "dawn/tests/perf_tests/DawnPerfTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

constexpr unsigned int kNumIterations = 3;

using StoreOp = wgpu::StoreOp;
DAWN_TEST_PARAM_STRUCT(StoreOpParams, StoreOp);

// Measures the cost of rendering with a depth attachment using loadOp=Clear and configurable
// storeOp (Discard vs Store).
class StoreOpDepthPerfTest : public DawnPerfTestWithParams<StoreOpParams> {
  public:
    StoreOpDepthPerfTest() : DawnPerfTestWithParams(kNumIterations, 1) {}
    ~StoreOpDepthPerfTest() override = default;

    void SetUpPerfTest() override;

  private:
    void Step() override;

    wgpu::Texture CreateColorTexture() {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = kWidth;
        descriptor.size.height = kHeight;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::RenderAttachment;
        return device.CreateTexture(&descriptor);
    }

    wgpu::Texture CreateDepthTexture() {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = kWidth;
        descriptor.size.height = kHeight;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::Depth24Plus;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::RenderAttachment;
        return device.CreateTexture(&descriptor);
    }

    static constexpr uint32_t kWidth = 4096;
    static constexpr uint32_t kHeight = 4096;
    static constexpr uint32_t kNumTextures = 5;

    wgpu::Texture colorTexture[kNumTextures];
    wgpu::TextureView colorTextureView[kNumTextures];
    wgpu::Texture depthTexture[kNumTextures];
    wgpu::TextureView depthTextureView[kNumTextures];

    wgpu::RenderPipeline pipeline;
};

void StoreOpDepthPerfTest::SetUpPerfTest() {
    const char* vs = R"(
        @vertex
        fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
            const pos = array(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0));
            return vec4f(pos[VertexIndex], 0.0, 1.0);
        })";
    const char* fs = R"(
        @fragment
        fn main() -> @location(0) vec4f {
            return vec4f(0.0, 1.0, 0.0, 1.0);
        })";

    utils::ComboRenderPipelineDescriptor pipelineDescriptor;
    pipelineDescriptor.vertex.module = utils::CreateShaderModule(device, vs);
    pipelineDescriptor.cFragment.module = utils::CreateShaderModule(device, fs);
    pipelineDescriptor.cFragment.targetCount = 1;
    pipelineDescriptor.cTargets[0].writeMask = wgpu::ColorWriteMask::All;
    pipelineDescriptor.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
    pipelineDescriptor.EnableDepthStencil(wgpu::TextureFormat::Depth24Plus);
    pipelineDescriptor.cDepthStencil.depthWriteEnabled = wgpu::OptionalBool::True;
    pipelineDescriptor.cDepthStencil.depthCompare = wgpu::CompareFunction::Less;
    pipeline = device.CreateRenderPipeline(&pipelineDescriptor);

    for (uint32_t i = 0; i < kNumTextures; ++i) {
        colorTexture[i] = CreateColorTexture();
        depthTexture[i] = CreateDepthTexture();
        colorTextureView[i] = colorTexture[i].CreateView();
        depthTextureView[i] = depthTexture[i].CreateView();
    }
}

void StoreOpDepthPerfTest::Step() {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    if (SupportsTimestampQuery()) {
        RecordBeginTimestamp(encoder);
    }

    for (unsigned int iteration = 0; iteration < kNumIterations; ++iteration) {
        for (uint32_t i = 0; i < kNumTextures; ++i) {
            utils::ComboRenderPassDescriptor renderPass({colorTextureView[i]}, depthTextureView[i]);
            renderPass.UnsetDepthStencilLoadStoreOpsForFormat(wgpu::TextureFormat::Depth24Plus);

            // Color attachment
            renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
            renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
            renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};

            // Depth attachment: loadOp=Clear, storeOp=configurable
            renderPass.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
            renderPass.cDepthStencilAttachmentInfo.depthStoreOp = GetParam().mStoreOp;
            renderPass.cDepthStencilAttachmentInfo.depthClearValue = 1.0f;

            wgpu::RenderPassEncoder renderPassEncoder = encoder.BeginRenderPass(&renderPass);
            renderPassEncoder.SetPipeline(pipeline);
            renderPassEncoder.Draw(3);
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

TEST_P(StoreOpDepthPerfTest, Run) {
    RunTest();
}

DAWN_INSTANTIATE_TEST_P(StoreOpDepthPerfTest,
                        {D3D11Backend(), D3D12Backend(), MetalBackend(), OpenGLBackend(),
                         VulkanBackend()},
                        {wgpu::StoreOp::Discard, wgpu::StoreOp::Store});

}  // anonymous namespace
}  // namespace dawn
