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
// visibility/availability transition required. However due to the obvious memory aliasing, a
// pipeline execution barrier is absolutely required to avoid WAR.
//
// This test validates that WebGPUs implicit barrier guarantees are not violated. It does so by
// detecting write modifications during a read. This can only happen if the dispatches are
// concurrent.
// Original simplified WAR
//      Edge case:
//      STEP 1: Dispatch 1 writes to buffers A and B
//      STEP 2: Dispatch 2 reads buffer A
//      STEP 3: Dispatch 3 writes to buffer A
//      STEP 4: Dispatch 4 reads buffer B
//      STEP 5: Dispatch 5 writes to buffer B
//
// The original simplified WAR cannot be executed directly as we need to write to buffers to have
// any side effects visible outside of the compute shader (i.e., in a buffer)

namespace dawn {
namespace {

class ConcurrentExecutionTest : public DawnTest {
  public:
    wgpu::Buffer CreateBuffer(const uint32_t count,
                              wgpu::BufferUsage usage = wgpu::BufferUsage::Storage |
                                                        wgpu::BufferUsage::CopySrc) {
        wgpu::BufferDescriptor descriptor;
        descriptor.size = count * sizeof(uint32_t);
        descriptor.usage = usage;
        return device.CreateBuffer(&descriptor);
    }

    wgpu::ComputePipeline CreateComputePipeline(const std::string& shader) {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = utils::CreateShaderModule(device, shader.c_str());
        return device.CreateComputePipeline(&csDesc);
    }
};

TEST_P(ConcurrentExecutionTest, CaseWriteAfterReadDependencyImplicit) {
    DAWN_TEST_UNSUPPORTED_IF(IsCompatibilityMode());
    auto kBufferSize = 100000;
    // Zero initialize each buffer.
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
            // Another dispatch changed this buffer. Record this event with a large constant.
            atomicStore(&out_[0], 0xAA00+ expected);
            break;
        }
        // Reverse iterator increment
        atomicAdd(&out_[(kIterCount-1) - i], 1);
    }
})";

    wgpu::ComputePipeline pipeline = CreateComputePipeline(shader.str());

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
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        // STEP 1 (from comment block above)
        // ReadWrite all. This may internally transition buffers from transfer.
        pass.SetBindGroup(0, bindGroup_read_a_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_b_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_c_write_d);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_d_write_c);
        pass.DispatchWorkgroups(1);

        // Write both a, b to ensure consistent state
        pass.SetBindGroup(0, bindGroup_read_c_write_a);
        pass.DispatchWorkgroups(1);
        pass.SetBindGroup(0, bindGroup_read_c_write_b);
        pass.DispatchWorkgroups(1);

        // STEP 2 (from comment block above)
        pass.SetBindGroup(0, bindGroup_read_a_write_d);
        pass.DispatchWorkgroups(1);

        // STEP 3 (from comment block above)
        pass.SetBindGroup(0, bindGroup_read_c_write_a);
        pass.DispatchWorkgroups(1);

        // STEP 4 (from comment block above)
        // Read B write D
        pass.SetBindGroup(0, bindGroup_read_b_write_d);
        pass.DispatchWorkgroups(1);

        // STEP 5 (from comment block above)
        // Read C write B
        // Here is the potential hazard. We need to have a pipeline execution barrier inserted here
        // to avoid a WAR in concurrent dispatch for when we write to B following the dispatch that
        // reads from B.
        //
        // If the backend implementation in dawn does not properly insert a barrier between these
        // dispatches we will end up concurrently writing in one dispatch while reading in the
        // previous dispatch. The test will detect this and fail.
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
