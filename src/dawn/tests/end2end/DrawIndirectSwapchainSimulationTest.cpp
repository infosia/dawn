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

constexpr uint32_t kRTSize = 500;
constexpr uint32_t kIndirectDrawCount = 5120;
// Match browser behavior: continuous frame submission
constexpr uint32_t kFrameCount = 100;

// Test class that simulates browser's continuous frame submission behavior
class DrawIndirectSwapchainSimulationTest : public DawnTest {
  protected:
    void SetUp() override { DawnTest::SetUp(); }
};

TEST_P(DrawIndirectSwapchainSimulationTest, BrowserBehavior) {
    // Create shaders
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, R"(
        @vertex
        fn main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4f {
            let pos = array<vec2f, 3>(
                vec2f(0.0, 0.5),
                vec2f(-0.5, -0.5),
                vec2f(0.5, -0.5)
            );
            return vec4f(pos[vertexIndex], 0.0, 1.0);
        }
    )");

    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, R"(
        @fragment
        fn main() -> @location(0) vec4f {
            return vec4f(1.0, 0.0, 0.0, 1.0);
        }
    )");

    // Create pipeline with depth stencil
    utils::ComboRenderPipelineDescriptor pipelineDesc;
    pipelineDesc.vertex.module = vsModule;
    pipelineDesc.cFragment.module = fsModule;
    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::Back;
    pipelineDesc.cTargets[0].format = wgpu::TextureFormat::BGRA8Unorm;  // Common swapchain format

    pipelineDesc.EnableDepthStencil(wgpu::TextureFormat::Depth24Plus);
    pipelineDesc.cDepthStencil.depthWriteEnabled = true;
    pipelineDesc.cDepthStencil.depthCompare = wgpu::CompareFunction::Less;

    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

    // Create indirect buffer
    std::vector<uint32_t> indirectData(kIndirectDrawCount * 4);
    for (uint32_t i = 0; i < kIndirectDrawCount; ++i) {
        indirectData[i * 4 + 0] = 3;
        indirectData[i * 4 + 1] = 1;
        indirectData[i * 4 + 2] = 0;
        indirectData[i * 4 + 3] = i * 4;
    }
    wgpu::Buffer indirectBuffer = utils::CreateBufferFromData(
        device, indirectData.data(), indirectData.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Indirect);

    // Create depth texture (reused like in browser)
    wgpu::TextureDescriptor depthTexDesc;
    depthTexDesc.size = {kRTSize, kRTSize, 1};
    depthTexDesc.format = wgpu::TextureFormat::Depth24Plus;
    depthTexDesc.usage = wgpu::TextureUsage::RenderAttachment;
    wgpu::Texture depthTexture = device.CreateTexture(&depthTexDesc);
    wgpu::TextureView depthTextureView = depthTexture.CreateView();

    dawn::InfoLog() << "Starting frame loop, will submit " << kFrameCount << " frames";

    // Simulate browser's continuous frame submission
    for (uint32_t frame = 0; frame < kFrameCount; ++frame) {
        // Create new render target each frame (simulates swapchain GetCurrentTexture)
        wgpu::TextureDescriptor texDesc;
        texDesc.size = {kRTSize, kRTSize, 1};
        texDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        texDesc.usage = wgpu::TextureUsage::RenderAttachment;
        wgpu::Texture renderTarget = device.CreateTexture(&texDesc);
        wgpu::TextureView renderTargetView = renderTarget.CreateView();

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        utils::ComboRenderPassDescriptor renderPassDesc({renderTargetView});
        renderPassDesc.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassDesc.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPassDesc.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;

        renderPassDesc.cDepthStencilAttachmentInfo.view = depthTextureView;
        renderPassDesc.cDepthStencilAttachmentInfo.depthClearValue = 1.0f;
        renderPassDesc.cDepthStencilAttachmentInfo.depthLoadOp = wgpu::LoadOp::Clear;
        renderPassDesc.cDepthStencilAttachmentInfo.depthStoreOp = wgpu::StoreOp::Store;
        renderPassDesc.cDepthStencilAttachmentInfo.stencilLoadOp = wgpu::LoadOp::Undefined;
        renderPassDesc.cDepthStencilAttachmentInfo.stencilStoreOp = wgpu::StoreOp::Undefined;
        renderPassDesc.depthStencilAttachment = &renderPassDesc.cDepthStencilAttachmentInfo;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);
        pass.SetPipeline(pipeline);

        // Submit 5120 DrawIndirect calls per frame
        for (uint32_t i = 0; i < kIndirectDrawCount; ++i) {
            pass.DrawIndirect(indirectBuffer, 16 * i);
        }
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
    }

    dawn::InfoLog() << "All frames submitted, waiting for completion...";
    WaitForAllOperations();
    dawn::InfoLog() << "Test completed successfully";
}

DAWN_INSTANTIATE_TEST(DrawIndirectSwapchainSimulationTest, D3D12Backend());

}  // anonymous namespace
}  // namespace dawn
