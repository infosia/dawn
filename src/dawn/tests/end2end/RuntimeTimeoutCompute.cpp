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

  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ShaderF16};
    }
};

// This test will take up to 5+ seconds to execute if we do not have module constant hoisting
// (transform) enabled on Apple silicon. See crbug.com/413690572
TEST_P(RuntimeTimeoutComputeTest, LargeConstantArrayInLoop) {
    DAWN_SKIP_TEST_IF_BASE(IsCPU() || IsWARP(), "timeout",
                           "skipping because software rendering is slow");

    std::string kShaderCode = R"(
enable f16;
@group(0)@binding(0) var < storage,read_write > A: array < i32 >;
@compute @workgroup_size(32)
fn main(@builtin(local_invocation_index) g: u32) {
  var offset = g;
  var b = f16(offset);
      for (var i = 0;  i < 80; i++) {
        var n = array(
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 2.3h,15h,6h,7h,18h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.7h,5h,6h,7h,8h,9h, 13.3h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,8h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        3.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.3h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        3.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 34.1h, 22.3h,15h,6h,7h,8h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,7h,18h,9h,
        3.0h, 1.0h, 2.3h, 14.1h, 22.6h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 14.1h, 22.3h,15h,6h,7h,18h,9h,
        13.0h, 1.0h, 2.3h, 14.1h, 22.3h,5h,6h,7h,8h,9h, 13.03h, 1.0h, 2.3h, 314.1h, 22.3h,15h,6h,4h,18h,9h,
        )[i];
         b +=  n;
      }
  A[offset] = i32( b);
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
    std::vector<uint32_t> expected = {1951, 1952};
    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), output, 0, expected.size());
}

DAWN_INSTANTIATE_TEST(RuntimeTimeoutComputeTest, D3D12Backend(), MetalBackend(), VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
