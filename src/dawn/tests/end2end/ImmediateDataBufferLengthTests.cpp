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

        // Create a result buffer for reading back computed values (sized for multiple draw calls)
        bufferDesc.size =
            8 * (8 + 8) * sizeof(uint32_t);  // 8 entries, each with 8 buffer lengths + 8 immediates
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

    // Helper to create a bind group layout with storage buffers for any shader stage
    wgpu::BindGroupLayout MakeStorageBindGroupLayout(uint32_t numBuffers, wgpu::ShaderStage stage) {
        switch (numBuffers) {
            case 1:
                return utils::MakeBindGroupLayout(
                    device, {{0, stage, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 2:
                return utils::MakeBindGroupLayout(
                    device, {{0, stage, wgpu::BufferBindingType::ReadOnlyStorage},
                             {1, stage, wgpu::BufferBindingType::ReadOnlyStorage}});
            case 3:
                return utils::MakeBindGroupLayout(
                    device, {{0, stage, wgpu::BufferBindingType::ReadOnlyStorage},
                             {1, stage, wgpu::BufferBindingType::ReadOnlyStorage},
                             {2, stage, wgpu::BufferBindingType::ReadOnlyStorage}});
            default:
                DAWN_UNREACHABLE();
                return {};
        }
    }

    // Helper to create bind groups with different buffer configurations
    wgpu::BindGroup MakeBindGroupWithBuffers(wgpu::BindGroupLayout layout, uint32_t numBuffers) {
        switch (numBuffers) {
            case 1:
                return utils::MakeBindGroup(device, layout, {{0, mStorageBuffer64}});
            case 2:
                return utils::MakeBindGroup(device, layout,
                                            {{0, mStorageBuffer64}, {1, mStorageBuffer256}});
            case 3:
                return utils::MakeBindGroup(
                    device, layout,
                    {{0, mStorageBuffer64}, {1, mStorageBuffer256}, {2, mStorageBuffer512}});
            default:
                DAWN_UNREACHABLE();
                return {};
        }
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
            lengthComputation += "    result[drawCallIndex].bufferLengths[" + std::to_string(i) +
                                 "] = arrayLength(&buffer" + std::to_string(i) + ");\n";
        }

        std::string immediateStruct = "struct ImmediateData {\n";
        immediateStruct += "        drawCallIndex : u32,\n";  // Add draw call index as first field
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        immediateStruct += "    }\n";

        std::string immediateAccess;
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateAccess += "    result[drawCallIndex].immediates[" + std::to_string(i) +
                               "] = immediateData.value" + std::to_string(i) + ";\n";
        }

        // Unified shader with all entry points
        std::string shader = R"(
            enable chromium_experimental_immediate;
            struct ResultData {
                bufferLengths : array<u32, 8>,
                immediates : array<u32, 8>,
            }
            @group(1) @binding(0) var<storage, read_write> result : array<ResultData, 8>;
)" + bufferDeclarations + immediateStruct +
                             R"(
            var<immediate> immediateData: ImmediateData;

            // Shared function for buffer length and immediate data processing
            fn processData() {
                let drawCallIndex = immediateData.drawCallIndex;
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

        wgpu::BindGroupLayout storageLayout =
            MakeStorageBindGroupLayout(numStorageBuffers, wgpu::ShaderStage::Compute);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for compute pipelines - calculate exact size needed
        // +1 for drawCallIndex parameter
        plDesc.immediateSize = (numImmediateValues + 1) * sizeof(uint32_t);

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
            MakeStorageBindGroupLayout(numStorageBuffers, wgpu::ShaderStage::Fragment);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for render pipelines - calculate exact size needed
        // +1 for drawCallIndex parameter
        plDesc.immediateSize = (numImmediateValues + 1) * sizeof(uint32_t);

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

    // Helper to create unified result bind group (for both compute and render)
    wgpu::BindGroup MakeResultBindGroup(wgpu::ShaderStage stage) {
        wgpu::BindGroupLayout resultLayout =
            utils::MakeBindGroupLayout(device, {{0, stage, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, mResultBuffer}});
    }

    // Helper to validate results from the single result buffer at specific indices
    void ValidateResultsAtIndices(const std::vector<std::vector<uint32_t>>& expectedResults,
                                  const std::string& callType = "Call") {
        for (size_t i = 0; i < expectedResults.size(); i++) {
            std::string message = callType + " " + std::to_string(i);
            // Each result entry is 16 uint32_t values (8 buffer lengths + 8 immediates)
            uint32_t offset = i * 16 * sizeof(uint32_t);
            EXPECT_BUFFER_U32_RANGE_EQ(expectedResults[i].data(), mResultBuffer, offset,
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
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data - drawCallIndex (0) + struct with 3 u32 values
    uint32_t immediateData[] = {0, 42, 123, 456};  // drawCallIndex=0, then value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData), sizeof(immediateData));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and immediate data [42, 123, 456] at index 0
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
    wgpu::ComputePipeline pipeline1 = MakePipelineWithImmediateAndBuffers(2, 1);  // 1 immediate
    wgpu::ComputePipeline pipeline2 = MakePipelineWithImmediateAndBuffers(2, 2);  // 2 immediates
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with pipeline1 - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 100};  // drawCallIndex=0, value0
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.DispatchWorkgroups(1);

    // Change to pipeline2 with more immediates - buffer lengths should still be correct, writes to
    // index 1
    pass.SetPipeline(pipeline2);
    uint32_t immediateData2[] = {1, 300, 400};  // drawCallIndex=1, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for both dispatch calls
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Dispatch call 0 expectations (1 immediate) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;   // mStorageBuffer64
    expectedResults[0][1] = 64;   // mStorageBuffer256
    expectedResults[0][8] = 100;  // immediate.value0

    // Dispatch call 1 expectations (2 immediates) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;   // mStorageBuffer64
    expectedResults[1][1] = 64;   // mStorageBuffer256
    expectedResults[1][8] = 300;  // immediate.value0
    expectedResults[1][9] = 400;  // immediate.value1

    ValidateResultsAtIndices(expectedResults, "Dispatch call");
}

// Test: SetBindGroup + SetImmediateData + Draw + SetPipeline (different buffer count) +
// SetBindGroup + Draw This tests changing both pipeline and bind group, ensuring buffer length
// tracking adapts correctly
TEST_P(ImmediateDataBufferLengthTest, ComputePipelinePipelineAndBindGroupChange) {
    wgpu::ComputePipeline pipeline1 =
        MakePipelineWithImmediateAndBuffers(2, 2);  // 2 buffers, 2 immediates
    wgpu::ComputePipeline pipeline2 =
        MakePipelineWithImmediateAndBuffers(3, 2);  // 3 buffers, 2 immediates

    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute), 2);
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(3, wgpu::ShaderStage::Compute), 3);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with 2 buffers - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 111, 222};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.DispatchWorkgroups(1);

    // Change to pipeline with 3 buffers and update bind group - writes to index 1
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t immediateData2[] = {1, 333, 444};  // drawCallIndex=1, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for both dispatch calls
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Dispatch call 0 expectations (2 buffers) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;   // mStorageBuffer64
    expectedResults[0][1] = 64;   // mStorageBuffer256
    expectedResults[0][8] = 111;  // immediate.value0
    expectedResults[0][9] = 222;  // immediate.value1

    // Dispatch call 1 expectations (3 buffers) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;   // mStorageBuffer64
    expectedResults[1][1] = 64;   // mStorageBuffer256
    expectedResults[1][2] = 128;  // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expectedResults[1][8] = 333;  // immediate.value0
    expectedResults[1][9] = 444;  // immediate.value1

    ValidateResultsAtIndices(expectedResults, "Dispatch call");
}

