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

#include "dawn/native/webgpu/Recorder.h"

#include "dawn/native/Device.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::webgpu {

bool Recorder::GetId(CapturedObject* object, ObjectId* out) {
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    object->id = id;
    *out = id;
    return true;
}

void Recorder::SnapshotGlobalState(Device* device,
                                   devtool::cmd::pb::SnapshotGlobalState* globalState) {
    ObjectId deviceId;
    GetId(device, &deviceId);
    globalState->mutable_device()->set_id(deviceId);
    // For each ObjectType, get the object tracking list and assign an ObjectId
    device->GetObjectTrackingList(native::ObjectType::Queue)
        ->ForEach([&](const native::ApiObjectBase* object) {
            ObjectId id;
            auto mutableObject = const_cast<native::ApiObjectBase*>(object);
            GetId(static_cast<native::webgpu::ObjectWGPU<WGPUQueue>*>(
                      native::webgpu::ToBackend(static_cast<native::QueueBase*>(mutableObject))),
                  &id);
            auto state = globalState->add_queues();
            state->set_id(id);
        });
    device->GetObjectTrackingList(native::ObjectType::Buffer)
        ->ForEach([&](const native::ApiObjectBase* object) {
            ObjectId id;
            auto mutableObject = const_cast<native::ApiObjectBase*>(object);
            GetId(static_cast<native::webgpu::ObjectWGPU<WGPUBuffer>*>(
                      native::webgpu::ToBackend(static_cast<native::BufferBase*>(mutableObject))),
                  &id);
            auto state = globalState->add_buffers();
            state->set_id(id);
        });
    // TODO(crbug.com/413053623): Finish for all ObjectType
}

void Recorder::StartCapture(Device* device) {
    auto globalState = mBlock.mutable_globalstate();
    SnapshotGlobalState(device, globalState);
}

void Recorder::End(std::ostream& out) {
    mBlock.SerializeToOstream(&out);
}

void Recorder::EncodeDeviceCreateBuffer(
    Device* device,
    Buffer* buffer,
    const native::UnpackedPtr<native::BufferDescriptor>& descriptor) {
    devtool::cmd::pb::Cmd* cmd = mBlock.add_cmds();
    devtool::cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd->mutable_devicecreatebuffer();

    ObjectId deviceId = device->id;
    createBufferCmd->set_deviceid(deviceId);

    devtool::cmd::pb::BufferDescriptor* pbDesc = createBufferCmd->mutable_bufferdescriptor();
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

void Recorder::EncodeQueueWriteBuffer(Queue* queue,
                                      Buffer* buffer,
                                      uint64_t bufferOffset,
                                      const void* data,
                                      size_t size) {
    devtool::cmd::pb::QueueWriteBufferCmd* cmd = mBlock.add_cmds()->mutable_queuewritebuffer();

    cmd->set_queueid(queue->id);
    cmd->set_bufferid(buffer->id);
    cmd->set_bufferoffset(bufferOffset);
    cmd->set_data(data, size);
}

}  // namespace dawn::native::webgpu
