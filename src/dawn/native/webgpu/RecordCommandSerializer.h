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
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "dawn/native/ChainUtils.h"
// #include "dawn/native/webgpu/DeviceWGPU.h"
// #include "dawn/native/webgpu/BufferWGPU.h"
// #include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native::webgpu {

class Device;
class Buffer;

using ObjectId = uint32_t;
using ObjectHandle = ObjectId;

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

    // // Get Object
    // // Used by objectIdResolver (deserializer) only?
    // bool GetFromId(ObjectId id, CapturedObject* out);

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
    RecordCommandSerializer();
    ~RecordCommandSerializer();

    void EncodeDeviceCreateBuffer(
        // std::ostream* output,
        Device* device,
        Buffer* buffer,
        // WGPUDevice device,
        // WGPUBuffer buffer,
        const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor);

    // void* GetCmdSpace(size_t size) override;

    // bool Flush() override;

    // size_t GetMaximumAllocationSize() const override;

    // void OnSerializeError() override;

    void StartCapture(std::string filename);
    void End();

  private:
    std::vector<uint8_t> mBuffer;

    ObjectIdManager mObjectIdManager;

    // raw_ptr<std::ostream> mOstream;
    std::unique_ptr<std::ofstream> mOstream;
};

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_RECORDCOMMANDSERIALIZER_H_
