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

#include "dawn/native/webgpu/RecordCommandSerializer.h"

#include <chrono>  // For std::chrono::seconds
#include <thread>  // For std::this_thread::sleep_for

#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::webgpu {

// ObjectIdProvider::GetId(WGPUDevice object, ObjectId* out) {

// }

// bool ObjectIdManager::GetId(WGPUDevice object, ObjectId* out) {
//     // TODO: if new increment id
//     ObjectId id = mCurrentId++;
//     mObjectIdMaps.emplace(id, object);
//     // store id in object
//     *out = id;
//     return true;
// }
// bool ObjectIdManager::GetId(WGPUBuffer object, ObjectId* out) {
//     ObjectId id = mCurrentId++;
//     mObjectIdMaps.emplace(id, object);
//     *out = id;
//     return true;
// }

// bool ObjectIdManager::GetFromId(ObjectId id, WGPUDevice* out) {
//     auto result = mObjectIdMaps.find(id);
//     if (result == mObjectIdMaps.end()) {
//         return false;
//     }
//     *out = reinterpret_cast<WGPUDevice>(result->second);
//     return true;
// }
// bool ObjectIdManager::GetFromId(ObjectId id, WGPUBuffer* out) {
//     auto result = mObjectIdMaps.find(id);
//     if (result == mObjectIdMaps.end()) {
//         return false;
//     }
//     *out = reinterpret_cast<WGPUBuffer>(result->second);
//     return true;
// }

bool ObjectIdManager::GetId(CapturedObject* object, ObjectId* out) {
    // TODO: if new increment id
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    // store id in object
    object->id = id;
    *out = id;
    return true;
}

// Used by objectIdResolver (deserializer) only?
bool ObjectIdManager::GetFromId(ObjectId id, CapturedObject** out) const {
    auto result = mObjectIdMaps.find(id);
    if (result == mObjectIdMaps.end()) {
        return false;
    }
    // *out = reinterpret_cast<WGPUBuffer>(result->second);
    *out = result->second;
    return true;
}

bool ObjectIdManager::SetId(CapturedObject* object, ObjectId id) {
    mObjectIdMaps.emplace(id, object);
    return true;
}

// CapturedObject* ObjectIdManager::GetResource(ObjectId id) const {
//     return mObjectIdMaps.find(id);
// }

// RecordCommandSerializer::RecordCommandSerializer(WGPUDevice device) {
// RecordCommandSerializer::RecordCommandSerializer() {
RecordCommandSerializer::RecordCommandSerializer(Device* device) {
    std::cout << "[RecordCommandSerializer] Initialized." << std::endl;

    // temp
    // Assign id to device (if we only capture cmds on one device)
    ObjectId deviceId;
    mObjectIdManager.GetId(device, &deviceId);

    ObjectId queueId;
    mObjectIdManager.GetId(static_cast<CapturedObject*>(ToBackend(device->GetQueue())), &queueId);
}

RecordCommandSerializer::~RecordCommandSerializer() {
    std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
}

// void RecordCommandSerializer::StartCapture(std::string filename) {
void RecordCommandSerializer::StartCapture() {
    // mOstream = std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::binary);
}

void RecordCommandSerializer::End(std::string filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cout << "[RecordCommandSerializer] ofstream \"" << filename << "\" is not open."
                  << std::endl;
        return;
    }
    mBlock.SerializeToOstream(&out);
    out.close();
    // mOstream = nullptr;
}

void RecordCommandSerializer::EncodeDeviceCreateBuffer(
    // std::ostream* output,
    // cmd::pb::Block* block,
    Device* device,
    Buffer* buffer,
    // WGPUDevice device,
    // WGPUBuffer buffer,
    const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor) {
    // cmd::pb::Cmd cmd;

    cmd::pb::Cmd* cmd = mBlock.add_cmds();
    cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd->mutable_devicecreatebuffer();

    // createBufferCmd->set_deviceid(0);

    // ObjectId deviceId;
    // mObjectIdManager.GetId(device, &deviceId);
    ObjectId deviceId = device->id;
    createBufferCmd->set_deviceid(deviceId);

    cmd::pb::BufferDescriptor* pbDesc = createBufferCmd->mutable_bufferdescriptor();
    if (descriptor->label.data != nullptr) {
        pbDesc->set_label(descriptor->label.data, descriptor->label.length);
    }
    // pbDesc->set_usage(
    //     static_cast<cmd::pb::BufferUsage>(static_cast<WGPUBufferUsage>(descriptor->usage)));
    pbDesc->set_usage(static_cast<uint64_t>(descriptor->usage));
    pbDesc->set_size(descriptor->size);
    pbDesc->set_mappedatcreation(descriptor->mappedAtCreation);

    // std::string serializedDescriptor;
    // pbDesc.SerializeToString(&serializedDescriptor);
    // createBufferCmd->set_bufferdescriptor(serializedDescriptor);

    // createBufferCmd->set_resultid(0);
    ObjectId bufferId;
    mObjectIdManager.GetId(buffer, &bufferId);
    createBufferCmd->set_resultid(bufferId);

    // // Serialize the whole command to the output stream.
    // // cmd.SerializeToOstream(output);
    // cmd.SerializeToOstream(mOstream.get());
}

