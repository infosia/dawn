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
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
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

    // Helper to create render shader without debug validation (for non-final draws)
    wgpu::ShaderModule CreateSimpleRenderShader(uint32_t numStorageBuffers,
                                                uint32_t numImmediateValues) {
        std::string bufferDeclarations;
        for (uint32_t i = 0; i < numStorageBuffers; i++) {
            bufferDeclarations += "    @group(0) @binding(" + std::to_string(i) +
                                  ") var<storage, read> buffer" + std::to_string(i) +
                                  " : array<f32>;\n";
        }

        std::string immediateStruct = "struct ImmediateData {\n";
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        immediateStruct += "    }\n";

        // Simple shader without debug validation
        std::string shader = R"(
            enable chromium_experimental_immediate;)" +
                             bufferDeclarations + immediateStruct + R"(
            var<immediate> immediateData: ImmediateData;

            // Vertex entry point
            @vertex fn vsMain() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 1.0);
            }

            // Fragment entry point - simple color output
            @fragment fn fsMain() -> @location(0) vec4f {
                return vec4f(0.0, 1.0, 0.0, 1.0);  // Green
            }
        )";

        return utils::CreateShaderModule(device, shader.c_str());
    }

    // Helper to create render shader with in-shader validation
    wgpu::ShaderModule CreateRenderShaderWithValidation(
        uint32_t numStorageBuffers,
        uint32_t numImmediateValues,
        const std::vector<uint32_t>& expectedBufferLengths,
        const std::vector<uint32_t>& expectedImmediates,
        bool enableDebugInfo = false) {
        std::string bufferDeclarations;
        for (uint32_t i = 0; i < numStorageBuffers; i++) {
            bufferDeclarations += "    @group(0) @binding(" + std::to_string(i) +
                                  ") var<storage, read> buffer" + std::to_string(i) +
                                  " : array<f32>;\n";
        }

        // Add debug buffer declaration if debug info is enabled
        std::string debugBufferDeclaration;
        if (enableDebugInfo) {
            debugBufferDeclaration = R"(
    struct DebugInfo {
        failedIndex: array<u32, 32>,      // Which validation failed (0=buffer0_len, 1=buffer1_len, etc.)
        expectedValue: array<u32, 32>,    // Expected values
        actualValue: array<u32, 32>,      // Actual values
        failureCount: u32,                // Number of failures recorded
    }
    @group(1) @binding(0) var<storage, read_write> debugInfo: DebugInfo;
)";
        }

        std::string immediateStruct = "struct ImmediateData {\n";
        for (uint32_t i = 0; i < numImmediateValues; i++) {
            immediateStruct += "        value" + std::to_string(i) + " : u32,\n";
        }
        immediateStruct += "    }\n";

        // Generate expected values as constants
        std::string expectedConstants;
        for (uint32_t i = 0; i < expectedBufferLengths.size(); i++) {
            expectedConstants += "const EXPECTED_BUFFER_LENGTH_" + std::to_string(i) +
                                 " : u32 = " + std::to_string(expectedBufferLengths[i]) + "u;\n";
        }
        for (uint32_t i = 0; i < expectedImmediates.size(); i++) {
            expectedConstants += "const EXPECTED_IMMEDIATE_" + std::to_string(i) +
                                 " : u32 = " + std::to_string(expectedImmediates[i]) + "u;\n";
        }

        // Generate validation logic with color output and optional debug info
        std::string validationLogic = "    var isValid = true;\n";

        if (enableDebugInfo) {
            validationLogic += "    debugInfo.failureCount = 0u;\n";
        }

        // Check buffer lengths
        for (uint32_t i = 0; i < expectedBufferLengths.size(); i++) {
            validationLogic += "    let actualBufferLength" + std::to_string(i) +
                               " = arrayLength(&buffer" + std::to_string(i) + ");\n";
            validationLogic += "    if (actualBufferLength" + std::to_string(i) +
                               " != EXPECTED_BUFFER_LENGTH_" + std::to_string(i) + ") {\n";
            validationLogic += "        isValid = false;\n";

            if (enableDebugInfo) {
                validationLogic += "        if (debugInfo.failureCount < 32u) {\n";
                validationLogic += "            debugInfo.failedIndex[debugInfo.failureCount] = " +
                                   std::to_string(i) + "u;\n";
                validationLogic +=
                    "            debugInfo.expectedValue[debugInfo.failureCount] = "
                    "EXPECTED_BUFFER_LENGTH_" +
                    std::to_string(i) + ";\n";
                validationLogic +=
                    "            debugInfo.actualValue[debugInfo.failureCount] = "
                    "actualBufferLength" +
                    std::to_string(i) + ";\n";
                validationLogic +=
                    "            debugInfo.failureCount = debugInfo.failureCount + 1u;\n";
                validationLogic += "        }\n";
            }

            validationLogic += "    }\n";
        }

        // Check immediate values
        for (uint32_t i = 0; i < expectedImmediates.size(); i++) {
            uint32_t debugIndex = expectedBufferLengths.size() + i;  // Offset by buffer count
            validationLogic += "    if (immediateData.value" + std::to_string(i) +
                               " != EXPECTED_IMMEDIATE_" + std::to_string(i) + ") {\n";
            validationLogic += "        isValid = false;\n";

            if (enableDebugInfo) {
                validationLogic += "        if (debugInfo.failureCount < 32u) {\n";
                validationLogic += "            debugInfo.failedIndex[debugInfo.failureCount] = " +
                                   std::to_string(debugIndex) + "u;\n";
                validationLogic +=
                    "            debugInfo.expectedValue[debugInfo.failureCount] = "
                    "EXPECTED_IMMEDIATE_" +
                    std::to_string(i) + ";\n";
                validationLogic +=
                    "            debugInfo.actualValue[debugInfo.failureCount] = "
                    "immediateData.value" +
                    std::to_string(i) + ";\n";
                validationLogic +=
                    "            debugInfo.failureCount = debugInfo.failureCount + 1u;\n";
                validationLogic += "        }\n";
            }

            validationLogic += "    }\n";
        }

        validationLogic += "    if (isValid) {\n";
        validationLogic += "        return vec4f(0.0, 1.0, 0.0, 1.0);  // Green for success\n";
        validationLogic += "    } else {\n";
        validationLogic += "        return vec4f(1.0, 0.0, 0.0, 1.0);  // Red for failure\n";
        validationLogic += "    }\n";

        // Create shader with validation logic
        std::string shader = R"(
            enable chromium_experimental_immediate;)" +
                             bufferDeclarations + debugBufferDeclaration + immediateStruct +
                             expectedConstants + R"(
            var<immediate> immediateData: ImmediateData;

            // Vertex entry point
            @vertex fn vsMain() -> @builtin(position) vec4f {
                return vec4f(0.0, 0.0, 0.0, 1.0);
            }

            // Fragment entry point with validation
            @fragment fn fsMain() -> @location(0) vec4f {
)" + validationLogic + R"(
            }
        )";

        return utils::CreateShaderModule(device, shader.c_str());
    }  // Helper to create compute pipelines with different numbers of immediate constants and
       // storage
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

    // Helper to create simple render pipelines without debug validation
    wgpu::RenderPipeline MakeSimpleRenderPipeline(uint32_t numStorageBuffers,
                                                  uint32_t numImmediateValues) {
        wgpu::ShaderModule shaderModule =
            CreateSimpleRenderShader(numStorageBuffers, numImmediateValues);

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout storageLayout;  // Declare outside to ensure proper lifecycle
        std::vector<wgpu::BindGroupLayout> layouts;

        if (numStorageBuffers > 0) {
            // Include storage buffer layout if we have storage buffers
            storageLayout = MakeStorageBindGroupLayoutForRender(numStorageBuffers);
            layouts.push_back(storageLayout);
        }

        if (!layouts.empty()) {
            plDesc.bindGroupLayoutCount = layouts.size();
            plDesc.bindGroupLayouts = layouts.data();
        } else {
            plDesc.bindGroupLayoutCount = 0;
            plDesc.bindGroupLayouts = nullptr;
        }

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

    // Helper to create render pipelines with in-shader validation
    wgpu::RenderPipeline MakeRenderPipelineWithValidation(
        uint32_t numStorageBuffers,
        uint32_t numImmediateValues,
        const std::vector<uint32_t>& expectedBufferLengths,
        const std::vector<uint32_t>& expectedImmediates,
        bool enableDebugInfo = false) {
        wgpu::ShaderModule shaderModule = CreateRenderShaderWithValidation(
            numStorageBuffers, numImmediateValues, expectedBufferLengths, expectedImmediates,
            enableDebugInfo);

        wgpu::PipelineLayoutDescriptor plDesc;
        wgpu::BindGroupLayout storageLayout;  // Declare outside to ensure proper lifecycle
        wgpu::BindGroupLayout debugLayout;    // Declare outside to ensure proper lifecycle
        std::vector<wgpu::BindGroupLayout> layouts;

        if (numStorageBuffers > 0) {
            // Include storage buffer layout if we have storage buffers
            storageLayout = MakeStorageBindGroupLayoutForRender(numStorageBuffers);
            layouts.push_back(storageLayout);
        }

        if (enableDebugInfo) {
            // Include debug buffer layout if debug info is enabled
            debugLayout = utils::MakeBindGroupLayout(
                device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
            layouts.push_back(debugLayout);
        }

        if (!layouts.empty()) {
            plDesc.bindGroupLayoutCount = layouts.size();
            plDesc.bindGroupLayouts = layouts.data();
        } else {
            plDesc.bindGroupLayoutCount = 0;
            plDesc.bindGroupLayouts = nullptr;
        }

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

    // Helper to create a bind group layout with storage buffers for render pipelines (vertex stage)
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

    // Helper to create debug buffer for validation information
    wgpu::Buffer CreateDebugBuffer() {
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.size = 512;  // 32 * 4 * 4 bytes (32 entries * 4 arrays * 4 bytes each) + padding
        bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        return device.CreateBuffer(&bufferDesc);
    }

    // Helper to create debug bind group
    wgpu::BindGroup CreateDebugBindGroup(const wgpu::Buffer& debugBuffer) {
        wgpu::BindGroupLayout debugLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Storage}});
        return utils::MakeBindGroup(device, debugLayout, {{0, debugBuffer}});
    }

    // Helper to read back and print debug information
    void PrintDebugInfo(const wgpu::Buffer& debugBuffer, const char* testName) {
        // Create staging buffer for readback
        wgpu::BufferDescriptor stagingDesc;
        stagingDesc.size = 512;
        stagingDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        wgpu::Buffer stagingBuffer = device.CreateBuffer(&stagingDesc);

        // Copy debug buffer to staging buffer
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToBuffer(debugBuffer, 0, stagingBuffer, 0, 512);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // Ensure all submitted work is done before mapping
        wgpu::Future workDoneFuture =
            queue.OnSubmittedWorkDone(wgpu::CallbackMode::WaitAnyOnly,
                                      [](wgpu::QueueWorkDoneStatus status, wgpu::StringView) {
                                          ASSERT_EQ(status, wgpu::QueueWorkDoneStatus::Success);
                                      });
        wgpu::FutureWaitInfo workDoneWaitInfo = {workDoneFuture};
        GetInstance().WaitAny(1, &workDoneWaitInfo, UINT64_MAX);
        ASSERT_TRUE(workDoneWaitInfo.completed);

        // Wait for completion and map buffer
        auto callback = [](wgpu::MapAsyncStatus status, wgpu::StringView) {
            ASSERT_EQ(status, wgpu::MapAsyncStatus::Success);
        };

        wgpu::Future future = stagingBuffer.MapAsync(wgpu::MapMode::Read, 0, 512,
                                                     wgpu::CallbackMode::WaitAnyOnly, callback);
        wgpu::FutureWaitInfo waitInfo = {future};
        GetInstance().WaitAny(1, &waitInfo, UINT64_MAX);
        ASSERT_TRUE(waitInfo.completed);

        const uint32_t* data = static_cast<const uint32_t*>(stagingBuffer.GetConstMappedRange());

        // Debug info structure: failedIndex[32], expectedValue[32], actualValue[32], failureCount
        const uint32_t* failedIndex = &data[0];
        const uint32_t* expectedValue = &data[32];
        const uint32_t* actualValue = &data[64];
        uint32_t failureCount = data[96];

        printf("\n=== Debug Info for %s ===\n", testName);
        printf("Total failures: %u\n", failureCount);

        for (uint32_t i = 0; i < failureCount && i < 32; i++) {
            uint32_t failedIdx = failedIndex[i];
            uint32_t expected = expectedValue[i];
            uint32_t actual = actualValue[i];

            if (failedIdx < 10) {  // Assume first 10 indices are buffer lengths
                printf("  Failure %u: Buffer[%u] length - Expected: %u, Actual: %u\n", i, failedIdx,
                       expected, actual);
            } else {
                printf("  Failure %u: Immediate[%u] value - Expected: %u, Actual: %u\n", i,
                       failedIdx - 10, expected, actual);
            }
        }
        printf("========================\n\n");

        stagingBuffer.Unmap();
    }

    wgpu::Buffer mStorageBuffer64;
    wgpu::Buffer mStorageBuffer256;
    wgpu::Buffer mStorageBuffer512;
    wgpu::Buffer mResultBuffer;
    wgpu::Texture mRenderTarget;
    wgpu::TextureView mRenderTargetView;

    // Helper to check if render target shows success (green) or failure (red)
    void ExpectRenderTargetColor(bool expectSuccess) {
        if (expectSuccess) {
            // Expect green color (0, 255, 0, 255)
            utils::RGBA8 expectedColor(0, 255, 0, 255);
            EXPECT_PIXEL_RGBA8_EQ(expectedColor, mRenderTarget, 0, 0);
        } else {
            // Expect red color (255, 0, 0, 255)
            utils::RGBA8 expectedColor(255, 0, 0, 255);
            EXPECT_PIXEL_RGBA8_EQ(expectedColor, mRenderTarget, 0, 0);
        }
    }

    // Helper to create debug result buffer for detailed failure reporting
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
    // Expected values for validation
    std::vector<uint32_t> expectedBufferLengths = {
        16, 64};  // mStorageBuffer64=16, mStorageBuffer256=64
    std::vector<uint32_t> expectedImmediates = {789, 987, 654};

    wgpu::RenderPipeline pipeline =
        MakeRenderPipelineWithValidation(2, 3, expectedBufferLengths, expectedImmediates);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);

    // Set immediate data - struct with 3 u32 values
    uint32_t immediateData[] = {789, 987, 654};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData), sizeof(immediateData));

    pass.Draw(1);  // Draw 1 point
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Check that render target shows green (success)
    ExpectRenderTargetColor(true);
}

