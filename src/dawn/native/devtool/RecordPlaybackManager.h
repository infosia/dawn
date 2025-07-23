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

#ifndef SRC_DAWN_NATIVE_DEVTOOL_RECORDPLAYBACKMANAGER_H_
#define SRC_DAWN_NATIVE_DEVTOOL_RECORDPLAYBACKMANAGER_H_

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/webgpu/ObjectWGPU.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#endif

#include "dawn/native/devtool/protos/cmd.pb.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace dawn::native::webgpu {

class Device;
class Buffer;
class Queue;

}  // namespace dawn::native::webgpu

namespace dawn::native::devtool {

using ObjectId = dawn::native::webgpu::ObjectId;
using CapturedObject = dawn::native::webgpu::CapturedObject;

class ObjectIdManager {
  public:
    // Get a new ObjectId and assign to the CapturedObject.
    // Called during recording.
    // Called during playback for assigning to objects exisit outside of the record range (e.g.
    // Device, default Queue).
    bool GetId(CapturedObject* object, ObjectId* out);

    // Get a pointer to the CapturedObject given an ObjectId.
    // Called during playback.
    bool GetFromId(ObjectId id, CapturedObject** out) const;

    // Add an ObjectId and CapturedObject pair to the indexing.
    // Called during playback for:
    //   - Command that creates a new object.
    //   - Objects exist outside of the record range (e.g. Device, default Queue).
    bool SetId(CapturedObject* object, ObjectId id);

  private:
    absl::flat_hash_map<ObjectId, CapturedObject*> mObjectIdMaps;

    // A simple monotically incrementing id
    ObjectId mCurrentId = 0;
};

class RecordCommandSerializer {
  public:
    explicit RecordCommandSerializer(dawn::native::webgpu::Device* device);
    ~RecordCommandSerializer();

    void EncodeDeviceCreateBuffer(dawn::native::webgpu::Device* device,
                                  dawn::native::webgpu::Buffer* buffer,
                                  const UnpackedPtr<dawn::native::BufferDescriptor>& descriptor);

    void EncodeQueueWriteBuffer(dawn::native::webgpu::Queue* queue,
                                dawn::native::webgpu::Buffer* buffer,
                                uint64_t bufferOffset,
                                const void* data,
                                size_t size);

    void StartCapture();
    void End(std::ostream& out);

    ObjectIdManager mObjectIdManager;

  private:
    cmd::pb::Block mBlock;
};

class PlaybackCommandDeserializer {
  public:
    explicit PlaybackCommandDeserializer(dawn::native::webgpu::Device* device);
    ~PlaybackCommandDeserializer();

    void StartPlayback(std::istream& in);
    void End();

    // TODO(crbug.com/413053623): More playback control API

    ObjectIdManager mObjectIdManager;

  private:
    cmd::pb::Block mBlock;
};

}  // namespace dawn::native::devtool

#endif  // SRC_DAWN_NATIVE_DEVTOOL_RECORDPLAYBACKMANAGER_H_
