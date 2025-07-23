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

#include "dawn/devtool/Replayer.h"

#include <webgpu/webgpu.h>
#include "dawn/common/Assert.h"
#include "dawn/common/StringViewUtils.h"

namespace dawn::devtool {

namespace {

// static command decode functions

// TODO(crbug.com/413053623): Use codegen to some degree if possible.

void DecodeDeviceCreateBuffer(const cmd::pb::DeviceCreateBufferCmd& cmd, Replayer* playback) {
    ObjectId deviceId = cmd.deviceid();
    ObjectId bufferId = cmd.resultid();

    const cmd::pb::BufferDescriptor& pbDesc = cmd.bufferdescriptor();

    WGPUBufferDescriptor descriptor = {};
    descriptor.label = ToOutputStringView(static_cast<std::string_view>(pbDesc.label()));
    descriptor.size = pbDesc.size();
    descriptor.usage = static_cast<WGPUBufferUsage>(pbDesc.usage());
    descriptor.mappedAtCreation = pbDesc.mappedatcreation();

    WGPUDevice device = nullptr;
    playback->GetFromId<ObjectType::Device>(deviceId, &device);

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    playback->SetId<ObjectType::Buffer>(buffer, bufferId);
}

void DecodeQueueWriteBuffer(const cmd::pb::QueueWriteBufferCmd& cmd, Replayer* playback) {
    ObjectId queueId = cmd.queueid();
    ObjectId bufferId = cmd.bufferid();

    WGPUQueue queue = nullptr;
    playback->GetFromId<ObjectType::Queue>(queueId, &queue);
    WGPUBuffer buffer = nullptr;
    playback->GetFromId<ObjectType::Buffer>(bufferId, &buffer);

    wgpuQueueWriteBuffer(queue, buffer, cmd.bufferoffset(), cmd.data().c_str(), cmd.data().size());
}

void DecodeCmd(const cmd::pb::Cmd& cmd, Replayer* playback) {
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

template <ObjectType type, typename WGPUHandle>
bool Replayer::GetFromId(ObjectId id, WGPUHandle* out) const {
    const auto& map = mObjectIdMaps[type];
    auto result = map.find(id);
    if (result == map.end()) {
        return false;
    }
    *out = reinterpret_cast<WGPUHandle>(result->second);
    return true;
}

template <ObjectType type, typename WGPUHandle>
bool Replayer::SetId(WGPUHandle object, ObjectId id) {
    mObjectIdMaps[type].emplace(id, object);
    return true;
}

// TODO(crbug.com/413053623): Use codegen.

template bool Replayer::GetFromId<ObjectType::Buffer, WGPUBuffer>(ObjectId, WGPUBuffer*) const;
template bool Replayer::GetFromId<ObjectType::Queue, WGPUQueue>(ObjectId, WGPUQueue*) const;
template bool Replayer::GetFromId<ObjectType::Device, WGPUDevice>(ObjectId, WGPUDevice*) const;

template bool Replayer::SetId<ObjectType::Buffer, WGPUBuffer>(WGPUBuffer, ObjectId);
template bool Replayer::SetId<ObjectType::Queue, WGPUQueue>(WGPUQueue, ObjectId);
template bool Replayer::SetId<ObjectType::Device, WGPUDevice>(WGPUDevice, ObjectId);

void Replayer::InitFromSnapshotGlobalState(WGPUDevice device,
                                           const cmd::pb::SnapshotGlobalState& globalState) {
    // link / create each existing objects in globalState before recording
    auto deviceState = globalState.device();
    SetId<ObjectType::Device>(device, deviceState.id());

    // Now dawn only support the default queue.
    DAWN_ASSERT(globalState.queues_size() == 1);
    auto defaultQueueState = globalState.queues(0);
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    SetId<ObjectType::Queue>(queue, defaultQueueState.id());

    for (int i = 0; i < globalState.buffers_size(); i++) {
        // TODO(crbug.com/413053623): create existing buffers
    }
}

void Replayer::StartPlayback(WGPUDevice device, std::istream& in) {
    mBlock.ParseFromIstream(&in);

    InitFromSnapshotGlobalState(device, mBlock.globalstate());

    for (int i = 0; i < mBlock.cmds_size(); i++) {
        DecodeCmd(mBlock.cmds(i), this);
    }
}

void Replayer::End() {}

}  // namespace dawn::devtool
