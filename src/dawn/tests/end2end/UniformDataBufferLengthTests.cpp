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

#include <algorithm>
#include <string>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/Constants.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

// Test fixture specifically for testing the interaction between uniform data (WriteBuffer)
// and storage buffer length tracking. This is critical to ensure that when both uniform
// constants and buffer length data are uploaded to the same GPU buffer, they don't interfere
// with each other across multiple Apply calls with changing pipeline/bind group state.
class UniformDataBufferLengthTest : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        // Create storage buffers of different sizes to test buffer length tracking
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.usage = wgpu::BufferUsage::Storage;

        bufferDesc.size = 64;  // 16 elements
        mStorageBuffer64 = device.CreateBuffer(&bufferDesc);

        bufferDesc.size = 256;  // 64 elements
        mStorageBuffer256 = device.CreateBuffer(&bufferDesc);

        bufferDesc.size = 512;  // 128 elements
        mStorageBuffer512 = device.CreateBuffer(&bufferDesc);

        // Create a result buffer for reading back computed values
        bufferDesc.size = 256;
        bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        mResultBuffer = device.CreateBuffer(&bufferDesc);

        // Create render target for render pipeline tests
        wgpu::TextureDescriptor textureDesc;
        textureDesc.size = {1, 1, 1};
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment;
        mRenderTarget = device.CreateTexture(&textureDesc);
        mRenderTargetView = mRenderTarget.CreateView();
    }

    // Helper to create a bind group layout with storage buffers
    wgpu::BindGroupLayout MakeStorageBindGroupLayout(uint32_t numBuffers) {
        switch (numBuffers) {
            case 1:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 2:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 3:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                     {2, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage}});
            default:
                DAWN_UNREACHABLE();
                return {};
        }
    }

    // Helper to create bind groups with different buffer configurations
    wgpu::BindGroup MakeBindGroup1Buffer(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(device, layout, {{0, mStorageBuffer64}});
    }

    wgpu::BindGroup MakeBindGroup2Buffers(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(device, layout,
                                    {{0, mStorageBuffer64}, {1, mStorageBuffer256}});
    }

    wgpu::BindGroup MakeBindGroup3Buffers(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(
            device, layout,
            {{0, mStorageBuffer64}, {1, mStorageBuffer256}, {2, mStorageBuffer512}});
    }

    // Helper to create unified shader with compute, vertex, and fragment entry points
    wgpu::ShaderModule CreateUnifiedShaderModule(uint32_t numStorageBuffers,
                                                 uint32_t numUniformValues) {
        std::string bufferDeclarations;
        std::string lengthComputation;

        for (uint32_t i = 0; i < numStorageBuffers; i++) {
            bufferDeclarations += "    @group(0) @binding(" + std::to_string(i) +
                                  ") var<storage, read> buffer" + std::to_string(i) +
                                  " : array<f32>;\n";
            lengthComputation += "    result.bufferLengths[" + std::to_string(i) +
                                 "] = arrayLength(&buffer" + std::to_string(i) + ");\n";
        }

        std::string uniformStruct = "struct UniformData {\n";
        for (uint32_t i = 0; i < numUniformValues; i++) {
            uniformStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        uniformStruct += "    }\n";

        std::string uniformAccess;
        for (uint32_t i = 0; i < numUniformValues; i++) {
            uniformAccess += "    result.uniforms[" + std::to_string(i) + "] = uniformData.value" +
                             std::to_string(i) + ";\n";
        }

        // Unified shader with all entry points
        std::string shader = R"(
            struct ResultData {
                bufferLengths : array<u32, 8>,
                uniforms : array<u32, 8>,
            }
            @group(1) @binding(0) var<storage, read_write> result : ResultData;
)" + bufferDeclarations + uniformStruct +
                             R"(
            @group(2) @binding(0) var<uniform> uniformData: UniformData;

            // Shared function for buffer length and uniform data processing
            fn processData() {
)" + lengthComputation + uniformAccess +
                             R"(
            }

            // Compute entry point
            @compute @workgroup_size(1) fn csMain() {
                processData();
            }

            // Vertex entry point
            @vertex fn vsMain() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 1.0);
            }

            // Fragment entry point
            @fragment fn fsMain() -> @location(0) vec4f {
                processData();
                return vec4f(1.0, 0.0, 0.0, 1.0);
            }
        )";

        return utils::CreateShaderModule(device, shader.c_str());
    }

    // Helper to create uniform buffers for different numbers of values
    wgpu::Buffer CreateUniformBuffer(uint32_t numValues) {
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = sizeof(uint32_t) * std::max(1u, numValues);  // At least 1 uint32_t
        bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        return device.CreateBuffer(&bufferDesc);
    }

    // Helper to create compute pipelines with different numbers of uniform constants and storage
    // buffers
    wgpu::ComputePipeline MakePipelineWithUniformAndBuffers(uint32_t numStorageBuffers,
                                                            uint32_t numUniformValues) {
        wgpu::ShaderModule shaderModule =
            CreateUnifiedShaderModule(numStorageBuffers, numUniformValues);

        wgpu::BindGroupLayout storageLayout = MakeStorageBindGroupLayout(numStorageBuffers);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});
        wgpu::BindGroupLayout uniformLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout, uniformLayout};
        plDesc.bindGroupLayoutCount = 3;
        plDesc.bindGroupLayouts = layouts;

        wgpu::PipelineLayout layout = device.CreatePipelineLayout(&plDesc);

        wgpu::ComputePipelineDescriptor pipelineDesc;
        pipelineDesc.layout = layout;
        pipelineDesc.compute.module = shaderModule;
        pipelineDesc.compute.entryPoint = "csMain";
        return device.CreateComputePipeline(&pipelineDesc);
    }

    // Helper to create render pipelines with different numbers of uniform constants and storage
    // buffers
    wgpu::RenderPipeline MakeRenderPipelineWithUniformAndBuffers(uint32_t numStorageBuffers,
                                                                 uint32_t numUniformValues) {
        wgpu::ShaderModule shaderModule =
            CreateUnifiedShaderModule(numStorageBuffers, numUniformValues);

        wgpu::BindGroupLayout storageLayout =
            MakeStorageBindGroupLayoutForRender(numStorageBuffers);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
        wgpu::BindGroupLayout uniformLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout, uniformLayout};
        plDesc.bindGroupLayoutCount = 3;
        plDesc.bindGroupLayouts = layouts;

        wgpu::PipelineLayout layout = device.CreatePipelineLayout(&plDesc);

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.layout = layout;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vsMain";
        pipelineDesc.cFragment.module = shaderModule;
        pipelineDesc.cFragment.entryPoint = "fsMain";
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;
        pipelineDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;

        return device.CreateRenderPipeline(&pipelineDesc);
    }

    // Helper to create a bind group layout with storage buffers for render pipelines (fragment
    // stage)
    wgpu::BindGroupLayout MakeStorageBindGroupLayoutForRender(uint32_t numBuffers) {
        switch (numBuffers) {
            case 1:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 2:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 3:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
                     {2, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}});
            default:
                DAWN_UNREACHABLE();
                return {};
        }
    }

    // Helper to create bind groups for render pipelines
    wgpu::BindGroup MakeBindGroup1BufferForRender(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(device, layout, {{0, mStorageBuffer64}});
    }

    wgpu::BindGroup MakeBindGroup2BuffersForRender(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(device, layout,
                                    {{0, mStorageBuffer64}, {1, mStorageBuffer256}});
    }

    wgpu::BindGroup MakeBindGroup3BuffersForRender(wgpu::BindGroupLayout layout) {
        return utils::MakeBindGroup(
            device, layout,
            {{0, mStorageBuffer64}, {1, mStorageBuffer256}, {2, mStorageBuffer512}});
    }

    wgpu::BindGroup MakeResultBindGroup() {
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, mResultBuffer}});
    }

    wgpu::BindGroup MakeResultBindGroupForRender() {
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, mResultBuffer}});
    }

    wgpu::BindGroup MakeUniformBindGroup(wgpu::Buffer uniformBuffer, wgpu::ShaderStage stage) {
        wgpu::BindGroupLayout uniformLayout =
            utils::MakeBindGroupLayout(device, {{0, stage, wgpu::BufferBindingType::Uniform}});
        return utils::MakeBindGroup(device, uniformLayout, {{0, uniformBuffer}});
    }

    wgpu::Buffer mStorageBuffer64;
    wgpu::Buffer mStorageBuffer256;
    wgpu::Buffer mStorageBuffer512;
    wgpu::Buffer mResultBuffer;
    wgpu::Texture mRenderTarget;
    wgpu::TextureView mRenderTargetView;
};