// Test: Render pipeline with multiple draws and pipeline changes
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineChangeWithDifferentImmediates) {
    // Expected values for the final (second) draw
    std::vector<uint32_t> expectedBufferLengths = {16, 64};               // Same buffer layout
    std::vector<uint32_t> expectedImmediates = {1300, 1400, 1500, 1600};  // Final immediate values

    // Create pipelines: first simple (no validation), second with validation for final check
    wgpu::RenderPipeline pipeline1 = MakeSimpleRenderPipeline(2, 2);  // No validation
    wgpu::RenderPipeline pipeline2 = MakeRenderPipelineWithValidation(
        2, 4, expectedBufferLengths, expectedImmediates);  // With validation

    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with pipeline1 (no validation, just draws green)
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup);

    uint32_t immediateData1[] = {1100, 1200};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Change to pipeline2 with more immediates - buffer lengths should still be correct
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup);

    uint32_t immediateData2[] = {1300, 1400, 1500, 1600};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);  // This draw has validation enabled and should show the final result

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Check that render target shows green (success) from the final draw with validation
    ExpectRenderTargetColor(true);
}

// Test: Render pipeline with buffer count changes
TEST_P(ImmediateDataBufferLengthTest, RenderPipelinePipelineAndBindGroupChange) {
    // Expected values for the final (second) draw with 3 buffers
    std::vector<uint32_t> expectedBufferLengths = {16, 64, 128};  // 3 buffers in final draw
    std::vector<uint32_t> expectedImmediates = {2333, 2444};      // Final immediate values

    // Create pipelines: first simple (no validation), second with validation for final check
    wgpu::RenderPipeline pipeline1 = MakeSimpleRenderPipeline(2, 2);  // No validation
    wgpu::RenderPipeline pipeline2 = MakeRenderPipelineWithValidation(
        3, 2, expectedBufferLengths, expectedImmediates);  // With validation

    wgpu::BindGroup storageBindGroup1 =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));
    wgpu::BindGroup storageBindGroup2 =
        MakeBindGroup3BuffersForRender(MakeStorageBindGroupLayoutForRender(3));

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    // First draw with 2 buffers (no validation)
    pass.SetPipeline(pipeline1);
    pass.SetBindGroup(0, storageBindGroup1);

    uint32_t immediateData1[] = {2111, 2222};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));
    pass.Draw(1);

    // Change to pipeline with 3 buffers and update bind group
    pass.SetPipeline(pipeline2);
    pass.SetBindGroup(0, storageBindGroup2);  // Now has 3 buffers

    uint32_t immediateData2[] = {2333, 2444};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));
    pass.Draw(1);  // This draw has validation enabled and should show the final result

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Check that render target shows green (success) from the final draw with validation
    ExpectRenderTargetColor(true);
}

