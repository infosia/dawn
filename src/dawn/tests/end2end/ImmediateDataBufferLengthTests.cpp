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

// Storage buffer size in bytes.
enum class Size { Small = 64u, Medium = 256u, Large = 512u };

enum class StorageBufferCount { Single = 1u, Double = 2u, Triple = 3u };

struct PipelineResourceStates {
    uint32_t storageBufferCount = 0;
    uint32_t immediateCount = 0;
};

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

        // Create a result buffer.
        // Maximum: 2 draw calls, 3 storage buffers and 4 immediates.
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.usage = wgpu::BufferUsage::Storage;
        bufferDesc.size = 2u * (static_cast<uint32_t>(StorageBufferCount::Triple) + 4) *
                          kImmediateConstantElementByteSize;
        bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        mResultBuffer = device.CreateBuffer(&bufferDesc);
    }

    // Helper to create bind groups with different buffer configurations
    wgpu::BindGroup MakeBindGroupWithBuffers(wgpu::BindGroupLayout layout,
                                             StorageBufferCount bufferCount,
                                             const std::array<Size, 3>& bufferSizes) {
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.usage = wgpu::BufferUsage::Storage;

        std::vector<wgpu::BindGroupEntry> entries;
        wgpu::BindGroupEntry entry;

        for (uint32_t i = 0; i < static_cast<uint32_t>(bufferCount); ++i) {
            bufferDesc.size = static_cast<uint32_t>(bufferSizes[i]);
            wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);
            mStorageBuffers.push_back(buffer);
            entry.binding = i;
            entry.buffer = buffer;
            entries.push_back(entry);
        }

        wgpu::BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = layout;
        bindGroupDesc.entryCount = entries.size();
        bindGroupDesc.entries = entries.data();
        return device.CreateBindGroup(&bindGroupDesc);
    }

    // Helper to create unified shader with compute, vertex, and fragment entry points
    wgpu::ShaderModule CreateUnifiedShaderModule(uint32_t immediateCount,
                                                 StorageBufferCount bufferCount) {
        std::string bufferDeclarations;
        std::string lengthComputation;

        for (uint32_t i = 0; i < static_cast<uint32_t>(bufferCount); i++) {
            bufferDeclarations += "    @group(0) @binding(" + std::to_string(i) +
                                  ") var<storage, read> buffer" + std::to_string(i) +
                                  " : array<f32>;\n";
            lengthComputation += "    result[resultIndex].bufferLengths[" + std::to_string(i) +
                                 "] = arrayLength(&buffer" + std::to_string(i) + ");\n";
        }

        std::string immediateStruct = "struct ImmediateData {\n";
        immediateStruct += "        resultIndex : u32,\n";  // Add result index as first field
        for (uint32_t i = 0; i < immediateCount; i++) {
            immediateStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        immediateStruct += "    }\n";

        std::string immediateAccess;
        for (uint32_t i = 0; i < immediateCount; i++) {
            immediateAccess += "    result[resultIndex].immediates[" + std::to_string(i) +
                               "] = immediateData.value" + std::to_string(i) + ";\n";
        }

        // Unified shader with all entry points
        std::string shader = R"(
            enable chromium_experimental_immediate;
            struct ResultData {
                bufferLengths : array<u32, 3>,
                immediates : array<u32, 4>,
            }
            @group(1) @binding(0) var<storage, read_write> result : array<ResultData, 2>;
)" + bufferDeclarations + immediateStruct +
                             R"(
            var<immediate> immediateData: ImmediateData;

            // Shared function for buffer length and immediate data processing
            fn processData() {
                let resultIndex = immediateData.resultIndex;
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

    void UpdateBufferLengthExpectedResults(StorageBufferCount bufferCount,
                                           const std::array<Size, 3>& bufferSizes) {
        currentSetStorageCount = static_cast<uint32_t>(bufferCount);

        size_t index =
            resultIndex * (static_cast<size_t>(StorageBufferCount::Triple) + kMaxImmediateCounts);
        for (uint32_t i = 0; i < static_cast<uint32_t>(bufferCount); ++i) {
            expectedResults[index + i] = static_cast<uint32_t>(bufferSizes[i]) / sizeof(uint32_t);
        }
    }

    void UpdateImmediateExpectedResults(const std::vector<uint32_t>& immediates) {
        currentSetImmediateCount = immediates.size();

        size_t index =
            resultIndex * (static_cast<size_t>(StorageBufferCount::Triple) + kMaxImmediateCounts);
        for (size_t i = 0; i < immediates.size(); ++i) {
            expectedResults[index + static_cast<size_t>(StorageBufferCount::Triple) + i] =
                immediates[i];
        }
    }

    void SetPipelineResourceState(uint32_t immediateCount, uint32_t bufferCount) {
        pipelineResourceStates[resultIndex].immediateCount = immediateCount;
        pipelineResourceStates[resultIndex].storageBufferCount = bufferCount;
    }

    void HandleInheritStates() {
        if (resultIndex != 0) {
            constexpr uint32_t offset =
                static_cast<size_t>(StorageBufferCount::Triple) + kMaxImmediateCounts;

            // Handle Bindgroup inheritance.
            uint32_t lastStorageBufferCount =
                pipelineResourceStates[resultIndex - 1].storageBufferCount;
            if (lastStorageBufferCount == pipelineResourceStates[resultIndex].storageBufferCount) {
                uint32_t inheritBufferCount = lastStorageBufferCount - currentSetStorageCount;

                size_t lastIndex = resultIndex * (static_cast<size_t>(StorageBufferCount::Triple) +
                                                  kMaxImmediateCounts) -
                                   1 - kMaxImmediateCounts;
                for (uint32_t i = inheritBufferCount; i > 0; --i) {
                    expectedResults[lastIndex - i + offset] = expectedResults[lastIndex - i];
                }
            }

            // Handle immediate data inheritance.
            uint32_t lastImmediateCount = pipelineResourceStates[resultIndex - 1].immediateCount;
            if (lastImmediateCount == pipelineResourceStates[resultIndex].immediateCount) {
                uint32_t inheritImmediateCount = lastImmediateCount - currentSetImmediateCount;
                size_t lastIndex = resultIndex * (static_cast<size_t>(StorageBufferCount::Triple) +
                                                  kMaxImmediateCounts) -
                                   1;
                for (uint32_t i = inheritImmediateCount; i > 0; --i) {
                    expectedResults[lastIndex - i + offset] = expectedResults[lastIndex - i];
                }
            }
        }

        currentSetStorageCount = 0;
        currentSetImmediateCount = 0;
    }

    static constexpr uint32_t kMaxImmediateCounts = 4;
    static constexpr uint32_t kMaxExecutionTime = 2;
    std::vector<wgpu::Buffer> mStorageBuffers;
    wgpu::Buffer mResultBuffer;

    // Record storage buffer counts and immediates count to handle inheritance.
    std::array<PipelineResourceStates, kMaxExecutionTime> pipelineResourceStates;
    uint32_t currentSetStorageCount = 0;
    // Current test cases are all from offset 0.
    uint32_t currentSetImmediateCount = 0;
    std::array<uint32_t, 14> expectedResults = {};
    uint32_t resultIndex = 0;
};

// Test fixture specifically for testing the interaction between immediate data (SetImmediateData)
// and storage buffer length tracking. This is critical to ensure that when both immediate
// constants and buffer length data are uploaded to the same GPU buffer, they don't interfere
// with each other across multiple Apply calls with changing pipeline/bind group state.
class ImmediateDataBufferLengthComputePipelineTest : public ImmediateDataBufferLengthTest {
  protected:
    // Helper to create compute pipelines with different numbers of immediate constants and storage
    // buffers
    wgpu::ComputePipeline MakeComputePipelineWithImmediateAndBuffers(
        uint32_t immediateCount,
        StorageBufferCount bufferCount) {
        wgpu::ShaderModule shaderModule = CreateUnifiedShaderModule(immediateCount, bufferCount);

        wgpu::BindGroupLayout storageLayout = MakeStorageBindGroupLayout(bufferCount);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for compute pipelines - calculate exact size needed
        // +1 for drawCallIndex parameter
        plDesc.immediateSize = (immediateCount + 1) * kImmediateConstantElementByteSize;

        wgpu::PipelineLayout layout = device.CreatePipelineLayout(&plDesc);

        wgpu::ComputePipelineDescriptor pipelineDesc;
        pipelineDesc.layout = layout;
        pipelineDesc.compute.module = shaderModule;
        return device.CreateComputePipeline(&pipelineDesc);
    }

    // Helper to create a bind group layout with storage buffers for any shader stage
    wgpu::BindGroupLayout MakeStorageBindGroupLayout(StorageBufferCount bufferCount) {
        switch (bufferCount) {
            case StorageBufferCount::Single:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage}});
            case StorageBufferCount::Double:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage}});
            case StorageBufferCount::Triple:
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

    // Helper to create unified result bind group (for both compute and render)
    wgpu::BindGroup MakeResultBindGroup() {
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, mResultBuffer}});
    }

    void UseComputePipeline(wgpu::ComputePassEncoder pass,
                            uint32_t immediateCount,
                            StorageBufferCount bufferCount) {
        SetPipelineResourceState(immediateCount, static_cast<uint32_t>(bufferCount));
        pass.SetPipeline(MakeComputePipelineWithImmediateAndBuffers(immediateCount, bufferCount));
    }

    void SetStorageBuffer(wgpu::ComputePassEncoder pass,
                          StorageBufferCount count,
                          const std::array<Size, 3>& bufferSizes) {
        pass.SetBindGroup(
            0, MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(count), count, bufferSizes));
        wgpu::BindGroup resultBindGroup = MakeResultBindGroup();
        pass.SetBindGroup(1, resultBindGroup);
        UpdateBufferLengthExpectedResults(count, bufferSizes);
    }

    void SetImmediates(wgpu::ComputePassEncoder pass, const std::vector<uint32_t>& immediates) {
        std::vector<uint32_t> immediateData;
        immediateData.push_back(resultIndex);  // First value is always the draw call index
        immediateData.insert(immediateData.end(), immediates.begin(), immediates.end());
        pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData.data()),
                              immediateData.size() * sizeof(uint32_t));
        UpdateImmediateExpectedResults(immediates);
    }

    void Dispatch(wgpu::ComputePassEncoder pass) {
        pass.DispatchWorkgroups(1);
        HandleInheritStates();
        resultIndex++;
    }

    void CheckExpectedResults() {
        EXPECT_BUFFER_U32_RANGE_EQ(expectedResults.data(), mResultBuffer, 0, expectedResults.size())
            << "dispatch index: " << std::to_string(resultIndex);
    }
};

