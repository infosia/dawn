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

#include <string>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/Constants.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

// Test fixture specifically for testing the interaction between immediate data (SetImmediateData)
// and storage buffer length tracking. This is critical to ensure that when both immediate
// constants and buffer length data are uploaded to the same GPU buffer, they don't interfere
// with each other across multiple Apply calls with changing pipeline/bind group state.
class ImmediateDataBufferLengthTest : public DawnTest {
  protected:
    void GetRequiredLimits(const dawn::utils::ComboLimits& supported,
                           dawn::utils::ComboLimits& required) override {
        DAWN_TEST_UNSUPPORTED_IF(supported.maxImmediateSize < kDefaultMaxImmediateDataBytes);
        required.maxImmediateSize = kDefaultMaxImmediateDataBytes;
    }

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
                                                 uint32_t numImmediateValues) {
        std::string bufferDeclarations;
        std::string lengthComputation;

        for (uint32_t i = 0; i < numStorageBuffers; i++) {
            bufferDeclarations += "    @group(0) @binding(" + std::to_string(i) +
                                  ") var<storage, read> buffer" + std::to_string(i) +
                                  " : array<f32>;\n";
            lengthComputation += "    result.bufferLengths[" + std::to_string(i) +
                                 "] = arrayLength(&buffer" + std::to_string(i) + ");\n";
        }

        std::string immediateStruct = "struct ImmediateData {\n";
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        immediateStruct += "    }\n";

        std::string immediateAccess;
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateAccess += "    result.immediates[" + std::to_string(i) +
                               "] = immediateData.value" + std::to_string(i) + ";\n";
        }

        // Unified shader with all entry points
        std::string shader = R"(
            enable chromium_experimental_immediate;
            struct ResultData {
                bufferLengths : array<u32, 8>,
                immediates : array<u32, 8>,
            }
            @group(1) @binding(0) var<storage, read_write> result : ResultData;
)" + bufferDeclarations + immediateStruct +
                             R"(
            var<immediate> immediateData: ImmediateData;

            // Shared function for buffer length and immediate data processing
            fn processData() {
)" + lengthComputation + immediateAccess +
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

    // Helper to create compute pipelines with different numbers of immediate constants and storage
    // buffers
    wgpu::ComputePipeline MakePipelineWithImmediateAndBuffers(uint32_t numStorageBuffers,
                                                              uint32_t numImmediateValues) {
        wgpu::ShaderModule shaderModule =
            CreateUnifiedShaderModule(numStorageBuffers, numImmediateValues);

        wgpu::BindGroupLayout storageLayout = MakeStorageBindGroupLayout(numStorageBuffers);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for compute pipelines
        plDesc.immediateSize = kDefaultMaxImmediateDataBytes;

        wgpu::PipelineLayout layout = device.CreatePipelineLayout(&plDesc);

        wgpu::ComputePipelineDescriptor pipelineDesc;
        pipelineDesc.layout = layout;
        pipelineDesc.compute.module = shaderModule;
        pipelineDesc.compute.entryPoint = "csMain";
        return device.CreateComputePipeline(&pipelineDesc);
    }

    // Helper to create render pipelines with different numbers of immediate constants and storage
    // buffers
    wgpu::RenderPipeline MakeRenderPipelineWithImmediateAndBuffers(uint32_t numStorageBuffers,
                                                                   uint32_t numImmediateValues) {
        wgpu::ShaderModule shaderModule =
            CreateUnifiedShaderModule(numStorageBuffers, numImmediateValues);

        wgpu::BindGroupLayout storageLayout =
            MakeStorageBindGroupLayoutForRender(numStorageBuffers);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for render pipelines
        plDesc.immediateSize = kDefaultMaxImmediateDataBytes;

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

    // Helper to create multiple result buffers for render pipelines (one per draw call)
    std::vector<wgpu::Buffer> CreateMultipleResultBuffersForRender(uint32_t numDrawCalls) {
        std::vector<wgpu::Buffer> resultBuffers;
        for (uint32_t i = 0; i < numDrawCalls; i++) {
            wgpu::BufferDescriptor bufferDesc;
            bufferDesc.size = 256;
            bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
            resultBuffers.push_back(device.CreateBuffer(&bufferDesc));
        }
        return resultBuffers;
    }

    // Helper to create individual result bind group for a specific result buffer
    wgpu::BindGroup MakeResultBindGroupForRenderWithBuffer(wgpu::Buffer resultBuffer) {
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, resultBuffer}});
    }

    // Helper to validate results from multiple individual result buffers
    void ValidateIndividualDrawResults(const std::vector<wgpu::Buffer>& resultBuffers,
                                       const std::vector<std::vector<uint32_t>>& expectedResults) {
        ASSERT_EQ(resultBuffers.size(), expectedResults.size());

        for (size_t i = 0; i < resultBuffers.size(); i++) {
            std::string message = "Draw call " + std::to_string(i);
            EXPECT_BUFFER_U32_RANGE_EQ(expectedResults[i].data(), resultBuffers[i], 0,
                                       expectedResults[i].size())
                << message;
        }
    }

    wgpu::Buffer mStorageBuffer64;
    wgpu::Buffer mStorageBuffer256;
    wgpu::Buffer mStorageBuffer512;
    wgpu::Buffer mResultBuffer;
    wgpu::Texture mRenderTarget;
    wgpu::TextureView mRenderTargetView;
};

