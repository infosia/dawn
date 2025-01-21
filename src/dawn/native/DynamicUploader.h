// Copyright 2018 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_DYNAMICUPLOADER_H_
#define SRC_DAWN_NATIVE_DYNAMICUPLOADER_H_

#include <memory>
#include <vector>

#include "dawn/common/Ref.h"
#include "dawn/native/Error.h"
#include "dawn/native/Forward.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/RingBufferAllocator.h"
#include "partition_alloc/pointers/raw_ptr.h"

// DynamicUploader is the front-end implementation used to manage multiple ring buffers for upload
// usage.
namespace dawn::native {

class BufferBase;
class DynamicUploader;

class UploadHandle {
  public:
    UploadHandle(DynamicUploader* dynamicUploader,
                 uint64_t startOffset,
                 Ref<BufferBase> stagingBuffer,
                 bool isSubAllocation);
    ~UploadHandle();

    uint64_t GetStartOffset() const;
    BufferBase* GetStagingBuffer() const;
    uint8_t* GetMappedPointer() const;

  private:
    raw_ptr<DynamicUploader> mDynamicUploader = nullptr;
    uint64_t mStartOffset = 0;
    Ref<BufferBase> mStagingBuffer;
    bool mIsSubAllocation = true;
};

class DynamicUploader {
  public:
    explicit DynamicUploader(DeviceBase* device);
    ~DynamicUploader() = default;

    ResultOrError<std::unique_ptr<UploadHandle>> Allocate(uint64_t allocationSize,
                                                          ExecutionSerial serial,
                                                          uint64_t offsetAlignment);
    ResultOrError<std::unique_ptr<UploadHandle>> AllocateDynamicBuffer(uint64_t allocationSize,
                                                                       const char* label,
                                                                       bool ignoreLazyClear = true);
    void Deallocate(ExecutionSerial lastCompletedSerial, bool freeAll = false);
    void TrackStagingMemorySizeToRelease(uint64_t stagingMemorySize);

  private:
    static constexpr uint64_t kRingBufferSize = 4 * 1024 * 1024;

    struct RingBuffer {
        Ref<BufferBase> mStagingBuffer;
        RingBufferAllocator mAllocator;
    };

    ResultOrError<std::unique_ptr<UploadHandle>> AllocateInternal(uint64_t allocationSize,
                                                                  ExecutionSerial serial,
                                                                  uint64_t offsetAlignment);
    void FlushIfNecessary();

    std::vector<std::unique_ptr<RingBuffer>> mRingBuffers;
    raw_ptr<DeviceBase> mDevice;
    SerialQueue<ExecutionSerial, uint64_t> mReleasedStagingMemorySizes;
};
}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_DYNAMICUPLOADER_H_