// Test fixture specifically for testing the interaction between immediate data (SetImmediateData)
// and storage buffer length tracking. This is critical to ensure that when both immediate
// constants and buffer length data are uploaded to the same GPU buffer, they don't interfere
// with each other across multiple Apply calls with changing pipeline/bind group state.
class ImmediateDataBufferLengthRenderPipelineTest : public ImmediateDataBufferLengthTest {
  protected:
    void SetUp() override {
        ImmediateDataBufferLengthTest::SetUp();

        // Create render target for render pipeline tests
        wgpu::TextureDescriptor textureDesc;
        textureDesc.size = {1, 1, 1};
        textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment;
        mRenderTarget = device.CreateTexture(&textureDesc);
        mRenderTargetView = mRenderTarget.CreateView();
    }

    // Helper to create render pipelines with different numbers of immediate constants and storage
    // buffers
    wgpu::RenderPipeline MakeRenderPipelineWithImmediateAndBuffers(uint32_t immediateCount,
                                                                   StorageBufferCount bufferCount) {
        wgpu::ShaderModule shaderModule = CreateUnifiedShaderModule(immediateCount, bufferCount);

        wgpu::BindGroupLayout storageLayout = MakeStorageBindGroupLayout(bufferCount);
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout layouts[] = {storageLayout, resultLayout};
        plDesc.bindGroupLayoutCount = 2;
        plDesc.bindGroupLayouts = layouts;

        // Set up immediate data layout for render pipelines - calculate exact size needed
        // +1 for drawCallIndex parameter
        plDesc.immediateSize = (immediateCount + 1) * kImmediateConstantElementByteSize;

        wgpu::PipelineLayout layout = device.CreatePipelineLayout(&plDesc);

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.layout = layout;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.cFragment.module = shaderModule;
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;
        pipelineDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;

        return device.CreateRenderPipeline(&pipelineDesc);
    }