// Test: Render pipeline with multiple immediate data updates
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineMultipleImmediateDataUpdates) {
    // Expected values for the final draw (should use the last set immediate data)
    std::vector<uint32_t> expectedBufferLengths = {
        16, 64};  // mStorageBuffer64=16, mStorageBuffer256=64
    std::vector<uint32_t> expectedImmediates = {3070, 3080, 3090};  // Final immediate values

    // Single pipeline with validation and debug info enabled
    wgpu::RenderPipeline pipeline =
        MakeRenderPipelineWithValidation(2, 3, expectedBufferLengths, expectedImmediates, true);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));

    // Create debug buffer and bind group for detailed failure reporting
    wgpu::Buffer debugBuffer = CreateDebugBuffer();
    wgpu::BindGroup debugBindGroup = CreateDebugBindGroup(debugBuffer);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, debugBindGroup);  // Bind debug buffer for validation info

    // Set immediate data multiple times - only the last one should take effect
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

    // Check that render target shows green (success)
    ExpectRenderTargetColor(true);

    // Print debug info to see what's failing
    PrintDebugInfo(debugBuffer, "RenderPipelineMultipleImmediateDataUpdates");
}

// Test: Render pipeline with interleaved updates
TEST_P(ImmediateDataBufferLengthTest, RenderPipelineInterleavedUpdates) {
    // Expected values for the final draw
    std::vector<uint32_t> expectedBufferLengths = {
        16, 64};  // mStorageBuffer64=16, mStorageBuffer256=64
    std::vector<uint32_t> expectedImmediates = {6000, 7000};  // Final immediate values

    // Single pipeline with validation and debug info enabled
    wgpu::RenderPipeline pipeline =
        MakeRenderPipelineWithValidation(2, 2, expectedBufferLengths, expectedImmediates, true);
    wgpu::BindGroup storageBindGroup =
        MakeBindGroup2BuffersForRender(MakeStorageBindGroupLayoutForRender(2));

    // Create debug buffer and bind group for detailed failure reporting
    wgpu::Buffer debugBuffer = CreateDebugBuffer();
    wgpu::BindGroup debugBindGroup = CreateDebugBindGroup(debugBuffer);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

    utils::ComboRenderPassDescriptor renderPassDesc({mRenderTargetView});
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDesc);

    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, storageBindGroup);
    pass.SetBindGroup(1, debugBindGroup);  // Bind debug buffer for validation info

    // Update immediate data multiple times to test that the last one takes effect
    uint32_t immediateData1[] = {4000, 5000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData1), sizeof(immediateData1));

    uint32_t immediateData2[] = {6000, 7000};
    pass.SetImmediateData(0, reinterpret_cast<uint8_t*>(immediateData2), sizeof(immediateData2));

    pass.Draw(1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Check that render target shows green (success)
    ExpectRenderTargetColor(true);

    // Print debug info to see what's failing
    PrintDebugInfo(debugBuffer, "RenderPipelineInterleavedUpdates");
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
