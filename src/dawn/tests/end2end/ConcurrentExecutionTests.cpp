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

#include <numeric>
#include <string>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

// Write After Read (WAR) hazards do not require memory barriers as there is no
// visibility/availability transition required. However due to the obvious memory aliasing a
// pipeline execution barrier is absolutely required to avoid WAR.
//
// This test validates that WebGPUs implicit barrier guarantees are not violated. It does so by
// detecting write modifications during a read. This can only if the dispatches are concurrent.

namespace dawn {
namespace {

class ConcurrentExecutionTest : public DawnTest {
  public:
    wgpu::Buffer CreateBuffer(const std::vector<uint32_t>& data,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        uint64_t bufferSize = static_cast<uint64_t>(data.size() * sizeof(uint32_t));
        return utils::CreateBufferFromData(device, data.data(), bufferSize, usage);
    }

    wgpu::Buffer CreateBuffer(const uint32_t count,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        return CreateBuffer(std::vector<uint32_t>(count, 0), usage);
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

TEST_P(ConcurrentExecutionTest, CaseWriteAfterReadDependencyImplicit) {
    DAWN_TEST_UNSUPPORTED_IF(IsCompatibilityMode());
    auto kBufferSize = 100000;
    wgpu::Buffer buf_a = CreateBuffer(kBufferSize);
    wgpu::Buffer buf_b = CreateBuffer(kBufferSize);
    wgpu::Buffer buf_c = CreateBuffer(kBufferSize);
    wgpu::Buffer buf_d = CreateBuffer(kBufferSize);
    std::ostringstream shader;
    shader << R"(
@group(0) @binding(0) var<storage, read> in_ : array<u32>;
@group(0) @binding(1) var<storage, read_write> out_ : array<atomic<u32>>;

@compute @workgroup_size(1) fn main() {
   const kIterCount =)"
           << kBufferSize << R"(;
    var expected = in_[0];
    for(var i = 0i; i < kIterCount;i++){
        var read_val = in_[i];
        if(read_val != expected){
            // Another dispatch changed this buffer.
            atomicStore(&out_[0], 0xAA00+ expected);
            break;
        }
        // Reverse iter increment
        atomicAdd(&out_[(kIterCount-1) - i], 1);
    }
})";

    wgpu::ComputePipeline pipeline = CreateComputePipeline(shader.str(), "main");

    wgpu::BindGroup bindGroup_read_a_write_d =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_a}, {1, buf_d}});
    wgpu::BindGroup bindGroup_read_b_write_d =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_b}, {1, buf_d}});

    wgpu::BindGroup bindGroup_read_c_write_d =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_c}, {1, buf_d}});
    wgpu::BindGroup bindGroup_read_d_write_c =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_d}, {1, buf_c}});

    wgpu::BindGroup bindGroup_read_c_write_a =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_c}, {1, buf_a}});
    wgpu::BindGroup bindGroup_read_c_write_b =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buf_c}, {1, buf_b}});

    wgpu::CommandBuffer commands;
    {
        // Edge case:
        // Pass 1 writes to buffers A and B
        // Pass 2 reads buffer A
        // Pass 3 writes to buffer A
        // Pass 4 reads buffer B
        // Pass 5 writes to buffer B
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        // ReadWrite all
        pass.SetBindGroup(0, bindGroup_read_a_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_b_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_c_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_d_write_c);
        pass.DispatchWorkgroups(1);

        pass.SetBindGroup(0, bindGroup_read_c_write_a);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_c_write_b);
        pass.DispatchWorkgroups(1);

        pass.SetBindGroup(0, bindGroup_read_a_write_d);
        pass.DispatchWorkgroups(1);

        pass.SetBindGroup(0, bindGroup_read_c_write_a);
        pass.DispatchWorkgroups(1);

        // Read B write D
        pass.SetBindGroup(0, bindGroup_read_b_write_d);
        pass.DispatchWorkgroups(1);

        // Read C write B
        // Here is the potential hazard. We need to have a pipeline execution barrier inserted here
        // to avoid a WAR in concurrent dispatch for when we write to B following the dispatch that
        // reads from B.
        pass.SetBindGroup(0, bindGroup_read_c_write_b);
        pass.DispatchWorkgroups(1);
        pass.End();

        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);
    std::vector<uint32_t> expected_a{2};
    EXPECT_BUFFER_U32_RANGE_EQ(expected_a.data(), buf_b, 0, expected_a.size());

    std::vector<uint32_t> expected_d{5};
    EXPECT_BUFFER_U32_RANGE_EQ(expected_d.data(), buf_d, 0, expected_d.size());
}

DAWN_INSTANTIATE_TEST(ConcurrentExecutionTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      D3D12Backend({"use_dxc"}),
                      MetalBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
