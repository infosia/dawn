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

#include <array>
#include <limits>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

constexpr uint32_t kRTSize = 1;

class ImmediateDataTest : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ChromiumExperimentalImmediateData};
    }

    uint32_t GetMaxImmediateDataRangeByteSize() {
        if (maxImmediateDataByteSize != std::numeric_limits<uint32_t>::max()) {
            return maxImmediateDataByteSize;
        }
        wgpu::SupportedLimits supportedLimits = {};
        wgpu::DawnExperimentalImmediateDataLimits immediateDataLimits = {};
        supportedLimits.nextInChain = &immediateDataLimits;
        device.GetLimits(&supportedLimits);
        for (auto* chain = supportedLimits.nextInChain; chain; chain = chain->nextInChain) {
            switch (chain->sType) {
                case (wgpu::SType::DawnExperimentalImmediateDataLimits): {
                    auto* t = static_cast<wgpu::DawnExperimentalImmediateDataLimits*>(
                        supportedLimits.nextInChain);
                    maxImmediateDataByteSize = t->maxImmediateDataRangeByteSize;
                    break;
                }
                default:
                    DAWN_UNREACHABLE();
            }
        }
        return maxImmediateDataByteSize;
    }

    void SetUp() override {
        DawnTest::SetUp();

        mVsModule = utils::CreateShaderModule(device, R"(
            @vertex fn main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
                const pos = array(
                    vec2( 1.0, -1.0),
                    vec2(-1.0, -1.0),
                    vec2( 0.0,  1.0),
                );
                return vec4(pos[VertexIndex], 0.0, 1.0);
            })");

        mFsModule = utils::CreateShaderModule(device, R"(
            enable chromium_experimental_push_constant;
            var<push_constant> constants: vec4f;
            // to reuse the same pipeline layout
            @fragment fn main() -> @location(0) vec4f {
                return constants;
            })");

        mCsModule = utils::CreateShaderModule(device, R"(
            enable chromium_experimental_push_constant;
            var<push_constant> constants: vec4u;
            @group(0) @binding(0) var<storage, read_write> output : vec4u;

            @compute @workgroup_size(1, 1, 1)
            fn main() {
                output = constants;
            })");

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = sizeof(uint32_t) * 4;
        bufferDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage;
        mStorageBuffer = device.CreateBuffer(&bufferDesc);
    }

    wgpu::BindGroupLayout CreateBindGroupLayout() {
        wgpu::BindGroupLayoutEntry entries[1];
        entries[0].binding = 0;
        entries[0].visibility = wgpu::ShaderStage::Compute;
        entries[0].buffer.type = wgpu::BufferBindingType::Storage;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = entries;

        return device.CreateBindGroupLayout(&bindGroupLayoutDesc);
    }

    wgpu::PipelineLayout CreatePipelineLayout() {
        wgpu::BindGroupLayout bindGroupLayout = CreateBindGroupLayout();

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        pipelineLayoutDesc.immediateDataRangeByteSize = GetMaxImmediateDataRangeByteSize();
        return device.CreatePipelineLayout(&pipelineLayoutDesc);
    }

    wgpu::RenderPipeline CreateRenderPipeline() {
        wgpu::Buffer dst = utils::CreateBufferFromData<uint32_t>(
            device,
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst,
            {0, 0, 0, 0});

        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = mVsModule;
        pipelineDescriptor.cFragment.module = mFsModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        pipelineDescriptor.layout = CreatePipelineLayout();

        return device.CreateRenderPipeline(&pipelineDescriptor);
    }

    wgpu::ComputePipeline CreateComputePipeline() {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = mCsModule;
        csDesc.layout = CreatePipelineLayout();

        return device.CreateComputePipeline(&csDesc);
    }

    wgpu::BindGroup CreateBindGroup() {
        return utils::MakeBindGroup(device, CreateBindGroupLayout(), {{0, mStorageBuffer}});
    }

    wgpu::ShaderModule mVsModule;
    wgpu::ShaderModule mFsModule;
    wgpu::ShaderModule mCsModule;
    wgpu::Buffer mStorageBuffer;
    uint32_t maxImmediateDataByteSize = std::numeric_limits<uint32_t>::max();
};

// ImmediateData has been uploaded successfully.
TEST_P(ImmediateDataTest, BasicRenderPipeline) {
    wgpu::RenderPipeline pipeline = CreateRenderPipeline();
    utils::BasicRenderPass renderPass = utils::CreateBasicRenderPass(device, kRTSize, kRTSize);

    // rgba8unorm: {0.2, 0.4, 0.6. 1.0} => {51, 102, 153, 255}
    std::array<float, 4> immediateData = {0.2, 0.4, 0.6, 1.0};
    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder renderPassEncoder =
        commandEncoder.BeginRenderPass(&renderPass.renderPassInfo);
    renderPassEncoder.SetImmediateData(0, immediateData.data(),
                                       immediateData.size() * sizeof(uint32_t));
    renderPassEncoder.SetPipeline(CreateRenderPipeline());
    renderPassEncoder.SetBindGroup(0, CreateBindGroup());
    renderPassEncoder.Draw(3);
    renderPassEncoder.End();
    wgpu::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8(51, 102, 153, 255), renderPass.color, 0, 0);
}

// ImmediateData has been uploaded successfully.
TEST_P(ImmediateDataTest, BasicComputePipeline) {
    std::array<uint32_t, 4> immediateData = {25, 128, 240, 255};
    wgpu::CommandEncoder commandEncoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder computePassEncoder = commandEncoder.BeginComputePass();
    computePassEncoder.SetPipeline(CreateComputePipeline());
    computePassEncoder.SetImmediateData(0, immediateData.data(),
                                        immediateData.size() * sizeof(uint32_t));
    computePassEncoder.SetBindGroup(0, CreateBindGroup());
    computePassEncoder.DispatchWorkgroups(1);
    computePassEncoder.End();
    wgpu::CommandBuffer commands = commandEncoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_BUFFER_U32_RANGE_EQ(immediateData.data(), mStorageBuffer, 0, immediateData.size());
}

DAWN_INSTANTIATE_TEST(ImmediateDataTest, VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
