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
    bool hasDevice = playback->GetFromId(deviceId, &device);
    DAWN_ASSERT(hasDevice);

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    playback->SetId(buffer, bufferId);
}

void DecodeQueueWriteBuffer(const cmd::pb::QueueWriteBufferCmd& cmd, Replayer* playback) {
    ObjectId queueId = cmd.queueid();
    ObjectId bufferId = cmd.bufferid();

    WGPUQueue queue = nullptr;
    bool hasQueue = playback->GetFromId(queueId, &queue);
    DAWN_ASSERT(hasQueue);
    WGPUBuffer buffer = nullptr;
    bool hasBuffer = playback->GetFromId(bufferId, &buffer);
    DAWN_ASSERT(hasBuffer);

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

void Replayer::InitFromSnapshotGlobalState(WGPUDevice device,
                                           const cmd::pb::SnapshotGlobalState& globalState) {
    // link / create each existing objects in globalState before recording
    auto deviceState = globalState.device();
    SetId(device, deviceState.id());

    // Now dawn only support the default queue.
    DAWN_ASSERT(globalState.queues_size() == 1);
    auto defaultQueueState = globalState.queues(0);
    WGPUQueue queue = wgpuDeviceGetQueue(device);
    SetId(queue, defaultQueueState.id());

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
