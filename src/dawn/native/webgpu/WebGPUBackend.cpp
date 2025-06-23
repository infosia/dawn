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

// WebGPUBackend.cpp: contains the definition of symbols exported by WebGPUBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

#include "dawn/native/WebGPUBackend.h"

#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::webgpu {

DAWN_NATIVE_EXPORT void StartRecording(WGPUDevice device) {
    Device* deviceWGPU = ToBackend(FromAPI(device));
    deviceWGPU->SetRecord(std::make_unique<native::webgpu::RecordCommandSerializer>(deviceWGPU));
    // auto record = deviceWGPU->GetRecordCommandSerializer();
    // record->StartCapture();
}

DAWN_NATIVE_EXPORT void EndRecording(WGPUDevice device, std::ostream& out) {
    Device* deviceWGPU = ToBackend(FromAPI(device));
    auto record = deviceWGPU->GetRecordCommandSerializer();
    record->End(out);
}

DAWN_NATIVE_EXPORT void StartPlayback(WGPUDevice device, std::istream& in) {
    Device* deviceWGPU = ToBackend(FromAPI(device));
    deviceWGPU->SetReplay(
        std::make_unique<native::webgpu::PlaybackCommandDeserializer>(deviceWGPU));
    auto playback = deviceWGPU->GetPlaybackCommandDeserializer();
    playback->StartPlayback(in);
}

DAWN_NATIVE_EXPORT WGPUBuffer GetResource(WGPUDevice device, uint32_t id) {
    // temp
    Device* deviceWGPU = ToBackend(FromAPI(device));
    auto playback = deviceWGPU->GetPlaybackCommandDeserializer();
    native::webgpu::CapturedObject* capturedBuffer;
    playback->mObjectIdManager.GetFromId(id, &capturedBuffer);
    native::webgpu::Buffer* buffer = static_cast<native::webgpu::Buffer*>(capturedBuffer);
    return ToAPI(buffer);
}

}  // namespace dawn::native::webgpu
