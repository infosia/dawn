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
#include <utility>

#include "dawn/common/Alloc.h"
#include "dawn/common/Assert.h"
#include "dawn/common/Platform.h"
#include "dawn/wire/WireClient.h"
#include "dawn/wire/client/Client.h"

#if DAWN_PLATFORM_IS(WINDOWS)
#include <Windows.h>

#include <atomic>
#include <string>

#include "dawn/common/Constants.h"
#endif

namespace dawn::wire::client {

#if DAWN_PLATFORM_IS(WINDOWS)
namespace {

struct SharedTransferBufferInfo {
    std::string name = {};
    raw_ptr<void> handle = nullptr;
    size_t size = 0;
    raw_ptr<uint8_t, AllowPtrArithmetic> mappedPointer = nullptr;
    bool IsEmpty() const { return handle == nullptr && mappedPointer == nullptr; }
};

// Create a unique name for named shared memory file mapping handle
std::string MakeSharedName() {
    static std::atomic<uint64_t> counter{1};
    return "Local\\DawnWire_" + std::to_string(counter.fetch_add(1));
}

SharedTransferBufferInfo AllocateSharedTransferBuffer(size_t size) {
    std::string name = MakeSharedName();
    size_t allocatedSize = Align(size, kD3D12SharedBufferMemoryFileMappingHandleSizeAlignment);
    // Always allocate a shared memory file mapping handle
    if (allocatedSize == 0) {
        allocatedSize = kD3D12SharedBufferMemoryFileMappingHandleSizeAlignment;
    }
    HANDLE fileMappingHandle = CreateFileMappingA(
        INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
        static_cast<DWORD>(static_cast<uint64_t>(allocatedSize) >> 32),
        static_cast<DWORD>(static_cast<uint64_t>(allocatedSize) & 0xFFFFFFFF), name.c_str());
    if (fileMappingHandle == nullptr) {
        return {};
    }

    void* mappedPointer = MapViewOfFile(fileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (mappedPointer == nullptr) {
        CloseHandle(fileMappingHandle);
        return {};
    }

    SharedTransferBufferInfo sharedMemoryInfo;
    sharedMemoryInfo.name = name;
    sharedMemoryInfo.size = size;
    sharedMemoryInfo.handle = fileMappingHandle;
    sharedMemoryInfo.mappedPointer = static_cast<uint8_t*>(mappedPointer);
    return sharedMemoryInfo;
}

void DeallocateSharedMemory(const SharedTransferBufferInfo& sharedMemoryInfo) {
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
    class ReadHandleImpl : public ReadHandle {
      public:
        explicit ReadHandleImpl(std::unique_ptr<uint8_t[]> stagingData, size_t size)
            : mStagingData(std::move(stagingData)), mSize(size) {}

        ~ReadHandleImpl() override = default;

        size_t SerializeCreateSize() override { return 0; }

        void SerializeCreate(void*) override {}

        const void* GetData() override { return mStagingData.get(); }

        bool DeserializeDataUpdate(const void* deserializePointer,
                                   size_t deserializeSize,
                                   size_t offset,
                                   size_t size) override {
            if (deserializeSize != size || deserializePointer == nullptr) {
                return false;
            }

            if (offset > mSize || size > mSize - offset) {
                return false;
            }

            void* start = static_cast<uint8_t*>(mStagingData.get()) + offset;
            memcpy(start, deserializePointer, size);
            return true;
        }

      private:
        std::unique_ptr<uint8_t[]> mStagingData;
        size_t mSize;
    };

    class WriteHandleImpl : public WriteHandle {
      public:
        explicit WriteHandleImpl(std::unique_ptr<uint8_t[]> stagingData, size_t size)
            : mStagingData(std::move(stagingData)), mSize(size) {}

        ~WriteHandleImpl() override = default;

        size_t SerializeCreateSize() override { return 0; }

        void SerializeCreate(void*) override {}

        void* GetData() override { return mStagingData.get(); }

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override {
            DAWN_ASSERT(offset <= mSize);
            DAWN_ASSERT(size <= mSize - offset);
            return size;
        }

        void SerializeDataUpdate(void* serializePointer, size_t offset, size_t size) override {
            DAWN_ASSERT(mStagingData != nullptr);
            DAWN_ASSERT(serializePointer != nullptr);
            DAWN_ASSERT(offset <= mSize);
            DAWN_ASSERT(size <= mSize - offset);
            memcpy(serializePointer, static_cast<uint8_t*>(mStagingData.get()) + offset, size);
        }

      private:
        std::unique_ptr<uint8_t[]> mStagingData;
        size_t mSize;
    };

#if DAWN_PLATFORM_IS(WINDOWS)
    class ReadHandleWithSharedMemoryFileHandleImpl : public ReadHandle {
      public:
        explicit ReadHandleWithSharedMemoryFileHandleImpl(
            const SharedTransferBufferInfo& sharedMemoryInfo)
            : mSharedMemoryInfo(sharedMemoryInfo) {
            DAWN_ASSERT(!sharedMemoryInfo.IsEmpty());
        }

        ~ReadHandleWithSharedMemoryFileHandleImpl() override {
            DeallocateSharedMemory(mSharedMemoryInfo);
        }

        // Send the name and the size of the file mapping handle to wire server
        size_t SerializeCreateSize() override {
            return mSharedMemoryInfo.name.size() + sizeof(size_t);
        }

        void SerializeCreate(void* ptr) override {
            memcpy(ptr, &mSharedMemoryInfo.size, sizeof(size_t));
            memcpy(static_cast<uint8_t*>(ptr) + sizeof(size_t), mSharedMemoryInfo.name.c_str(),
                   mSharedMemoryInfo.name.size());
        }

        const void* GetData() override { return mSharedMemoryInfo.mappedPointer; }

        // We don't need to deserialize anything as the memory has already been in the shared
        // memory.
        bool DeserializeDataUpdate(const void* deserializePointer,
                                   size_t deserializeSize,
                                   size_t offset,
                                   size_t size) override {
            return true;
        }

      private:
        SharedTransferBufferInfo mSharedMemoryInfo;
    };

    class WriteHandleWithSharedMemoryFileHandleImpl : public WriteHandle {
      public:
        explicit WriteHandleWithSharedMemoryFileHandleImpl(
            const SharedTransferBufferInfo& sharedMemoryInfo)
            : mSharedMemoryInfo(sharedMemoryInfo) {
            DAWN_ASSERT(!mSharedMemoryInfo.IsEmpty());
        }

        ~WriteHandleWithSharedMemoryFileHandleImpl() override = default;

        // Send the name and the size of the file mapping handle to wire server
        size_t SerializeCreateSize() override {
            return mSharedMemoryInfo.name.size() + sizeof(mSharedMemoryInfo.size);
        }

        void SerializeCreate(void* ptr) override {
            memcpy(ptr, &mSharedMemoryInfo.size, sizeof(mSharedMemoryInfo.size));
            memcpy(static_cast<uint8_t*>(ptr) + sizeof(mSharedMemoryInfo.size),
                   mSharedMemoryInfo.name.c_str(), mSharedMemoryInfo.name.size());
        }

        void* GetData() override { return mSharedMemoryInfo.mappedPointer; }

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override { return 0; }

        // We don't need to serialize anything as the memory has already been in the shared memory.
        void SerializeDataUpdate(void* serializePointer, size_t offset, size_t size) override {}

      private:
        SharedTransferBufferInfo mSharedMemoryInfo;
    };
#endif

  public:
    InlineMemoryTransferService() {}
    ~InlineMemoryTransferService() override = default;

    ReadHandle* CreateReadHandle(size_t size) override {
        auto stagingData = std::unique_ptr<uint8_t[]>(AllocNoThrow<uint8_t>(size));
        if (stagingData) {
            return new ReadHandleImpl(std::move(stagingData), size);
        }
        return nullptr;
    }

    WriteHandle* CreateWriteHandle(size_t size) override {
        auto stagingData = std::unique_ptr<uint8_t[]>(AllocNoThrow<uint8_t>(size));
        if (stagingData) {
            memset(stagingData.get(), 0, size);
            return new WriteHandleImpl(std::move(stagingData), size);
        }
        return nullptr;
    }

#if DAWN_PLATFORM_IS(WINDOWS)
    ReadHandle* CreateReadHandleWithSharedTransferBuffer(size_t size) override {
        SharedTransferBufferInfo sharedTransferBufferInfo = AllocateSharedTransferBuffer(size);
        if (sharedTransferBufferInfo.IsEmpty()) {
            return nullptr;
        }
        return new ReadHandleWithSharedMemoryFileHandleImpl(sharedTransferBufferInfo);
    }

    WriteHandle* CreateWriteHandleWithSharedTransferBuffer(size_t size) override {
        SharedTransferBufferInfo sharedMemoryInfo = AllocateSharedTransferBuffer(size);
        if (sharedMemoryInfo.IsEmpty()) {
            return nullptr;
        }
        return new WriteHandleWithSharedMemoryFileHandleImpl(sharedMemoryInfo);
    }
#endif
};

std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService() {
    return std::make_unique<InlineMemoryTransferService>();
}

}  // namespace dawn::wire::client
