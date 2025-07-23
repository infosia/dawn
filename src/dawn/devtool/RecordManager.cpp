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

#include "dawn/devtool/RecordManager.h"

#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::devtool {

using Device = native::webgpu::Device;
using Queue = native::webgpu::Queue;
using Buffer = native::webgpu::Buffer;

bool RecordCommandSerializer::GetId(CapturedObject* object, ObjectId* out) {
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    object->id = id;
    *out = id;
    return true;
}

void RecordCommandSerializer::SnapshotGlobalState(Device* device,
                                                  cmd::pb::SnapshotGlobalState* globalState) {
    ObjectId deviceId;
    GetId(device, &deviceId);
    globalState->mutable_device()->set_id(deviceId);
    // for each device->mObjectLists type, GetId
    device->GetObjectTrackingList(ObjectType::Queue)->ForEach([&](const ApiObjectBase* object) {
        ObjectId id;
        auto mutableObject = const_cast<ApiObjectBase*>(object);
        GetId(
            // static_cast<const webgpu::ObjectWGPU<WGPUQueue>*>(webgpu::ToBackend(static_cast<const
            // QueueBase*>(queue))),
            static_cast<webgpu::ObjectWGPU<WGPUQueue>*>(
                webgpu::ToBackend(static_cast<QueueBase*>(mutableObject))),
            &id);
        auto state = globalState->add_queues();
        state->set_id(id);
    });
    device->GetObjectTrackingList(ObjectType::Buffer)->ForEach([&](const ApiObjectBase* object) {
        ObjectId id;
        auto mutableObject = const_cast<ApiObjectBase*>(object);
        GetId(static_cast<webgpu::ObjectWGPU<WGPUBuffer>*>(
                  webgpu::ToBackend(static_cast<BufferBase*>(mutableObject))),
              &id);
        auto state = globalState->add_buffers();
        state->set_id(id);
    });
    // ...
}

// void RecordCommandSerializer::StartCapture(std::string filename) {
void RecordCommandSerializer::StartCapture(Device* device) {
    auto globalState = mBlock.mutable_globalstate();
    SnapshotGlobalState(device, globalState);
}

void RecordCommandSerializer::End(std::ostream& out) {
    // mBlock.set_currentid(mCurrentId);
    mBlock.SerializeToOstream(&out);
}

void RecordCommandSerializer::EncodeDeviceCreateBuffer(
    Device* device,
    Buffer* buffer,
    const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor) {
    cmd::pb::Cmd* cmd = mBlock.add_cmds();
    cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd->mutable_devicecreatebuffer();

    ObjectId deviceId = device->id;
    createBufferCmd->set_deviceid(deviceId);

    cmd::pb::BufferDescriptor* pbDesc = createBufferCmd->mutable_bufferdescriptor();
    if (descriptor->label.data != nullptr) {
        pbDesc->set_label(descriptor->label.data, descriptor->label.length);
    }
    pbDesc->set_usage(static_cast<uint64_t>(descriptor->usage));
    pbDesc->set_size(descriptor->size);
    pbDesc->set_mappedatcreation(descriptor->mappedAtCreation);

    ObjectId bufferId;
    GetId(buffer, &bufferId);
    createBufferCmd->set_resultid(bufferId);
}

void RecordCommandSerializer::EncodeQueueWriteBuffer(Queue* queue,
                                                     Buffer* buffer,
                                                     uint64_t bufferOffset,
                                                     const void* data,
                                                     size_t size) {
    cmd::pb::QueueWriteBufferCmd* cmd = mBlock.add_cmds()->mutable_queuewritebuffer();

    cmd->set_queueid(queue->id);
    cmd->set_bufferid(buffer->id);
    cmd->set_bufferoffset(bufferOffset);
    cmd->set_data(data, size);
}

}  // namespace dawn::native::devtool