// Test: Multiple SetImmediateData calls with buffer length updates
// This tests the edge case where immediate data is updated multiple times
TEST_P(ImmediateDataBufferLengthTest, ComputePipelineMultipleImmediateDataUpdates) {
    wgpu::ComputePipeline pipeline = MakePipelineWithImmediateAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(1, wgpu::ShaderStage::Compute), 1);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data multiple times
    uint32_t immediateData1[] = {0, 10, 20, 30};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    uint32_t immediateData2[] = {0, 40, 50, 60};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    uint32_t immediateData3[] = {0, 70, 80, 90};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData3), sizeof(immediateData3));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final immediate data [70, 80, 90] at index 0
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
    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute), 2);
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute),
                                 2);  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and immediate data updates
    pass.SetBindGroup(0, storageBindGroup1);
    uint32_t immediateData1[] = {0, 1000, 2000};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    pass.SetBindGroup(0, storageBindGroup2);
    uint32_t immediateData2[] = {0, 3000, 4000};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    pass.DispatchWorkgroups(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final immediate data [3000, 4000] at index 0
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 3000;  // final immediate.value0
    expected[9] = 4000;  // final immediate.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Compute pipeline change with same layout but different immediate counts
// This tests a potentially problematic case where pipelines have the same bind group layout
// but different immediate data sizes. This might expose issues with immediate data validation.
TEST_P(ImmediateDataBufferLengthTest, ComputePipelineSameLayoutDifferentImmediateCounts) {
    // Both pipelines use the same storage layout (2 buffers) but different immediate counts
    wgpu::ComputePipeline pipeline1 = MakePipelineWithImmediateAndBuffers(2, 3);  // 3 immediates
    wgpu::ComputePipeline pipeline2 =
        MakePipelineWithImmediateAndBuffers(2, 2);  // 2 immediates (same layout, fewer immediates)

    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Compute), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Compute);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First dispatch with pipeline1 (3 immediates) - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 100, 200, 300};  // drawCallIndex=0, 3 values
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.DispatchWorkgroups(1);

    // Second dispatch with pipeline2 (2 immediates, same layout) - writes to index 1
    pass.SetPipeline(pipeline2);

    uint32_t immediateData2[] = {1, 500, 600};  // drawCallIndex=1, 2 values
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.DispatchWorkgroups(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each dispatch call
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Dispatch call 0 expectations (3 immediates) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;    // mStorageBuffer64
    expectedResults[0][1] = 64;    // mStorageBuffer256
    expectedResults[0][8] = 100;   // immediate.value0
    expectedResults[0][9] = 200;   // immediate.value1
    expectedResults[0][10] = 300;  // immediate.value2

    // Dispatch call 1 expectations (2 immediates) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;   // mStorageBuffer64
    expectedResults[1][1] = 64;   // mStorageBuffer256
    expectedResults[1][8] = 500;  // immediate.value0
    expectedResults[1][9] = 600;  // immediate.value1
    // Note: values at indices 10-11 should remain 0 since pipeline2 only has 2 immediates

    ValidateResultsAtIndices(expectedResults, "Dispatch call");
}

