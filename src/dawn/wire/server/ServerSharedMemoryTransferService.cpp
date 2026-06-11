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

#include "dawn/wire/WireServer.h"
#include "src/dawn/wire/server/Server.h"
#include "src/utils/assert.h"

namespace dawn::wire::server {

class SharedMemoryTransferService : public MemoryTransferService {
  public:
    class ReadHandleImpl : public ReadHandle {
      public:
        ReadHandleImpl(std::shared_ptr<TransferBufferManager> manager,
                       uint64_t bufferId,
                       size_t size)
            : mManager(std::move(manager)), mBufferId(bufferId), mSize(size) {}

        ~ReadHandleImpl() override { mManager->ReleaseBuffer(mBufferId); }

        size_t SizeOfSerializeDataUpdate(size_t offset, size_t size) override {
            // Shared memory: no serialization needed.
            return 0;
        }

        void SerializeDataUpdate(std::span<const uint8_t> data,
                                 size_t offset,
                                 std::span<char> serializeData) override {
            // Copy GPU data directly into the shared buffer so the client can read it.
            std::span<const uint8_t> buffer = mManager->GetBufferPointer(mBufferId);
            if (!buffer.empty() && offset + data.size() <= mSize) {
                std::span<const uint8_t> targetSubSpan = buffer.subspan(offset, data.size());
                std::span<uint8_t> targetSubSpanNonConst(const_cast<uint8_t*>(targetSubSpan.data()),
                                                         targetSubSpan.size());
                std::copy(data.begin(), data.end(), targetSubSpanNonConst.begin());
            }
        }

      private:
        std::shared_ptr<TransferBufferManager> mManager;
        uint64_t mBufferId;
        size_t mSize;
    };

    class WriteHandleImpl : public WriteHandle {
      public:
        WriteHandleImpl(std::shared_ptr<TransferBufferManager> manager,
                        uint64_t bufferId,
                        size_t size)
            : mManager(std::move(manager)), mBufferId(bufferId), mSize(size) {}

        ~WriteHandleImpl() override { mManager->ReleaseBuffer(mBufferId); }

        bool DeserializeDataUpdate(std::span<const uint8_t> deserializeData,
                                   std::span<uint8_t> target,
                                   size_t offset) override {
            std::span<const uint8_t> source = mManager->GetBufferPointer(mBufferId);
            if (source.empty()) {
                return false;
            }
            if (offset + target.size() > mSize) {
                return false;
            }
            std::span<const uint8_t> sourceSubSpan = source.subspan(offset, target.size());
            std::copy(sourceSubSpan.begin(), sourceSubSpan.end(), target.begin());
            return true;
        }

      private:
        uint8_t* GetSourceData() const override {
            return const_cast<uint8_t*>(mManager->GetBufferPointer(mBufferId).data());
        }

        size_t GetSourceSize() const override { return mSize; }

        std::shared_ptr<TransferBufferManager> mManager;
        uint64_t mBufferId;
        size_t mSize;
    };

    explicit SharedMemoryTransferService(std::shared_ptr<TransferBufferManager> manager)
        : mManager(std::move(manager)) {}
    ~SharedMemoryTransferService() override = default;

    bool DeserializeReadHandle(const void* deserializePointer,
                               size_t deserializeSize,
                               ReadHandle** readHandle) override {
        DAWN_ASSERT(readHandle != nullptr);
        if (deserializeSize < sizeof(TransferBufferHandle)) {
            return false;
        }

        TransferBufferHandle handle = *static_cast<const TransferBufferHandle*>(deserializePointer);

        if (!mManager->HasBuffer(handle.id)) {
            return false;
        }
        size_t actualSize = mManager->GetBufferSize(handle.id);
        if (actualSize < handle.size) {
            return false;
        }

        *readHandle = new ReadHandleImpl(mManager, handle.id, handle.size);
        return true;
    }

    bool DeserializeWriteHandle(const void* deserializePointer,
                                size_t deserializeSize,
                                WriteHandle** writeHandle) override {
        DAWN_ASSERT(writeHandle != nullptr);
        if (deserializeSize < sizeof(TransferBufferHandle)) {
            return false;
        }

        TransferBufferHandle handle = *static_cast<const TransferBufferHandle*>(deserializePointer);

        if (!mManager->HasBuffer(handle.id)) {
            return false;
        }
        size_t actualSize = mManager->GetBufferSize(handle.id);
        if (actualSize < handle.size) {
            return false;
        }

        *writeHandle = new WriteHandleImpl(mManager, handle.id, handle.size);
        return true;
    }

  private:
    std::shared_ptr<TransferBufferManager> mManager;
};

std::unique_ptr<MemoryTransferService> CreateSimpleMemoryTransferService(
    std::shared_ptr<TransferBufferManager> transferBufferManager) {
    return std::make_unique<SharedMemoryTransferService>(std::move(transferBufferManager));
}

}  // namespace dawn::wire::server