// === COMPUTE PIPELINE TESTS ===

// Test: SetBindGroup + SetImmediateData + Draw
// This is the basic case to ensure immediate data and buffer lengths work together
TEST_P(ImmediateDataBufferLengthTest, ComputePipelineBasicImmediateAndBufferLength) {
    wgpu::ComputePipeline pipeline = MakePipelineWithImmediateAndBuffers(2, 3);
    wgpu::BindGroup storageBindGroup = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data - struct with 3 u32 values
    uint32_t immediateData[] = {42, 123, 456};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData), sizeof(immediateData));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and immediate data [42, 123, 456]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64 has 64 bytes = 16 f32 elements
    expected[1] = 64;    // mStorageBuffer256 has 256 bytes = 64 f32 elements
    expected[8] = 42;    // immediate.value0
    expected[9] = 123;   // immediate.value1
    expected[10] = 456;  // immediate.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: SetBindGroup + SetImmediateData + Draw + SetPipeline (different immediates) + Draw
// This tests that changing pipelines with different immediate counts still preserves buffer lengths
TEST_P(ImmediateDataBufferLengthTest, ComputePipelinePipelineChangeWithDifferentImmediates) {
    wgpu::ComputePipeline pipeline1 = MakePipelineWithImmediateAndBuffers(2, 2);  // 2 immediates
    wgpu::ComputePipeline pipeline2 = MakePipelineWithImmediateAndBuffers(2, 4);  // 4 immediates
    wgpu::BindGroup storageBindGroup = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with pipeline1
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {100, 200};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.DispatchWorkgroups(1);

    // Change to pipeline2 with more immediates - buffer lengths should still be correct
    pass.SetPipeline(pipeline2);
    uint32_t immediateData2[] = {300, 400, 500, 600};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and immediate data [300, 400, 500, 600] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 300;   // immediate.value0
    expected[9] = 400;   // immediate.value1
    expected[10] = 500;  // immediate.value2
    expected[11] = 600;  // immediate.value3

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: SetBindGroup + SetImmediateData + Draw + SetPipeline (different buffer count) +
// SetBindGroup + Draw This tests changing both pipeline and bind group, ensuring buffer length
// tracking adapts correctly
TEST_P(ImmediateDataBufferLengthTest, ComputePipelinePipelineAndBindGroupChange) {
    wgpu::ComputePipeline pipeline1 =
        MakePipelineWithImmediateAndBuffers(2, 2);  // 2 buffers, 2 immediates
    wgpu::ComputePipeline pipeline2 =
        MakePipelineWithImmediateAndBuffers(3, 2);  // 3 buffers, 2 immediates

    wgpu::BindGroup storageBindGroup1 = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup3Buffers(MakeStorageBindGroupLayout(3));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with 2 buffers
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {111, 222};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.DispatchWorkgroups(1);

    // Change to pipeline with 3 buffers and update bind group
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t immediateData2[] = {333, 444};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64, 128] and immediate data [333, 444] from second draw
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;   // mStorageBuffer64
    expected[1] = 64;   // mStorageBuffer256
    expected[2] = 128;  // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expected[8] = 333;  // immediate.value0
    expected[9] = 444;  // immediate.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Multiple SetImmediateData calls with buffer length updates
// This tests the edge case where immediate data is updated multiple times
TEST_P(ImmediateDataBufferLengthTest, ComputePipelineMultipleImmediateDataUpdates) {
    wgpu::ComputePipeline pipeline = MakePipelineWithImmediateAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup = MakeBindGroup1Buffer(MakeStorageBindGroupLayout(1));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data multiple times
    uint32_t immediateData1[] = {10, 20, 30};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    uint32_t immediateData2[] = {40, 50, 60};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    uint32_t immediateData3[] = {70, 80, 90};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData3), sizeof(immediateData3));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final immediate data [70, 80, 90]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;   // mStorageBuffer64
    expected[8] = 70;   // final immediate.value0
    expected[9] = 80;   // final immediate.value1
    expected[10] = 90;  // final immediate.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Interleaved SetBindGroup and SetImmediateData calls
