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

#include "dawn/native/devtool/PlaybackManager.h"

#include <webgpu/webgpu.h>
// #include <webgpu/webgpu_cpp.h>

#include "dawn/native/Buffer.h"
#include "dawn/native/Device.h"
#include "dawn/native/Queue.h"
// #include "dawn/native/webgpu/BufferWGPU.h"
// #include "dawn/native/webgpu/DeviceWGPU.h"
// #include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::devtool {

// using Device = webgpu::Device;
// using Queue = webgpu::Queue;
// using Buffer = webgpu::Buffer;

namespace {

// Command decode functions

void DecodeDeviceCreateBuffer(const cmd::pb::DeviceCreateBufferCmd& cmd,
                              PlaybackCommandDeserializer* playback) {
    ObjectId deviceId = cmd.deviceid();
    ObjectId bufferId = cmd.resultid();

    const cmd::pb::BufferDescriptor& pbDesc = cmd.bufferdescriptor();

    BufferDescriptor descriptor = {};
    descriptor.label = static_cast<std::string_view>(pbDesc.label());
    descriptor.size = pbDesc.size();
    descriptor.usage = static_cast<wgpu::BufferUsage>(pbDesc.usage());
    descriptor.mappedAtCreation = pbDesc.mappedatcreation();

    void* capturedDevice = nullptr;
    playback->GetFromId(deviceId, &capturedDevice);
    DeviceBase* device = reinterpret_cast<DeviceBase*>(capturedDevice);

    // Buffer* buffer = webgpu::ToBackend(device->APICreateBuffer(&descriptor));
    // buffer->id = bufferId;

    BufferBase* buffer = device->APICreateBuffer(&descriptor);

    playback->SetId(buffer, bufferId);
}

void DecodeQueueWriteBuffer(const cmd::pb::QueueWriteBufferCmd& cmd,
                            PlaybackCommandDeserializer* playback) {
    ObjectId queueId = cmd.queueid();
    ObjectId bufferId = cmd.bufferid();

    void* capturedQueue = nullptr;
    playback->GetFromId(queueId, &capturedQueue);
    QueueBase* queue = reinterpret_cast<QueueBase*>(capturedQueue);
    void* capturedBuffer = nullptr;
    playback->GetFromId(bufferId, &capturedBuffer);
    BufferBase* buffer = reinterpret_cast<BufferBase*>(capturedBuffer);

    queue->APIWriteBuffer(buffer, cmd.bufferoffset(), cmd.data().c_str(), cmd.data().size());
}

void DecodeCmd(const cmd::pb::Cmd& cmd, PlaybackCommandDeserializer* playback) {
    switch (cmd.type_case()) {
        case cmd::pb::Cmd::TypeCase::kDeviceCreateBuffer: {
            DecodeDeviceCreateBuffer(cmd.devicecreatebuffer(), playback);
            break;
        }
        case cmd::pb::Cmd::TypeCase::kQueueWriteBuffer: {
            DecodeQueueWriteBuffer(cmd.queuewritebuffer(), playback);
            break;
        }
        default:
            break;
    }
}

}  // anonymous namespace

// PlaybackCommandDeserializer::PlaybackCommandDeserializer(Device* device)
// // : mObjectIdManager(static_cast<DeviceBase*>(device))
// {}

bool PlaybackCommandDeserializer::GetFromId(ObjectId id, void** out) const {
    auto result = mObjectIdMaps.find(id);
    if (result == mObjectIdMaps.end()) {
        return false;
    }
    *out = result->second;
    return true;
}

bool PlaybackCommandDeserializer::SetId(void* object, ObjectId id) {
    mObjectIdMaps.emplace(id, object);
    return true;
}

void PlaybackCommandDeserializer::InitFromSnapshotGlobalState(
    DeviceBase* device,
    const cmd::pb::SnapshotGlobalState& globalState) {
    // link / create each existing objects in globalState before recording

    auto deviceState = globalState.device();
    SetId(device, deviceState.id());

    // Now dawn only support the default queue.
    DAWN_ASSERT(globalState.queues_size() == 1);
    auto defaultQueueState = globalState.queues(0);
    QueueBase* queue = device->GetQueue();
    SetId(queue, defaultQueueState.id());

    for (int i = 0; i < globalState.buffers_size(); i++) {
        // TODO(crbug.com/413053623): create existing buffers
    }
}

void PlaybackCommandDeserializer::StartPlayback(DeviceBase* device, std::istream& in) {
    mBlock.ParseFromIstream(&in);

    InitFromSnapshotGlobalState(device, mBlock.globalstate());

    for (int i = 0; i < mBlock.cmds_size(); i++) {
        DecodeCmd(mBlock.cmds(i), this);
    }
}

void PlaybackCommandDeserializer::End() {}

}  // namespace dawn::native::devtool