// === COMPUTE PIPELINE TESTS ===

// Test: SetBindGroup + WriteBuffer + Dispatch
// This is the basic case to ensure uniform data and buffer lengths work together
TEST_P(UniformDataBufferLengthTest, ComputePipelineBasicUniformAndBufferLength) {
    wgpu::ComputePipeline pipeline = MakePipelineWithUniformAndBuffers(2, 3);
    wgpu::BindGroup storageBindGroup = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(3);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Compute);

    // Set uniform data - struct with 3 u32 values
    uint32_t uniformData[] = {42, 123, 456};
    queue.WriteBuffer(uniformBuffer, 0, uniformData, sizeof(uniformData));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and uniform data [42, 123, 456]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64 has 64 bytes = 16 f32 elements
    expected[1] = 64;    // mStorageBuffer256 has 256 bytes = 64 f32 elements
    expected[8] = 42;    // uniform.value0
    expected[9] = 123;   // uniform.value1
    expected[10] = 456;  // uniform.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: SetBindGroup + WriteBuffer + Dispatch + SetPipeline (different uniforms) + Dispatch
// This tests that changing pipelines with different uniform counts still preserves buffer lengths
TEST_P(UniformDataBufferLengthTest, ComputePipelinePipelineChangeWithDifferentUniforms) {
    wgpu::ComputePipeline pipeline1 = MakePipelineWithUniformAndBuffers(2, 2);  // 2 uniforms
    wgpu::ComputePipeline pipeline2 = MakePipelineWithUniformAndBuffers(2, 4);  // 4 uniforms
    wgpu::BindGroup storageBindGroup = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::Buffer uniformBuffer1 = CreateUniformBuffer(2);
    wgpu::Buffer uniformBuffer2 = CreateUniformBuffer(4);
    wgpu::BindGroup uniformBindGroup1 =
        MakeUniformBindGroup(uniformBuffer1, wgpu::ShaderStage::Compute);
    wgpu::BindGroup uniformBindGroup2 =
        MakeUniformBindGroup(uniformBuffer2, wgpu::ShaderStage::Compute);

    uint32_t uniformData1[] = {100, 200};
    queue.WriteBuffer(uniformBuffer1, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {300, 400, 500, 600};
    queue.WriteBuffer(uniformBuffer2, 0, uniformData2, sizeof(uniformData2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with pipeline1
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup1);
    pass.DispatchWorkgroups(1);

    // Change to pipeline2 with more uniforms - buffer lengths should still be correct
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(2, uniformBindGroup2);
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and uniform data [300, 400, 500, 600] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 300;   // uniform.value0
    expected[9] = 400;   // uniform.value1
    expected[10] = 500;  // uniform.value2
    expected[11] = 600;  // uniform.value3

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: SetBindGroup + WriteBuffer + Dispatch + SetPipeline (different buffer count) + SetBindGroup
// + Dispatch This tests changing both pipeline and bind group, ensuring buffer length tracking
// adapts correctly
TEST_P(UniformDataBufferLengthTest, ComputePipelinePipelineAndBindGroupChange) {
    wgpu::ComputePipeline pipeline1 =
        MakePipelineWithUniformAndBuffers(2, 2);  // 2 buffers, 2 uniforms
    wgpu::ComputePipeline pipeline2 =
        MakePipelineWithUniformAndBuffers(3, 2);  // 3 buffers, 2 uniforms

    wgpu::BindGroup storageBindGroup1 = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup3Buffers(MakeStorageBindGroupLayout(3));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(2);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Compute);

    uint32_t uniformData1[] = {111, 222};
    queue.WriteBuffer(uniformBuffer, 0, uniformData1, sizeof(uniformData1));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with 2 buffers
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);
    pass.DispatchWorkgroups(1);

    // Change to pipeline with 3 buffers and update bind group
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t uniformData2[] = {333, 444};
    queue.WriteBuffer(uniformBuffer, 0, uniformData2, sizeof(uniformData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64, 128] and uniform data [333, 444] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;   // mStorageBuffer64
    expected[1] = 64;   // mStorageBuffer256
    expected[2] = 128;  // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expected[8] = 333;  // uniform.value0
    expected[9] = 444;  // uniform.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Multiple WriteBuffer calls with buffer length updates
// This tests the edge case where uniform data is updated multiple times
TEST_P(UniformDataBufferLengthTest, ComputePipelineMultipleUniformDataUpdates) {
    wgpu::ComputePipeline pipeline = MakePipelineWithUniformAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup = MakeBindGroup1Buffer(MakeStorageBindGroupLayout(1));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(3);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Compute);

    // Set uniform data multiple times - only the last one should take effect
    uint32_t uniformData1[] = {10, 20, 30};
    queue.WriteBuffer(uniformBuffer, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {40, 50, 60};
    queue.WriteBuffer(uniformBuffer, 0, uniformData2, sizeof(uniformData2));

    uint32_t uniformData3[] = {70, 80, 90};
    queue.WriteBuffer(uniformBuffer, 0, uniformData3, sizeof(uniformData3));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final uniform data [70, 80, 90]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;   // mStorageBuffer64
    expected[8] = 70;   // final uniform.value0
    expected[9] = 80;   // final uniform.value1
    expected[10] = 90;  // final uniform.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Interleaved SetBindGroup and WriteBuffer calls
// This tests complex state management scenarios
TEST_P(UniformDataBufferLengthTest, ComputePipelineInterleavedUpdates) {
    wgpu::ComputePipeline pipeline = MakePipelineWithUniformAndBuffers(2, 2);
    wgpu::BindGroup storageBindGroup1 = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup2Buffers(
        MakeStorageBindGroupLayout(2));  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::Buffer uniformBuffer1 = CreateUniformBuffer(2);
    wgpu::Buffer uniformBuffer2 = CreateUniformBuffer(2);
    wgpu::BindGroup uniformBindGroup1 =
        MakeUniformBindGroup(uniformBuffer1, wgpu::ShaderStage::Compute);
    wgpu::BindGroup uniformBindGroup2 =
        MakeUniformBindGroup(uniformBuffer2, wgpu::ShaderStage::Compute);

    uint32_t uniformData1[] = {1000, 2000};
    queue.WriteBuffer(uniformBuffer1, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {3000, 4000};
    queue.WriteBuffer(uniformBuffer2, 0, uniformData2, sizeof(uniformData2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and uniform data updates
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(2, uniformBindGroup1);

    pass.SetBindGroup(0, storageBindGroup2);
    pass.SetBindGroup(2, uniformBindGroup2);

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final uniform data [3000, 4000]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 3000;  // final uniform.value0
    expected[9] = 4000;  // final uniform.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// === RENDER PIPELINE TESTS ===

// Test: Render pipeline with SetBindGroup + WriteBuffer + Draw
// This tests the basic case for render pipelines
TEST_P(UniformDataBufferLengthTest, RenderPipelineBasicUniformAndBufferLength) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithUniformAndBuffers(2, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(3);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Fragment);

    // Set uniform data - struct with 3 u32 values
    uint32_t uniformData[] = {789, 987, 654};
    queue.WriteBuffer(uniformBuffer, 0, uniformData, sizeof(uniformData));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);

    pass.Draw(1);  // Draw 1 point
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and uniform data [789, 987, 654]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64 has 64 bytes = 16 f32 elements
    expected[1] = 64;    // mStorageBuffer256 has 256 bytes = 64 f32 elements
    expected[8] = 789;   // uniform.value0
    expected[9] = 987;   // uniform.value1
    expected[10] = 654;  // uniform.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with multiple draws and pipeline changes
TEST_P(UniformDataBufferLengthTest, RenderPipelinePipelineChangeWithDifferentUniforms) {
    wgpu::RenderPipeline pipeline1 = MakeRenderPipelineWithUniformAndBuffers(2, 2);  // 2 uniforms
    wgpu::RenderPipeline pipeline2 = MakeRenderPipelineWithUniformAndBuffers(2, 4);  // 4 uniforms
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::Buffer uniformBuffer1 = CreateUniformBuffer(2);
    wgpu::Buffer uniformBuffer2 = CreateUniformBuffer(4);
    wgpu::BindGroup uniformBindGroup1 =
        MakeUniformBindGroup(uniformBuffer1, wgpu::ShaderStage::Fragment);
    wgpu::BindGroup uniformBindGroup2 =
        MakeUniformBindGroup(uniformBuffer2, wgpu::ShaderStage::Fragment);

    uint32_t uniformData1[] = {1100, 1200};
    queue.WriteBuffer(uniformBuffer1, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {1300, 1400, 1500, 1600};
    queue.WriteBuffer(uniformBuffer2, 0, uniformData2, sizeof(uniformData2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup1);
    pass.Draw(1);

    // Change to pipeline2 with more uniforms - buffer lengths should still be correct
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(2, uniformBindGroup2);
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and uniform data [1300, 1400, 1500, 1600] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;     // mStorageBuffer64
    expected[1] = 64;     // mStorageBuffer256
    expected[8] = 1300;   // uniform.value0
    expected[9] = 1400;   // uniform.value1
    expected[10] = 1500;  // uniform.value2
    expected[11] = 1600;  // uniform.value3

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with buffer count changes
TEST_P(UniformDataBufferLengthTest, RenderPipelinePipelineAndBindGroupChange) {
    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithUniformAndBuffers(2, 2);  // 2 buffers, 2 uniforms
    wgpu::RenderPipeline pipeline2 =
        MakeRenderPipelineWithUniformAndBuffers(3, 2);  // 3 buffers, 2 uniforms

    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroup3BuffersForRender(MakeStorageBindGroupLayoutForRender(3));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(2);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Fragment);

    uint32_t uniformData1[] = {2111, 2222};
    queue.WriteBuffer(uniformBuffer, 0, uniformData1, sizeof(uniformData1));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with 2 buffers
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);
    pass.Draw(1);

    // Change to pipeline with 3 buffers and update bind group
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t uniformData2[] = {2333, 2444};
    queue.WriteBuffer(uniformBuffer, 0, uniformData2, sizeof(uniformData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64, 128] and uniform data [2333, 2444] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[2] = 128;   // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expected[8] = 2333;  // uniform.value0
    expected[9] = 2444;  // uniform.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with multiple uniform data updates
TEST_P(UniformDataBufferLengthTest, RenderPipelineMultipleUniformDataUpdates) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithUniformAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup1BufferForRender(MakeStorageBindGroupLayoutForRender(1));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::Buffer uniformBuffer = CreateUniformBuffer(3);
    wgpu::BindGroup uniformBindGroup =
        MakeUniformBindGroup(uniformBuffer, wgpu::ShaderStage::Fragment);

    // Set uniform data multiple times
    uint32_t uniformData1[] = {3010, 3020, 3030};
    queue.WriteBuffer(uniformBuffer, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {3040, 3050, 3060};
    queue.WriteBuffer(uniformBuffer, 0, uniformData2, sizeof(uniformData2));

    uint32_t uniformData3[] = {3070, 3080, 3090};
    queue.WriteBuffer(uniformBuffer, 0, uniformData3, sizeof(uniformData3));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);
    pass.SetBindGroup(2, uniformBindGroup);

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final uniform data [3070, 3080, 3090]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;     // mStorageBuffer64
    expected[8] = 3070;   // final uniform.value0
    expected[9] = 3080;   // final uniform.value1
    expected[10] = 3090;  // final uniform.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with interleaved updates
TEST_P(UniformDataBufferLengthTest, RenderPipelineInterleavedUpdates) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithUniformAndBuffers(2, 2);
    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup2BuffersForRender(
        MakeStorageBindGroupLayoutForRender(2));  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::Buffer uniformBuffer1 = CreateUniformBuffer(2);
    wgpu::Buffer uniformBuffer2 = CreateUniformBuffer(2);
    wgpu::BindGroup uniformBindGroup1 =
        MakeUniformBindGroup(uniformBuffer1, wgpu::ShaderStage::Fragment);
    wgpu::BindGroup uniformBindGroup2 =
        MakeUniformBindGroup(uniformBuffer2, wgpu::ShaderStage::Fragment);

    uint32_t uniformData1[] = {4000, 5000};
    queue.WriteBuffer(uniformBuffer1, 0, uniformData1, sizeof(uniformData1));

    uint32_t uniformData2[] = {6000, 7000};
    queue.WriteBuffer(uniformBuffer2, 0, uniformData2, sizeof(uniformData2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and uniform data updates
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(2, uniformBindGroup1);

    pass.SetBindGroup(0, storageBindGroup2);
    pass.SetBindGroup(2, uniformBindGroup2);

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final uniform data [6000, 7000]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 6000;  // final uniform.value0
    expected[9] = 7000;  // final uniform.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

DAWN_INSTANTIATE_TEST(UniformDataBufferLengthTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
