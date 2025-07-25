
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

#include <fstream>
#include <string>
#include "include/dawn/devtool/Playback.h"
#include "include/dawn/native/WebGPUBackend.h"

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class RecordPlaybackTests : public DawnTest {};

TEST_P(RecordPlaybackTests, QueueWriteBuffer) {
    WGPUDevice cDevice = device.Get();
    native::webgpu::StartRecording(cDevice);
    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);

    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer1, 0, &value, sizeof(value));
    EXPECT_BUFFER_U32_EQ(value, buffer1, 0);
    const std::string kFilename = "out/Debug/captured4.dawn";
    std::ofstream out(kFilename, std::ios::binary);
    if (!out.is_open()) {
        std::cout << "[RecordPlaybackTests] ofstream \"" << kFilename << "\" is not open."
                  << std::endl;
        return;
    }
    native::webgpu::EndRecording(cDevice, out);
    out.close();

    std::ifstream in(kFilename, std::ios::binary);
    if (!in.is_open()) {
        std::cout << "[RecordPlaybackTests] ifstream \"" << kFilename << "\" is not open."
                  << std::endl;
        return;
    }
    auto replayer = devtool::StartPlayback(cDevice, in);
    in.close();

    WGPUBuffer buffer1Replay = devtool::GetResource(replayer.get(), cDevice, 2);
    EXPECT_BUFFER_U32_EQ(value, buffer1Replay, 0);
}
DAWN_INSTANTIATE_TEST(RecordPlaybackTests, WebGPUBackend());
}  // anonymous namespace
}  // namespace dawn