    // Helper to create a bind group layout with storage buffers for any shader stage
    wgpu::BindGroupLayout MakeStorageBindGroupLayout(StorageBufferCount bufferCount) {
        switch (bufferCount) {
            case StorageBufferCount::Single:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}});
            case StorageBufferCount::Double:
                return utils::MakeBindGroupLayout(
                    device,
                    {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage},
                     {1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::ReadOnlyStorage}});
            case StorageBufferCount::Triple:
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

    // Helper to create unified result bind group (for both compute and render)
    wgpu::BindGroup MakeResultBindGroup() {
        wgpu::BindGroupLayout resultLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, resultLayout, {{0, mResultBuffer}});
    }

    void UseRenderPipeline(wgpu::RenderPassEncoder pass,
                           uint32_t immediateCount,
                           StorageBufferCount bufferCount) {
        SetPipelineResourceState(immediateCount, static_cast<uint32_t>(bufferCount));
        pass.SetPipeline(MakeRenderPipelineWithImmediateAndBuffers(immediateCount, bufferCount));
    }

    void SetStorageBuffer(wgpu::RenderPassEncoder pass,
                          StorageBufferCount count,
                          const std::array<Size, 3>& bufferSizes) {
        pass.SetBindGroup(
            0, MakeBindGroupWithBuffers(MakeStorageBindGroupLayout(count), count, bufferSizes));
        wgpu::BindGroup resultBindGroup = MakeResultBindGroup();
        pass.SetBindGroup(1, resultBindGroup);
        UpdateBufferLengthExpectedResults(count, bufferSizes);
    }

    void SetImmediates(wgpu::RenderPassEncoder pass, const std::vector<uint32_t>& immediates) {
        std::vector<uint32_t> immediateData;
        immediateData.push_back(resultIndex);  // First value is always the draw call index
        immediateData.insert(immediateData.end(), immediates.begin(), immediates.end());
        pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData.data()),
                              immediateData.size() * sizeof(uint32_t));
        UpdateImmediateExpectedResults(immediates);
    }

    void Draw(wgpu::RenderPassEncoder pass) {
        pass.Draw(1);
        HandleInheritStates();
        resultIndex++;
    }

    void CheckExpectedResults() {
        EXPECT_BUFFER_U32_RANGE_EQ(expectedResults.data(), mResultBuffer, 0, expectedResults.size())
            << "draw index: " << std::to_string(resultIndex);
    }

    wgpu::Texture mRenderTarget;
    wgpu::TextureView mRenderTargetView;
};