// === RENDER PIPELINE TESTS ===

// Test: Render pipeline with SetBindGroup + SetImmediateData + Draw
// This tests the basic case for render pipelines
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineBasicImmediateAndBufferLength) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithImmediateAndBuffers(2, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data - drawCallIndex (0) + struct with 3 u32 values
    uint32_t immediateData[] = {0, 789, 987, 654};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData), sizeof(immediateData));

    pass.Draw(1);  // Draw 1 point
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and immediate data [789, 987, 654] at index 0
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64 has 64 bytes = 16 f32 elements
    expected[1] = 64;    // mStorageBuffer256 has 256 bytes = 64 f32 elements
    expected[8] = 789;   // immediate.value0
    expected[9] = 987;   // immediate.value1
    expected[10] = 654;  // immediate.value2

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Render pipeline with multiple draws and pipeline changes (updated to use single result
// buffer)
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineChangeWithDifferentImmediates) {
    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 2);  // 2 immediates
    wgpu::RenderPipeline pipeline2 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 3);  // 3 immediates
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1 - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 1100, 1200};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Second draw with pipeline2 - writes to index 1
    pass.SetPipeline(pipeline2);

    uint32_t immediateData2[] = {1, 1300, 1400, 1500};  // drawCallIndex=1, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each draw call with different expectations
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Draw call 0 expectations (2 immediates) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;    // mStorageBuffer64
    expectedResults[0][1] = 64;    // mStorageBuffer256
    expectedResults[0][8] = 1100;  // immediate.value0
    expectedResults[0][9] = 1200;  // immediate.value1

    // Draw call 1 expectations (3 immediates) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;     // mStorageBuffer64
    expectedResults[1][1] = 64;     // mStorageBuffer256
    expectedResults[1][8] = 1300;   // immediate.value0
    expectedResults[1][9] = 1400;   // immediate.value1
    expectedResults[1][10] = 1500;  // immediate.value2

    ValidateResultsAtIndices(expectedResults, "Draw call");
}

// Test: Render pipeline with buffer count changes (updated to use single result buffer)
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineAndBindGroupChange) {
    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 2);  // 2 buffers, 2 immediates
    wgpu::RenderPipeline pipeline2 =
        MakeRenderPipelineWithImmediateAndBuffers(3, 2);  // 3 buffers, 2 immediates

    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment), 2);
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(3, wgpu::ShaderStage::Fragment), 3);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with 2 buffers - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 2111, 2222};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Second draw with 3 buffers - writes to index 1
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t immediateData2[] = {1, 2333, 2444};  // drawCallIndex=1, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each draw call with different buffer configurations
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Draw call 0 expectations (2 buffers) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;    // mStorageBuffer64
    expectedResults[0][1] = 64;    // mStorageBuffer256
    expectedResults[0][8] = 2111;  // immediate.value0
    expectedResults[0][9] = 2222;  // immediate.value1

    // Draw call 1 expectations (3 buffers) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;    // mStorageBuffer64
    expectedResults[1][1] = 64;    // mStorageBuffer256
    expectedResults[1][2] = 128;   // mStorageBuffer512 has 512 bytes = 128 f32 elements
    expectedResults[1][8] = 2333;  // immediate.value0
    expectedResults[1][9] = 2444;  // immediate.value1

    ValidateResultsAtIndices(expectedResults, "Draw call");
}

