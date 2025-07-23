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

#include "dawn/native/devtool/RecordPlaybackManager.h"

#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::devtool {

using Device = webgpu::Device;
using Queue = webgpu::Queue;
using Buffer = webgpu::Buffer;

namespace {

// Command decode functions

void DecodeDeviceCreateBuffer(const cmd::pb::DeviceCreateBufferCmd& cmd,
                              ObjectIdManager* objectIdManager) {
    ObjectId deviceId = cmd.deviceid();
    ObjectId bufferId = cmd.resultid();

    const cmd::pb::BufferDescriptor& pbDesc = cmd.bufferdescriptor();

    BufferDescriptor descriptor = {};
    descriptor.label = static_cast<std::string_view>(pbDesc.label());
    descriptor.size = pbDesc.size();
    descriptor.usage = static_cast<wgpu::BufferUsage>(pbDesc.usage());
    descriptor.mappedAtCreation = pbDesc.mappedatcreation();

    CapturedObject* capturedDevice = nullptr;
    objectIdManager->GetFromId(deviceId, &capturedDevice);
    Device* device = static_cast<Device*>(capturedDevice);

    Buffer* buffer = webgpu::ToBackend(device->APICreateBuffer(&descriptor));

    buffer->id = bufferId;
    objectIdManager->SetId(buffer, bufferId);
}

void DecodeQueueWriteBuffer(const cmd::pb::QueueWriteBufferCmd& cmd,
                            ObjectIdManager* objectIdManager) {
    ObjectId queueId = cmd.queueid();
    ObjectId bufferId = cmd.bufferid();

    CapturedObject* capturedQueue = nullptr;
    objectIdManager->GetFromId(queueId, &capturedQueue);
    Queue* queue = static_cast<Queue*>(capturedQueue);
    CapturedObject* capturedBuffer = nullptr;
    objectIdManager->GetFromId(bufferId, &capturedBuffer);
    Buffer* buffer = static_cast<Buffer*>(capturedBuffer);

    queue->APIWriteBuffer(buffer, cmd.bufferoffset(), cmd.data().c_str(), cmd.data().size());
}

void DecodeCmd(const cmd::pb::Cmd& cmd, ObjectIdManager* objectIdManager) {
    switch (cmd.type_case()) {
        case cmd::pb::Cmd::TypeCase::kDeviceCreateBuffer: {
            DecodeDeviceCreateBuffer(cmd.devicecreatebuffer(), objectIdManager);
            break;
        }
        case cmd::pb::Cmd::TypeCase::kQueueWriteBuffer: {
            DecodeQueueWriteBuffer(cmd.queuewritebuffer(), objectIdManager);
            break;
        }
        default:
            break;
    }
}

}  // anonymous namespace

bool ObjectIdManager::GetId(CapturedObject* object, ObjectId* out) {
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    object->id = id;
    *out = id;
    return true;
}

bool ObjectIdManager::GetFromId(ObjectId id, CapturedObject** out) const {
    auto result = mObjectIdMaps.find(id);
    if (result == mObjectIdMaps.end()) {
        return false;
    }
    *out = result->second;
    return true;
}

bool ObjectIdManager::SetId(CapturedObject* object, ObjectId id) {
    mObjectIdMaps.emplace(id, object);
    return true;
}

RecordCommandSerializer::RecordCommandSerializer(Device* device) {
    std::cout << "[RecordCommandSerializer] Initialized." << std::endl;
    // TODO(crbug.com/413053623): capture device and default queue states.
    ObjectId deviceId;
    mObjectIdManager.GetId(device, &deviceId);
    ObjectId queueId;
    mObjectIdManager.GetId(static_cast<CapturedObject*>(webgpu::ToBackend(device->GetQueue())),
                           &queueId);
}

RecordCommandSerializer::~RecordCommandSerializer() {
    std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
}

// void RecordCommandSerializer::StartCapture(std::string filename) {
void RecordCommandSerializer::StartCapture() {
    // mOstream = std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::binary);
}

void RecordCommandSerializer::End(std::ostream& out) {
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
    mObjectIdManager.GetId(buffer, &bufferId);
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

PlaybackCommandDeserializer::PlaybackCommandDeserializer(Device* device) {
    std::cout << "[PlaybackCommandDeserializer] Initialized." << std::endl;
    // mOstream = &std::cout;

    // TEMP: actually the same device being used as the recorder?
    mObjectIdManager.SetId(device, device->id);
    Queue* queue = webgpu::ToBackend(device->GetQueue());
    mObjectIdManager.SetId(queue, queue->id);

    // ObjectId deviceId;
    // mObjectIdManager.GetId(device, &deviceId);
}
PlaybackCommandDeserializer::~PlaybackCommandDeserializer() {
    std::cout << "[PlaybackCommandDeserializer] Destroyed." << std::endl;
}

void PlaybackCommandDeserializer::StartPlayback(std::istream& in) {
    mBlock.ParseFromIstream(&in);
    for (int i = 0; i < mBlock.cmds_size(); i++) {
        DecodeCmd(mBlock.cmds(i), &mObjectIdManager);
    }
}

void PlaybackCommandDeserializer::End() {}

}  // namespace dawn::native::devtool
