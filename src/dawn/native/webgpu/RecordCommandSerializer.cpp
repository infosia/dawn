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

#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"

// // Temp suppress protobuf compile error
// #if defined(__clang__)
// _Pragma("clang diagnostic ignored \"-Wextra-semi-stmt\"")
//     _Pragma("clang diagnostic ignored \"-Winconsistent-missing-destructor-override\"")
//         _Pragma("clang diagnostic ignored \"-Wsuggest-destructor-override\"")
// #endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#endif

#include "dawn/native/webgpu/protos/cmd.pb.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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
bool ObjectIdManager::GetFromId(ObjectId id, CapturedObject** out) {
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

// RecordCommandSerializer::RecordCommandSerializer(WGPUDevice device) {
RecordCommandSerializer::RecordCommandSerializer() {
    std::cout << "[RecordCommandSerializer] Initialized." << std::endl;
    // mOstream = &std::cout;
}
RecordCommandSerializer::~RecordCommandSerializer() {
    std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
}

void RecordCommandSerializer::StartCapture(std::string filename) {
    mOstream = std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::binary);
}

void RecordCommandSerializer::End() {
    mOstream->close();
    mOstream = nullptr;
}

void RecordCommandSerializer::EncodeDeviceCreateBuffer(
    // std::ostream* output,
    Device* device,
    Buffer* buffer,
    // WGPUDevice device,
    // WGPUBuffer buffer,
    const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor) {
    cmd::pb::Cmd cmd;

    cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd.mutable_devicecreatebuffer();

    // createBufferCmd->set_deviceid(0);

    // ObjectId deviceId;
    // mObjectIdManager.GetId(device, &deviceId);
    ObjectId deviceId = device->id;
    createBufferCmd->set_deviceid(deviceId);

    cmd::pb::BufferDescriptor* pbDesc = createBufferCmd->mutable_bufferdescriptor();
    if (descriptor->label.data != nullptr) {
        pbDesc->set_label(descriptor->label.data, descriptor->label.length);
    }
    pbDesc->set_usage(
        static_cast<cmd::pb::BufferUsage>(static_cast<WGPUBufferUsage>(descriptor->usage)));
    pbDesc->set_size(descriptor->size);
    pbDesc->set_mappedatcreation(descriptor->mappedAtCreation);

    // std::string serializedDescriptor;
    // pbDesc.SerializeToString(&serializedDescriptor);
    // createBufferCmd->set_bufferdescriptor(serializedDescriptor);

    // createBufferCmd->set_resultid(0);
    ObjectId bufferId;
    mObjectIdManager.GetId(buffer, &bufferId);
    createBufferCmd->set_resultid(bufferId);

    // Serialize the whole command to the output stream.
    // cmd.SerializeToOstream(output);
    cmd.SerializeToOstream(mOstream.get());
}






ReplayCommandDeserializer::ReplayCommandDeserializer() {
    std::cout << "[ReplayCommandDeserializer] Initialized." << std::endl;
    // mOstream = &std::cout;
}
ReplayCommandDeserializer::~ReplayCommandDeserializer() {
    std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
}

void ReplayCommandDeserializer::DecodeDeviceCreateBuffer(const cmd::pb::DeviceCreateBufferCmd& cmd) {
    ObjectId deviceId = cmd.deviceid();
    ObjectId bufferId = cmd.resultid();

    const cmd::pb::BufferDescriptor& pbDesc = cmd.bufferdescriptor();
    BufferDescriptor descriptor;
    // descriptor.label
    descriptor.size = pbDesc.size();
    // descriptor.usage = pbDesc.usage();
    descriptor.mappedAtCreation = pbDesc.mappedatcreation();
    
    Device* device = nullptr;
    mObjectIdManager.GetFromId(deviceId, reinterpret_cast<CapturedObject**>(&device));

    Buffer* buffer = ToBackend(device->APICreateBuffer(&descriptor));

    mObjectIdManager.SetId(buffer, bufferId);
}


void ReplayCommandDeserializer::DecodeCmd(const cmd::pb::Cmd& cmd) {
    switch(cmd.type_case()) {
        case cmd::pb::Cmd::TypeCase::kDeviceCreateBuffer: {
            DecodeDeviceCreateBuffer(cmd.devicecreatebuffer());
            break;
        }
        default:
        break;
    }
}



}  // namespace dawn::native::webgpu
