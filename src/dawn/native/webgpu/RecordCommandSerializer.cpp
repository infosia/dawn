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

#include <string>

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

bool ObjectIdManager::GetId(WGPUDevice object, ObjectId* out) {
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    *out = id;
    return true;
}
bool ObjectIdManager::GetId(WGPUBuffer object, ObjectId* out) {
    ObjectId id = mCurrentId++;
    mObjectIdMaps.emplace(id, object);
    *out = id;
    return true;
}

bool ObjectIdManager::GetFromId(ObjectId id, WGPUDevice* out) {
    auto result = mObjectIdMaps.find(id);
    if (result == mObjectIdMaps.end()) {
        return false;
    }
    *out = reinterpret_cast<WGPUDevice>(result->second);
    return true;
}
bool ObjectIdManager::GetFromId(ObjectId id, WGPUBuffer* out) {
    auto result = mObjectIdMaps.find(id);
    if (result == mObjectIdMaps.end()) {
        return false;
    }
    *out = reinterpret_cast<WGPUBuffer>(result->second);
    return true;
}

RecordCommandSerializer::RecordCommandSerializer(WGPUDevice device) {
    std::cout << "[RecordCommandSerializer] Initialized." << std::endl;
    mOstream = &std::cout;
}
RecordCommandSerializer::~RecordCommandSerializer() {
    std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
}

void RecordCommandSerializer::EncodeDeviceCreateBuffer(
    // std::ostream* output,
    WGPUDevice device,
    WGPUBuffer buffer,
    const UnpackedPtr<BufferDescriptor>& descriptor) {
    cmd::pb::Cmd cmd;

    cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd.mutable_devicecreatebuffer();

    // createBufferCmd->set_deviceid(0);
    ObjectId deviceId;
    mObjectIdManager.GetId(device, &deviceId);
    createBufferCmd->set_deviceid(deviceId);

    cmd::pb::BufferDescriptor pbDesc;
    if (descriptor->label.data != nullptr) {
        pbDesc.set_label(descriptor->label.data, descriptor->label.length);
    }
    pbDesc.set_usage(
        static_cast<cmd::pb::BufferUsage>(static_cast<WGPUBufferUsage>(descriptor->usage)));
    pbDesc.set_size(descriptor->size);
    pbDesc.set_mappedatcreation(descriptor->mappedAtCreation);

    std::string serializedDescriptor;
    pbDesc.SerializeToString(&serializedDescriptor);
    createBufferCmd->set_descriptor_(serializedDescriptor);

    // createBufferCmd->set_resultid(0);
    ObjectId bufferId;
    mObjectIdManager.GetId(buffer, &bufferId);
    createBufferCmd->set_resultid(bufferId);

    // Serialize the whole command to the output stream.
    // cmd.SerializeToOstream(output);
    cmd.SerializeToOstream(mOstream);
}

}  // namespace dawn::native::webgpu