// Test: SetBindGroup + SetImmediateData + Draw
// This is the basic case to ensure immediate data and buffer lengths work together
TEST_P(ImmediateDataBufferLengthComputePipelineTest, BasicImmediateAndBufferLength) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    UseComputePipeline(pass, 3, StorageBufferCount::Triple);
    SetStorageBuffer(pass, StorageBufferCount::Triple, {Size::Small, Size::Medium, Size::Large});
    SetImmediates(pass, {42, 123, 456});
    Dispatch(pass);

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: SetBindGroup + SetImmediateData + Draw + SetPipeline (different immediates) + Draw
// This tests that changing pipelines with different immediate counts still preserves buffer lengths
TEST_P(ImmediateDataBufferLengthComputePipelineTest, PipelineChangeWithDifferentImmediates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with pipeline1 - writes to index 0
    UseComputePipeline(pass, 1, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {100});
    Dispatch(pass);

    // Change to pipeline2 with more immediates
    UseComputePipeline(pass, 2, StorageBufferCount::Double);
    SetImmediates(pass, {300, 400});
    Dispatch(pass);

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Compute pipeline change with same layout but different immediate counts
// This tests a potentially problematic case where pipelines have the same bind group layout
// but different immediate data sizes. This might expose issues with immediate data validation.
TEST_P(ImmediateDataBufferLengthComputePipelineTest, PipelineChangeWithReducedImmediateCounts) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First dispatch with pipeline1 (3 immediates) - writes to index 0
    UseComputePipeline(pass, 3, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {100, 200, 300});
    Dispatch(pass);

    // Second dispatch with pipeline2 (2 immediates, same layout) - writes to index 1
    UseComputePipeline(pass, 2, StorageBufferCount::Double);
    SetImmediates(pass, {500, 600});
    Dispatch(pass);

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: SetBindGroup + SetImmediateData + Draw + SetPipeline (different buffer count) +
// SetBindGroup + Draw This tests changing both pipeline and bind group, ensuring buffer length
// tracking adapts correctly
TEST_P(ImmediateDataBufferLengthComputePipelineTest, PipelineAndBindGroupChange) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // First draw with 2 buffers
    UseComputePipeline(pass, 2, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {111, 222});
    Dispatch(pass);

    // Change to pipeline with 3 buffers and update bind group
    UseComputePipeline(pass, 2, StorageBufferCount::Triple);
    SetStorageBuffer(pass, StorageBufferCount::Triple, {Size::Large, Size::Small, Size::Medium});
    SetImmediates(pass, {333, 444});
    Dispatch(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Multiple SetImmediateData calls with buffer length updates
// This tests the edge case where immediate data is updated multiple times
TEST_P(ImmediateDataBufferLengthComputePipelineTest, MultipleImmediateDataUpdates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    UseComputePipeline(pass, 3, StorageBufferCount::Single);
    SetStorageBuffer(pass, StorageBufferCount::Single, {Size::Small});
    SetImmediates(pass, {10, 20, 30});
    SetImmediates(pass, {40, 50, 60});
    SetImmediates(pass, {70, 80, 90});
    Dispatch(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Interleaved SetBindGroup and SetImmediateData calls
// This tests complex state management scenarios
TEST_P(ImmediateDataBufferLengthComputePipelineTest, InterleavedUpdates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();

    // Interleave bind group and immediate data updates
    UseComputePipeline(pass, 2, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {1000, 2000});
    Dispatch(pass);

    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Medium, Size::Large});
    SetImmediates(pass, {3000, 4000});
    Dispatch(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

DAWN_INSTANTIATE_TEST(ImmediateDataBufferLengthComputePipelineTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

// Test: Render pipeline with SetBindGroup + SetImmediateData + Draw
// This tests the basic case for render pipelines
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, RenderPipelineBasicImmediateAndBufferLength) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    UseRenderPipeline(pass, 3, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {789, 987, 654});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Render pipeline with multiple draws and pipeline changes (updated to use single result
// buffer)
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, PipelineChangeWithIncreasedImmediates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1
    UseRenderPipeline(pass, 2, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {1100, 1200});
    Draw(pass);

    // Second draw with pipeline2
    UseRenderPipeline(pass, 3, StorageBufferCount::Double);
    SetImmediates(pass, {1300, 1400, 1500});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Pipeline change with same layout but different immediate counts
// This tests a potentially problematic case where pipelines have the same bind group layout
// but different immediate data sizes. This might expose issues with immediate data validation.
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, PipelineChangeWithReducedImmediateCounts) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1 (3 immediates) - writes to index 0
    UseRenderPipeline(pass, 3, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {1000, 2000, 3000});
    Draw(pass);

    // Second draw with pipeline2 (2 immediates, same layout) - writes to index 1
    UseRenderPipeline(pass, 2, StorageBufferCount::Double);
    SetImmediates(pass, {5000, 6000});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Render pipeline with buffer count changes (updated to use single result buffer)
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, PipelineAndBindGroupChange) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with 2 buffers
    UseRenderPipeline(pass, 2, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {2111, 2222});
    Draw(pass);

    // Second draw with 3 buffers - writes to index 1
    UseRenderPipeline(pass, 2, StorageBufferCount::Triple);
    SetStorageBuffer(pass, StorageBufferCount::Triple, {Size::Large, Size::Small, Size::Medium});
    SetImmediates(pass, {2333, 2444});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Render pipeline with multiple immediate data updates
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, MultipleImmediateDataUpdates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    UseRenderPipeline(pass, 3, StorageBufferCount::Single);
    SetStorageBuffer(pass, StorageBufferCount::Single, {Size::Small});
    SetImmediates(pass, {3010, 3020, 3030});
    SetImmediates(pass, {3040, 3050, 3060});
    SetImmediates(pass, {3070, 3080, 3090});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

// Test: Render pipeline with interleaved updates
TEST_P(ImmediateDataBufferLengthRenderPipelineTest, InterleavedUpdates) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    UseRenderPipeline(pass, 2, StorageBufferCount::Double);
    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Small, Size::Medium});
    SetImmediates(pass, {4000, 5000});
    Draw(pass);

    SetStorageBuffer(pass, StorageBufferCount::Double, {Size::Medium, Size::Large});
    SetImmediates(pass, {6000, 7000});
    Draw(pass);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    CheckExpectedResults();
}

DAWN_INSTANTIATE_TEST(ImmediateDataBufferLengthRenderPipelineTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
