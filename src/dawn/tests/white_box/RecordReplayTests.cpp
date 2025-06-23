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
// #include "dawn/native/WebGPUBackend.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

// Probably a white_box test

class RecordReplayTests : public DawnTest {};

TEST_P(RecordReplayTests, QueueWriteBuffer) {
    native::webgpu::Device* deviceWGPU = native::webgpu::ToBackend(native::FromAPI(device.Get()));
    deviceWGPU->SetRecord(std::make_unique<native::webgpu::RecordCommandSerializer>(deviceWGPU));
    auto record = deviceWGPU->GetRecordCommandSerializer();
    record->StartCapture();

    wgpu::BufferDescriptor descriptor;
    descriptor.size = 4;
    descriptor.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer buffer1 = device.CreateBuffer(&descriptor);

    // descriptor.size = 16;
    // wgpu::Buffer buffer2 = device.CreateBuffer(&descriptor);

    uint32_t value = 0x01020304;
    queue.WriteBuffer(buffer1, 0, &value, sizeof(value));

    EXPECT_BUFFER_U32_EQ(value, buffer1, 0);


    const std::string kCapturedFilename = "out/Debug/captured1.dawn";
    record->End(kCapturedFilename);

    deviceWGPU->SetReplay(
        std::make_unique<native::webgpu::PlaybackCommandDeserializer>(deviceWGPU));
    auto playback = deviceWGPU->GetPlaybackCommandDeserializer();
    playback->StartPlayback(kCapturedFilename);
    // playback->StartPlayback("out/Debug/captured2.dawn");

    // temp
    // device - 1
    // queue - 2
    // buffer - 3
    native::webgpu::ObjectId capturedBuffer1Id = 3;
    native::webgpu::CapturedObject* capturedBuffer1;
    playback->mObjectIdManager.GetFromId(capturedBuffer1Id, &capturedBuffer1);
    native::webgpu::Buffer* buffer1Replay = static_cast<native::webgpu::Buffer*>(capturedBuffer1);



    EXPECT_BUFFER_U32_EQ(value, native::ToAPI(buffer1Replay), 0);

    // uint32_t value = 0x01020304;
    // queue.WriteBuffer(buffer, 0, &value, sizeof(value));

    // EXPECT_BUFFER_U32_EQ(value, buffer, 0);
}

DAWN_INSTANTIATE_TEST(RecordReplayTests, WebGPUBackend());

}  // anonymous namespace
}  // namespace dawn
