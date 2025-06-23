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

#ifndef SRC_DAWN_NATIVE_WEBGPU_RECORDCOMMANDSERIALIZER_H_
#define SRC_DAWN_NATIVE_WEBGPU_RECORDCOMMANDSERIALIZER_H_

// #include "dawn/wire/Wire.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "dawn/native/ChainUtils.h"
// #include "dawn/native/webgpu/DeviceWGPU.h"
// #include "dawn/native/webgpu/BufferWGPU.h"
// #include "partition_alloc/pointers/raw_ptr.h"

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

class Device;
class Buffer;
class Queue;

using ObjectId = uint32_t;
using ObjectHandle = ObjectId;

// namespace cmd::pb {
// class Cmd;
// class DeviceCreateBufferCmd;
// }  // namespace cmd::pb

class CapturedObject {
  public:
    ObjectId id;
};

// TODO: simply use template for functions
class ObjectIdManager {
    // public:
    //   // Get Id
    //   bool GetId(WGPUDevice object, ObjectId* out);
    //   bool GetId(WGPUBuffer object, ObjectId* out);

    //   // Get Object
    //   bool GetFromId(ObjectId id, WGPUDevice* out);
    //   bool GetFromId(ObjectId id, WGPUBuffer* out);

    // private:
    //   absl::flat_hash_map<ObjectId, void*> mObjectIdMaps;
  public:
    // Get Id
    bool GetId(CapturedObject* object, ObjectId* out);

    // Get Object
    // Used by objectIdResolver (deserializer) only?
    bool GetFromId(ObjectId id, CapturedObject** out) const;
    bool SetId(CapturedObject* object, ObjectId id);

    // CapturedObject* GetResource(ObjectId id) const;

  private:
    absl::flat_hash_map<ObjectId, CapturedObject*> mObjectIdMaps;

    // Simply increment
    // ObjectId mCurrentId = 0;
    ObjectId mCurrentId = 1;  // temp, 0 for device
};

// class ObjectIdProvider {
//   public:
//     // ObjectId GetId(const Device* object);
//     // ObjectId GetId(const Buffer* object);
//     // bool GetId(const Device* object, ObjectId* out);
//     // bool GetId(const Buffer* object, ObjectId* out);
//     bool GetId(WGPUDevice object, ObjectId* out);
//     bool GetId(WGPUBuffer object, ObjectId* out);
// }

// class ObjectIdResolver {
//   public:
//     // bool GetFromId(ObjectId id, Device* out);
//     // bool GetFromId(ObjectId id, Buffer* out);
//     bool GetFromId(ObjectId id, WGPUDevice* out);
//     bool GetFromId(ObjectId id, WGPUBuffer* out);
// }

// class RecordCommandSerializer : public dawn::wire::CommandSerializer {
class RecordCommandSerializer {
  public:
    // RecordCommandSerializer(WGPUDevice device);
    // RecordCommandSerializer();
    RecordCommandSerializer(Device* device);
    ~RecordCommandSerializer();

    void EncodeDeviceCreateBuffer(
        // std::ostream* output,
        // cmd::pb::Block* block,
        Device* device,
        Buffer* buffer,
        // WGPUDevice device,
        // WGPUBuffer buffer,
        const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor);

    void EncodeQueueWriteBuffer(Queue* queue,
                                Buffer* buffer,
                                uint64_t bufferOffset,
                                const void* data,
                                size_t size);

    // void* GetCmdSpace(size_t size) override;

    // bool Flush() override;

    // size_t GetMaximumAllocationSize() const override;

    // void OnSerializeError() override;

    void StartCapture();
    void End(std::string filename);
    void End(std::ostream& out);

    ObjectIdManager mObjectIdManager;

  private:
    // std::vector<uint8_t> mBuffer;
    cmd::pb::Block mBlock;
};

class PlaybackCommandDeserializer {
  public:
    // PlaybackCommandDeserializer();
    PlaybackCommandDeserializer(Device* device);
    ~PlaybackCommandDeserializer();

    // Decode cmds can be a seperate class and only sits in cpp file
    void DecodeDeviceCreateBuffer(const cmd::pb::DeviceCreateBufferCmd& cmd);
    void DecodeQueueWriteBuffer(const cmd::pb::QueueWriteBufferCmd& cmd);
    void DecodeCmd(const cmd::pb::Cmd& cmd);

    void StartPlayback(std::string filename);
    void StartPlayback(std::istream& in);
    void End();

    // API design
    // Load()
    // play(position (cmds[index])
    // getResource<T>(id) (return CapturedObject (*WGPUBackend object inherits))

    ObjectIdManager mObjectIdManager;

  private:
    cmd::pb::Block mBlock;
};

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_RECORDCOMMANDSERIALIZER_H_
