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

#include "dawn/common/Constants.h"
#endif

namespace dawn::wire::client {

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
    class ImportableTransferBufferImpl : public ImportableTransferBuffer {
      public:
        explicit ImportableTransferBufferImpl(size_t bufferSize) {
            // It is not allowed to create a file mapping handle with 0 size.
            DAWN_ASSERT(bufferSize != 0);

            size_t allocatedSize =
                Align(bufferSize, kD3D12SharedBufferMemoryFileMappingHandleSizeAlignment);

            // Create a file mapping handle backed by the system paging file. In Chromium the GPU
            // transfer buffer is also implemented in this way.
            mHandle = CreateFileMappingA(
                INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                static_cast<DWORD>(static_cast<uint64_t>(allocatedSize) >> 32),
                static_cast<DWORD>(static_cast<uint64_t>(allocatedSize) & 0xFFFFFFFF), nullptr);
            if (mHandle == nullptr) {
                Release();
                return;
            }

            mMappedPointer =
                MapViewOfFile(mHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, bufferSize);
            if (mMappedPointer == nullptr) {
                Release();
            }
        }

        ~ImportableTransferBufferImpl() override { Release(); }

        void Release() override {
            if (mMappedPointer != nullptr) {
                UnmapViewOfFile(mMappedPointer);
                mMappedPointer = nullptr;
            }
            if (mHandle != nullptr) {
                CloseHandle(mHandle);
                mHandle = nullptr;
            }
        }

        size_t SerializeCreateSize() const override {
            // Serialize a file mapping handle and the logical size of the buffer.
            return sizeof(HANDLE);
        }

        void SerializeCreate(void* ptr) const override {
            // Duplicate the handle to make sure the handle can be used in wire server. Note that
            // the underlying memory won't be released until both `mHandle` and `serverHandle` are
            // closed (`serverHandle` should be closed in the wire server).
            HANDLE serverHandle = nullptr;
            // Use the same desired access in `//base/memory/platform_shared_memory_region_win.cc`
            // in Chromium.
            DWORD desiredAccess = FILE_MAP_READ | FILE_MAP_WRITE | SECTION_QUERY | READ_CONTROL;
            if (!DuplicateHandle(GetCurrentProcess(), mHandle, GetCurrentProcess(), &serverHandle,
                                 desiredAccess, FALSE, 0)) {
                serverHandle = nullptr;
            }
            memcpy(ptr, &serverHandle, sizeof(serverHandle));
        }

        bool IsEmpty() const override { return mHandle == nullptr && mMappedPointer == nullptr; }

        void* GetMappedPointer() const override { return mMappedPointer; }

      private:
        HANDLE mHandle = nullptr;
        raw_ptr<void> mMappedPointer = nullptr;
    };

    class ReadHandleWithImportableTransferBufferImpl : public ReadHandle {
      public:
        explicit ReadHandleWithImportableTransferBufferImpl(
            ImportableTransferBuffer* transferBuffer)
            : mTransferBuffer(transferBuffer) {
            DAWN_ASSERT(transferBuffer != nullptr && !transferBuffer->IsEmpty());
        }

        // Send the name and the size of the file mapping handle to wire server
        size_t SerializeCreateSize() override { return mTransferBuffer->SerializeCreateSize(); }

        void SerializeCreate(void* ptr) override { mTransferBuffer->SerializeCreate(ptr); }

        const void* GetData() override { return mTransferBuffer->GetMappedPointer(); }

        // We don't need to deserialize anything as the memory has already been in the shared
        // memory.
        bool DeserializeDataUpdate(const void* deserializePointer,
                                   size_t deserializeSize,
                                   size_t offset,
                                   size_t size) override {
            return true;
        }

      private:
        raw_ptr<ImportableTransferBuffer> mTransferBuffer;
    };

    class WriteHandleWithImportableTransferBufferImpl : public WriteHandle {
      public:
        explicit WriteHandleWithImportableTransferBufferImpl(
            ImportableTransferBuffer* transferBuffer)
            : mTransferBuffer(transferBuffer) {
            DAWN_ASSERT(transferBuffer != nullptr && !transferBuffer->IsEmpty());
        }

        // Send the name and the size of the file mapping handle to wire server
        size_t SerializeCreateSize() override { return mTransferBuffer->SerializeCreateSize(); }

        void SerializeCreate(void* ptr) override { mTransferBuffer->SerializeCreate(ptr); }

        void* GetData() override { return mTransferBuffer->GetMappedPointer(); }

        // We don't need to serialize anything as the memory has already been in the shared memory.
        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override { return 0; }
        void SerializeDataUpdate(void* serializePointer, size_t offset, size_t size) override {}

      private:
        raw_ptr<ImportableTransferBuffer> mTransferBuffer;
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
    std::unique_ptr<ImportableTransferBuffer> CreateImportableTransferBuffer(size_t size) override {
        return std::make_unique<ImportableTransferBufferImpl>(size);
    }

    ReadHandle* CreateReadHandleWithImportableTransferBuffer(
        ImportableTransferBuffer* transferBuffer) override {
        return new ReadHandleWithImportableTransferBufferImpl(transferBuffer);
    }

    WriteHandle* CreateWriteHandleWithImportableTransferBuffer(
        ImportableTransferBuffer* transferBuffer) override {
        return new WriteHandleWithImportableTransferBufferImpl(transferBuffer);
    }
#endif
};

std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService() {
    return std::make_unique<InlineMemoryTransferService>();
}

}  // namespace dawn::wire::client