// This tests complex state management scenarios
TEST_P(ImmediateDataBufferLengthTest, ComputePipelineInterleavedUpdates) {
    wgpu::ComputePipeline pipeline = MakePipelineWithImmediateAndBuffers(2, 2);
    wgpu::BindGroup storageBindGroup1 = MakeBindGroup2Buffers(MakeStorageBindGroupLayout(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup2Buffers(
        MakeStorageBindGroupLayout(2));  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and immediate data updates
    pass.SetBindGroup(0, storageBindGroup1);
    uint32_t immediateData1[] = {1000, 2000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    pass.SetBindGroup(0, storageBindGroup2);
    uint32_t immediateData2[] = {3000, 4000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final immediate data [3000, 4000]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 3000;  // final immediate.value0
    expected[9] = 4000;  // final immediate.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// === RENDER PIPELINE TESTS ===

// Test: Render pipeline with SetBindGroup + SetImmediateData + Draw
// This tests the basic case for render pipelines
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineBasicImmediateAndBufferLength) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithImmediateAndBuffers(2, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data - struct with 3 u32 values
    uint32_t immediateData[] = {789, 987, 654};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData), sizeof(immediateData));

    pass.Draw(1);  // Draw 1 point
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and immediate data [789, 987, 654]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64 has 64 bytes = 16 f32 elements
    expected[1] = 64;    // mStorageBuffer256 has 256 bytes = 64 f32 elements
    expected[8] = 789;   // immediate.value0
    expected[9] = 987;   // immediate.value1
    expected[10] = 654;  // immediate.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with multiple draws and pipeline changes (updated to use individual result
// buffers)
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineChangeWithDifferentImmediates) {
    const uint32_t numDrawCalls = 2;

    // Create multiple result buffers - one for each draw call
    std::vector<wgpu::Buffer> resultBuffers = CreateMultipleResultBuffersForRender(numDrawCalls);

    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 2);  // 2 immediates
    wgpu::RenderPipeline pipeline2 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 4);  // 4 immediates
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1 - writes to result buffer 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    wgpu::BindGroup resultBindGroup0 = MakeResultBindGroupForRenderWithBuffer(resultBuffers[0]);
    pass.SetBindGroup(1, resultBindGroup0);

    uint32_t immediateData1[] = {1100, 1200};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Second draw with pipeline2 - writes to result buffer 1
    pass.SetPipeline(pipeline2);
    wgpu::BindGroup resultBindGroup1 = MakeResultBindGroupForRenderWithBuffer(resultBuffers[1]);
    pass.SetBindGroup(1, resultBindGroup1);

    uint32_t immediateData2[] = {1300, 1400, 1500, 1600};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each draw call with different expectations
    std::vector<std::vector<uint32_t>> expectedResults(numDrawCalls);

    // Draw call 0 expectations (2 immediates)
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;    // mStorageBuffer64
    expectedResults[0][1] = 64;    // mStorageBuffer256
    expectedResults[0][8] = 1100;  // immediate.value0
    expectedResults[0][9] = 1200;  // immediate.value1

    // Draw call 1 expectations (4 immediates)
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;     // mStorageBuffer64
    expectedResults[1][1] = 64;     // mStorageBuffer256
    expectedResults[1][8] = 1300;   // immediate.value0
    expectedResults[1][9] = 1400;   // immediate.value1
    expectedResults[1][10] = 1500;  // immediate.value2
    expectedResults[1][11] = 1600;  // immediate.value3

    ValidateIndividualDrawResults(resultBuffers, expectedResults);
}

