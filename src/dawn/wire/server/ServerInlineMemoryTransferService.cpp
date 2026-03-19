// Copyright 2019 The Dawn & Tint Authors
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

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/439062058): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include <cstring>
#include <memory>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/Platform.h"
#include "dawn/wire/WireServer.h"
#include "dawn/wire/server/Server.h"

#if DAWN_PLATFORM_IS(WINDOWS)
#include <Windows.h>

#include <string>
#endif

namespace dawn::wire::server {

#if DAWN_PLATFORM_IS(WINDOWS)
namespace {

struct SharedMemoryInfo {
    raw_ptr<void> handle = nullptr;
    raw_ptr<uint8_t, AllowPtrArithmetic> mappedPointer = nullptr;
    bool IsEmpty() const { return handle == nullptr && mappedPointer == nullptr; }
};

SharedMemoryInfo DeserializeSharedMemoryInfo(const void* deserializePointer,
                                             size_t deserializeSize) {
    DAWN_ASSERT(deserializeSize >= sizeof(size_t));

    size_t size = 0;
    memcpy(&size, deserializePointer, sizeof(size_t));

    size_t sharedMemoryNameSize = deserializeSize - sizeof(size_t);
    std::vector<char> name(sharedMemoryNameSize + 1);
    memcpy(name.data(), static_cast<const uint8_t*>(deserializePointer) + sizeof(size_t),
           sharedMemoryNameSize);

    HANDLE fileMappingHandle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.data());
    if (fileMappingHandle == nullptr) {
        return {};
    }

    void* mappedPointer = MapViewOfFile(fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (mappedPointer == nullptr) {
        CloseHandle(fileMappingHandle);
        return {};
    }

    SharedMemoryInfo sharedMemoryInfo;
    sharedMemoryInfo.handle = fileMappingHandle;
    sharedMemoryInfo.mappedPointer = static_cast<uint8_t*>(mappedPointer);
    return sharedMemoryInfo;
}

void DeallocateSharedMemory(const SharedMemoryInfo& sharedMemoryInfo) {
    if (sharedMemoryInfo.mappedPointer != nullptr) {
        UnmapViewOfFile(sharedMemoryInfo.mappedPointer);
    }
    if (sharedMemoryInfo.handle != nullptr) {
        CloseHandle(sharedMemoryInfo.handle);
    }
}

}  // anonymous namespace
#endif

class InlineMemoryTransferService : public MemoryTransferService {
  public:
    class ReadHandleImpl : public ReadHandle {
      public:
        ReadHandleImpl() {}
        ~ReadHandleImpl() override = default;

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override { return size; }

        void SerializeDataUpdate(const void* data,
                                 size_t offset,
                                 size_t size,
                                 void* serializePointer) override {
            if (size > 0) {
                DAWN_ASSERT(data != nullptr);
                DAWN_ASSERT(serializePointer != nullptr);
                memcpy(serializePointer, data, size);
            }
        }
    };

    class WriteHandleImpl : public WriteHandle {
      public:
        WriteHandleImpl() {}
        ~WriteHandleImpl() override = default;

        bool DeserializeDataUpdate(const void* deserializePointer,
                                   size_t deserializeSize,
                                   size_t offset,
                                   size_t size) override {
            auto target = GetTarget();
            if (deserializeSize != size || target.data() == nullptr ||
                deserializePointer == nullptr) {
                return false;
            }
            if (offset > target.size() || size > target.size() - offset) {
                return false;
            }
            memcpy(target.data() + offset, deserializePointer, size);
            return true;
        }
    };

#if DAWN_PLATFORM_IS(WINDOWS)
    class ReadHandleWithSharedMemoryFileHandleImpl : public ReadHandle {
      public:
        // sharedMemoryInfo may be empty as the wire client may have already been destroyed when
        // the wire server is created.
        explicit ReadHandleWithSharedMemoryFileHandleImpl(const SharedMemoryInfo& sharedMemoryInfo)
            : mSharedMemoryInfo(sharedMemoryInfo) {}
        ~ReadHandleWithSharedMemoryFileHandleImpl() override {
            DeallocateSharedMemory(mSharedMemoryInfo);
        }

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override { return 0; }

        void SerializeDataUpdate(const void* data,
                                 size_t offset,
                                 size_t size,
                                 void* serializePointer) override {
            if (size > 0 && mSharedMemoryInfo.mappedPointer != nullptr) {
                DAWN_ASSERT(data != nullptr);
                DAWN_ASSERT(serializePointer != nullptr);
                memcpy(mSharedMemoryInfo.mappedPointer + offset, data, size);
            }
        }

      private:
        SharedMemoryInfo mSharedMemoryInfo;
    };

    class WriteHandleWithSharedMemoryFileHandleImpl : public WriteHandle {
      public:
        // sharedMemoryInfo may be empty as the wire client may have already been destroyed when
        // the wire server is created.
        explicit WriteHandleWithSharedMemoryFileHandleImpl(const SharedMemoryInfo& sharedMemoryInfo)
            : mSharedMemoryInfo(sharedMemoryInfo) {}
        ~WriteHandleWithSharedMemoryFileHandleImpl() override {
            DeallocateSharedMemory(mSharedMemoryInfo);
        }

        bool DeserializeDataUpdate(const void* deserializePointer,
                                   size_t deserializeSize,
                                   size_t offset,
                                   size_t size) override {
            auto target = GetTarget();
            if (target.data() == nullptr) {
                return false;
            }
            if (offset > target.size() || size > target.size() - offset) {
                return false;
            }

            if (mSharedMemoryInfo.mappedPointer != nullptr) {
                memcpy(target.data() + offset, mSharedMemoryInfo.mappedPointer + offset, size);
            }

            return true;
        }

      private:
        SharedMemoryInfo mSharedMemoryInfo;
    };

#endif

    InlineMemoryTransferService() {}
    ~InlineMemoryTransferService() override = default;

    bool DeserializeReadHandle(const void* deserializePointer,
                               size_t deserializeSize,
                               ReadHandle** readHandle) override {
        DAWN_ASSERT(readHandle != nullptr);
        *readHandle = new ReadHandleImpl();
        return true;
    }

    bool DeserializeWriteHandle(const void* deserializePointer,
                                size_t deserializeSize,
                                WriteHandle** writeHandle) override {
        DAWN_ASSERT(writeHandle != nullptr);
        *writeHandle = new WriteHandleImpl();
        return true;
    }

#if DAWN_PLATFORM_IS(WINDOWS)
    bool DeserializeReadHandleWithSharedMemory(const void* deserializePointer,
                                               size_t deserializeSize,
                                               ReadHandle** readHandle) override {
        DAWN_ASSERT(readHandle != nullptr);

        SharedMemoryInfo sharedMemoryInfo =
            DeserializeSharedMemoryInfo(deserializePointer, deserializeSize);
        *readHandle = new ReadHandleWithSharedMemoryFileHandleImpl(sharedMemoryInfo);
        return true;
    }

    bool DeserializeWriteHandleWithSharedMemory(const void* deserializePointer,
                                                size_t deserializeSize,
                                                WriteHandle** writeHandle) override {
        DAWN_ASSERT(writeHandle != nullptr);

        SharedMemoryInfo sharedMemoryInfo =
            DeserializeSharedMemoryInfo(deserializePointer, deserializeSize);
        *writeHandle = new WriteHandleWithSharedMemoryFileHandleImpl(sharedMemoryInfo);
        return true;
    }
#endif
};

std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService() {
    return std::make_unique<InlineMemoryTransferService>();
}

}  // namespace dawn::wire::server