void RecordCommandSerializer::EncodeQueueWriteBuffer(
      Queue* queue,
      Buffer* buffer,
      uint64_t bufferOffset, const void* data, size_t size
    ) {
    // cmd::pb::Cmd* cmd = mBlock.add_cmds();
    // cmd::pb::QueueWriteBufferCmd* queueWriteBufferCmd = cmd->mutable_queuewritebuffer();
    cmd::pb::QueueWriteBufferCmd* cmd = mBlock.add_cmds()->mutable_queuewritebuffer();

    cmd->set_queueid(queue->id);
    cmd->set_bufferid(buffer->id);
    cmd->set_bufferoffset(bufferOffset);
    cmd->set_data(data, size);
}

// PlaybackCommandDeserializer::PlaybackCommandDeserializer() {
PlaybackCommandDeserializer::PlaybackCommandDeserializer(Device* device) {
    std::cout << "[PlaybackCommandDeserializer] Initialized." << std::endl;
    // mOstream = &std::cout;

    // temp: actually the same device being used as the recorder?
    mObjectIdManager.SetId(device, device->id);
    Queue* queue = ToBackend(device->GetQueue());
    mObjectIdManager.SetId(queue, queue->id);

    // ObjectId deviceId;
    // mObjectIdManager.GetId(device, &deviceId);
}
PlaybackCommandDeserializer::~PlaybackCommandDeserializer() {
    std::cout << "[PlaybackCommandDeserializer] Destroyed." << std::endl;
}

void PlaybackCommandDeserializer::StartPlayback(std::string filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cout << "[PlaybackCommandDeserializer] ifstream \"" << filename << "\" is not open."
                  << std::endl;
        return;
    }

    // // temp
    // while(!in.is_open()) {
    //     std::cout << "[PlaybackCommandDeserializer] wait for \""<< filename << "\" is_open." <<
    //     std::endl; std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // }

    mBlock.ParseFromIstream(&in);
    in.close();

    for (int i = 0; i < mBlock.cmds_size(); i++) {
        DecodeCmd(mBlock.cmds(i));
    }
}

void PlaybackCommandDeserializer::End() {
    // ?
}

void PlaybackCommandDeserializer::DecodeDeviceCreateBuffer(
    const cmd::pb::DeviceCreateBufferCmd& cmd) {
    ObjectId deviceId = cmd.deviceid();
    ObjectId bufferId = cmd.resultid();

    const cmd::pb::BufferDescriptor& pbDesc = cmd.bufferdescriptor();
    // native buffer desciprotr?
    BufferDescriptor descriptor = {};
    descriptor.label = static_cast<std::string_view>(pbDesc.label());
    descriptor.size = pbDesc.size();
    descriptor.usage = static_cast<wgpu::BufferUsage>(pbDesc.usage());
    descriptor.mappedAtCreation = pbDesc.mappedatcreation();

    CapturedObject* capturedDevice = nullptr;
    mObjectIdManager.GetFromId(deviceId, &capturedDevice);
    Device* device = static_cast<Device*>(capturedDevice);

    Buffer* buffer = ToBackend(device->APICreateBuffer(&descriptor));

    mObjectIdManager.SetId(buffer, bufferId);
}

void PlaybackCommandDeserializer::DecodeQueueWriteBuffer(
    const cmd::pb::QueueWriteBufferCmd& cmd) {
    ObjectId queueId = cmd.queueid();
    ObjectId bufferId = cmd.bufferid();


    CapturedObject* capturedQueue = nullptr;
    mObjectIdManager.GetFromId(queueId, &capturedQueue);
    Queue* queue = static_cast<Queue*>(capturedQueue);
    CapturedObject* capturedBuffer = nullptr;
    mObjectIdManager.GetFromId(bufferId, &capturedBuffer);
    Buffer* buffer = static_cast<Buffer*>(capturedBuffer);

    queue->APIWriteBuffer(buffer, cmd.bufferoffset(), cmd.data().c_str(), cmd.data().size());
}

void PlaybackCommandDeserializer::DecodeCmd(const cmd::pb::Cmd& cmd) {
    switch (cmd.type_case()) {
        case cmd::pb::Cmd::TypeCase::kDeviceCreateBuffer: {
            DecodeDeviceCreateBuffer(cmd.devicecreatebuffer());
            break;
        }
        case cmd::pb::Cmd::TypeCase::kQueueWriteBuffer: {
            DecodeQueueWriteBuffer(cmd.queuewritebuffer());
            break;
        }
        default:
            break;
    }
}

}  // namespace dawn::native::webgpu