// Test: Render pipeline with multiple immediate data updates
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineMultipleImmediateDataUpdates) {
    wgpu::RenderPipeline pipeline = MakeRenderPipelineWithImmediateAndBuffers(1, 3);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(1, wgpu::ShaderStage::Fragment), 1);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    // Set immediate data multiple times
    uint32_t immediateData1[] = {0, 3010, 3020, 3030};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    uint32_t immediateData2[] = {0, 3040, 3050, 3060};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    uint32_t immediateData3[] = {0, 3070, 3080, 3090};  // drawCallIndex=0, value0, value1, value2
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData3), sizeof(immediateData3));

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer length [16] and final immediate data [3070, 3080, 3090] at index 0
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
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment), 2);
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment),
                                 2);  // Same layout, but could be different buffers
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(1, resultBindGroup);

    // Interleave bind group and immediate data updates
    pass.SetBindGroup(0, storageBindGroup1);
    uint32_t immediateData1[] = {0, 4000, 5000};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    pass.SetBindGroup(0, storageBindGroup2);
    uint32_t immediateData2[] = {0, 6000, 7000};  // drawCallIndex=0, value0, value1
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Expected: buffer lengths [16, 64] and final immediate data [6000, 7000] at index 0
    std::vector<uint32_t> expected(16, 0);
    expected[0] = 16;    // mStorageBuffer64
    expected[1] = 64;    // mStorageBuffer256
    expected[8] = 6000;  // final immediate.value0
    expected[9] = 7000;  // final immediate.value1

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), mResultBuffer, 0, expected.size());
}

// Test: Pipeline change with same layout but different immediate counts
// This tests a potentially problematic case where pipelines have the same bind group layout
// but different immediate data sizes. This might expose issues with immediate data validation.
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineSameLayoutDifferentImmediateCounts) {
    // Both pipelines use the same storage layout (2 buffers) but different immediate counts
    wgpu::RenderPipeline pipeline1 =
        MakeRenderPipelineWithImmediateAndBuffers(2, 3);  // 3 immediates
    wgpu::RenderPipeline pipeline2 = MakeRenderPipelineWithImmediateAndBuffers(
        2, 2);  // 2 immediates (same layout, fewer immediates)

    wgpu::BindGroup storageBindGroup =
        MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(2, wgpu::ShaderStage::Fragment), 2);
    wgpu::BindGroup resultBindGroup = MakeResultBindGroup(wgpu::ShaderStage::Fragment);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1 (3 immediates) - writes to index 0
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, resultBindGroup);

    uint32_t immediateData1[] = {0, 1000, 2000, 3000};  // drawCallIndex=0, 3 values
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Second draw with pipeline2 (2 immediates, same layout) - writes to index 1
    pass.SetPipeline(pipeline2);

    uint32_t immediateData2[] = {1, 5000, 6000};  // drawCallIndex=1, 2 values
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Create expected results for each draw call
    std::vector<std::vector<uint32_t>> expectedResults(2);

    // Draw call 0 expectations (3 immediates) at index 0
    expectedResults[0] = std::vector<uint32_t>(16, 0);
    expectedResults[0][0] = 16;     // mStorageBuffer64
    expectedResults[0][1] = 64;     // mStorageBuffer256
    expectedResults[0][8] = 1000;   // immediate.value0
    expectedResults[0][9] = 2000;   // immediate.value1
    expectedResults[0][10] = 3000;  // immediate.value2

    // Draw call 1 expectations (2 immediates) at index 1
    expectedResults[1] = std::vector<uint32_t>(16, 0);
    expectedResults[1][0] = 16;    // mStorageBuffer64
    expectedResults[1][1] = 64;    // mStorageBuffer256
    expectedResults[1][8] = 5000;  // immediate.value0
    expectedResults[1][9] = 6000;  // immediate.value1
    // Note: value at index 10 should remain 0 since pipeline2 only has 2 immediates

    ValidateResultsAtIndices(expectedResults, "Draw call");
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
