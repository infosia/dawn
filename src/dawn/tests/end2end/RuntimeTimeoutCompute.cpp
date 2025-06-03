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

#include <cstdint>
#include <string>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"
#include "gtest/gtest.h"

namespace dawn {
namespace {

class RuntimeTimeoutComputeTest : public DawnTest {
  public:
    wgpu::Buffer CreateBuffer(const std::vector<uint32_t>& data,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        uint64_t bufferSize = static_cast<uint64_t>(data.size() * sizeof(uint32_t));
        return utils::CreateBufferFromData(device, data.data(), bufferSize, usage);
    }

    wgpu::Buffer CreateBuffer(const uint32_t count,
                              const uint32_t default_val = 0,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        return CreateBuffer(std::vector<uint32_t>(count, default_val), usage);
    }

    wgpu::ComputePipeline CreateComputePipeline(
        const std::string& shader,
        const char* entryPoint = nullptr,
        const std::vector<wgpu::ConstantEntry>* constants = nullptr) {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = utils::CreateShaderModule(device, shader.c_str());
        csDesc.compute.entryPoint = entryPoint;
        if (constants) {
            csDesc.compute.constants = constants->data();
            csDesc.compute.constantCount = constants->size();
        }
        return device.CreateComputePipeline(&csDesc);
    }
};

// This test will take up to 5+ seconds to execute if we do not have module constant hoisting
// (transform) enabled on Apple silicon. See crbug.com/413690572
TEST_P(RuntimeTimeoutComputeTest, LargeConstantArrayInLoop) {
    DAWN_SKIP_TEST_IF_BASE(IsCPU() || IsWARP(), "timeout",
                           "skipping because software rendering is slow");
    // Test disable due to CI failures. See crbug.com/421869294
    DAWN_SKIP_TEST_IF_BASE(IsMetal() && IsIntelGen9(), "timeout", "device found to be slow on CI");

    std::string kShaderCode = R"(


    struct Output {
  @size(4) value : u32
};
@group(0) @binding(0) var<storage, read_write> outputs : array<Output, 3>;



const values = array(
  array<bool, 3>(true, false, true)[0u],
  array<bool, 3>(true, false, true)[1u],
  array<bool, 3>(true, false, true)[2u]
);



@compute @workgroup_size(1)
fn main() {

  for (var i = 0u; i < 3; i++) {
    outputs[i].value = select(0u, 1u, values[i]);
  }
}
    )";

    StartTestTimer(0.5f);
    wgpu::ComputePipeline pipeline = CreateComputePipeline(kShaderCode);
    uint32_t kDefaultVal = 0;
    uint32_t kDispatchSizeDim = 64;
    uint32_t kBufferNumElements = 256 * kDispatchSizeDim;
    wgpu::Buffer output = CreateBuffer(kBufferNumElements, kDefaultVal);
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, output}});

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);

        pass.DispatchWorkgroups(kDispatchSizeDim, kDispatchSizeDim, kDispatchSizeDim);
        pass.End();
        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);
    std::vector<uint32_t> expected = {1, 0, 1};
    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), output, 0, expected.size());
}

DAWN_INSTANTIATE_TEST(RuntimeTimeoutComputeTest, D3D12Backend(), MetalBackend(), VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