// Test: Render pipeline with buffer count changes (updated to use individual result buffers)
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineAndBindGroupChange) {
    const uint32_t numDrawCalls = 2;

    // Create multiple result buffers - one for each draw call
    std::vector<wgpu::Buffer> resultBuffers = CreateMultipleResultBuffersForRender(numDrawCalls);

    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 2);  // 2 buffers, 2 immediates
    wgpu::RenderPipeline pipeline2 =
        MakeRenderPipelineWithImmediateAndBuffers(3, 2);  // 3 buffers, 2 immediates

    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroup3BuffersForRender(MakeStorageBindGroupLayoutForRender(3));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with 2 buffers - writes to result buffer 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    wgpu::BindGroup resultBindGroup0 = MakeResultBindGroupForRenderWithBuffer(resultBuffers[0]);
    pass.SetBindGroup(1, resultBindGroup0);

    uint32_t immediateData1[] = {2111, 2222};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Second draw with 3 buffers - writes to result buffer 1
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers
    wgpu::BindGroup resultBindGroup1 = MakeResultBindGroupForRenderWithBuffer(resultBuffers[1]);
    pass.SetBindGroup(1, resultBindGroup1);

    uint32_t immediateData2[] = {2333, 2444};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each draw call with different buffer configurations
    std::vector<std::vector<uint32_t>> expectedResults(numDrawCalls);

    // Draw call 0 expectations (2 buffers)
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;    // mStorageBuffer64
    expectedResults[0][1] = 64;    // mStorageBuffer256
    expectedResults[0][8] = 2111;  // immediate.value0
    expectedResults[0][9] = 2222;  // immediate.value1

    // Draw call 1 expectations (3 buffers)
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;    // mStorageBuffer64
    expectedResults[1][1] = 64;    // mStorageBuffer256
    expectedResults[1][2] = 128;   // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expectedResults[1][8] = 2333;  // immediate.value0
    expectedResults[1][9] = 2444;  // immediate.value1

    ValidateIndividualDrawResults(resultBuffers, expectedResults);
}

// Test: Render pipeline with multiple immediate data updates
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineMultipleImmediateDataUpdates) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithImmediateAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup1BufferForRender(MakeStorageBindGroupLayoutForRender(1));
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data multiple times
    uint32_t immediateData1[] = {3010, 3020, 3030};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    uint32_t immediateData2[] = {3040, 3050, 3060};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    uint32_t immediateData3[] = {3070, 3080, 3090};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData3), sizeof(immediateData3));

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final immediate data [3070, 3080, 3090]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;     // mStorageBuffer64
    expected[8] = 3070;   // final immediate.value0
    expected[9] = 3080;   // final immediate.value1
    expected[10] = 3090;  // final immediate.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with interleaved updates
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineInterleavedUpdates) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithImmediateAndBuffers(2, 2);
    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup storageBindGroup2 = MakeBindGroup2BuffersForRender(
        MakeStorageBindGroupLayoutForRender(2));  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroupForRender();

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and immediate data updates
    pass.SetBindGroup(0, storageBindGroup1);
    uint32_t immediateData1[] = {4000, 5000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    pass.SetBindGroup(0, storageBindGroup2);
    uint32_t immediateData2[] = {6000, 7000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final immediate data [6000, 7000]
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 6000;  // final immediate.value0
    expected[9] = 7000;  // final immediate.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

DAWN_INSTANTIATE_TEST(ImmediateDataBufferLengthTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
