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

#ifndef SRC_DAWN_NATIVE_WEBGPU_RECORDER_H_
#define SRC_DAWN_NATIVE_WEBGPU_RECORDER_H_

#include <iostream>

#include "absl/container/flat_hash_map.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/webgpu/ObjectWGPU.h"

// TODO(crbug.com/413053623): add compiler macro in src/dawn/utils
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#endif

#include "dawn/devtool/protos/cmd.pb.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace dawn::native {

struct BufferDescriptor;

namespace webgpu {

using ObjectId = uint64_t;

class Device;
class Buffer;
class Queue;

class Recorder {
  public:
    // TODO(crbug.com/413053623): Use codegen for these command encoding functions.
    void EncodeDeviceCreateBuffer(Device* device,
                                  Buffer* buffer,
                                  const native::UnpackedPtr<BufferDescriptor>& descriptor);

    void EncodeQueueWriteBuffer(Queue* queue,
                                Buffer* buffer,
                                uint64_t bufferOffset,
                                const void* data,
                                size_t size);

    void StartCapture(Device* device);
    void End(std::ostream& out);

  private:
    // Get a new ObjectId and assign to the CapturedObject.
    // Called when encoding commands.
    bool GetId(CapturedObject* object, ObjectId* out);

    void SnapshotGlobalState(Device* device, devtool::cmd::pb::SnapshotGlobalState* globalState);

    // A simple monotically incrementing id
    ObjectId mCurrentId = 0;
    absl::flat_hash_map<ObjectId, CapturedObject*> mObjectIdMaps;

    devtool::cmd::pb::Block mBlock;
};

}  // namespace webgpu

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_WEBGPU_RECORDER_H_
