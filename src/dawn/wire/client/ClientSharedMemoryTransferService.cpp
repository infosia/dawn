// Copyright 2026 The Dawn & Tint Authors
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

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "dawn/wire/WireClient.h"
#include "src/dawn/wire/client/Client.h"
#include "src/utils/assert.h"

namespace dawn::wire::client {

class SharedMemoryTransferService : public MemoryTransferService {
    class ReadHandleImpl : public ReadHandle {
      public:
        ReadHandleImpl(std::shared_ptr<TransferBufferManager> manager,
                       uint64_t bufferId,
                       size_t size)
            : mManager(std::move(manager)),
              mBufferId(bufferId),
              mSize(size),
              mBufferSpan(mManager->GetBufferPointer(mBufferId)) {}

        ~ReadHandleImpl() override { mManager->ReleaseBuffer(mBufferId); }

        size_t SerializeCreateSize() override { return sizeof(TransferBufferHandle); }

        void SerializeCreate(void* serializePointer) override {
            TransferBufferHandle handle = {mBufferId, mSize};
            *static_cast<TransferBufferHandle*>(serializePointer) = handle;
            mManager->AddBufferRef(mBufferId);
        }

        const void* GetData() override { return static_cast<const void*>(mBufferSpan.data()); }

        bool DeserializeDataUpdate(std::span<const uint8_t> deserializeData,
                                   size_t offset) override {
            // With shared memory the data is already in the buffer. If the server sends
            // an empty update, nothing needs to happen.
            DAWN_ASSERT(deserializeData.empty());
            return true;
        }

      private:
        std::shared_ptr<TransferBufferManager> mManager;
        uint64_t mBufferId;
        size_t mSize;
        std::span<const uint8_t> mBufferSpan;
    };

    class WriteHandleImpl : public WriteHandle {
      public:
        WriteHandleImpl(std::shared_ptr<TransferBufferManager> manager,
                        uint64_t bufferId,
                        size_t size)
            : mManager(std::move(manager)),
              mBufferId(bufferId),
              mSize(size),
              mBufferSpan(mManager->GetBufferPointer(mBufferId)) {}

        ~WriteHandleImpl() override { mManager->ReleaseBuffer(mBufferId); }

        size_t SerializeCreateSize() override { return sizeof(TransferBufferHandle); }

        void SerializeCreate(void* serializePointer) override {
            TransferBufferHandle handle = {mBufferId, mSize};
            *static_cast<TransferBufferHandle*>(serializePointer) = handle;
            // Add reference to the buffer because it will be passed to the dawn server. Without
            // the reference here when dawn client is destroyed before the command is flushed to the
            // dawn server, the buffer will be destroyed and cannot be accessed on dawn server.
            mManager->AddBufferRef(mBufferId);
        }

        void* GetData() override { return const_cast<uint8_t*>(mBufferSpan.data()); }

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override {
            // Shared memory: no serialization needed.
            return 0;
        }

        void SerializeDataUpdate(std::span<char> serializeData, size_t offset) override {
            // Shared memory: data is already visible to the server.
        }

      private:
        std::shared_ptr<TransferBufferManager> mManager;
        uint64_t mBufferId;
        size_t mSize;
        std::span<const uint8_t> mBufferSpan;
    };

  public:
    explicit SharedMemoryTransferService(std::shared_ptr<TransferBufferManager> manager)
        : mManager(std::move(manager)) {}
    ~SharedMemoryTransferService() override = default;

    ReadHandle* CreateReadHandle(size_t size) override {
        uint64_t id = mManager->CreateBuffer(size);
        if (id == 0) {
            return nullptr;
        }
        return new ReadHandleImpl(mManager, id, size);
    }

    WriteHandle* CreateWriteHandle(size_t size) override {
        uint64_t id = mManager->CreateBuffer(size);
        if (id == 0) {
            return nullptr;
        }
        return new WriteHandleImpl(mManager, id, size);
    }

  private:
    std::shared_ptr<TransferBufferManager> mManager;
};

std::unique_ptr<MemoryTransferService> CreateSimpleMemoryTransferService(
    std::shared_ptr<TransferBufferManager> transferBufferManager) {
    return std::make_unique<SharedMemoryTransferService>(std::move(transferBufferManager));
}

}  // namespace dawn::wire::client
