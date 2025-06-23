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

#include "dawn/common/FutureUtils.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class RecordReplayTests : public DawnTest {};

TEST_P(RecordReplayTests, QueueWriteBuffer) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    descriptor.size = 16;
    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }

    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }
    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }
    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }
    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }
    {
        wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);
    }

    // uint32_t value = 0x01020304;
    // queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    // EXPECT_BUFFER_U32_EQ(value, buffer, 0);
}

DAWN_INSTANTIATE_TEST(RecordReplayTests, WebGPUBackend());

}  // anonymous namespace
}  // namespace dawn
