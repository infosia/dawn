// Copyright 2021 The Dawn & Tint Authors
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

#include "dawn/wire/Wire.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "src/dawn/common/Alloc.h"

namespace dawn::wire {

CommandSerializer::CommandSerializer() = default;
CommandSerializer::~CommandSerializer() = default;

void CommandSerializer::OnSerializeError() {}

CommandHandler::CommandHandler() = default;
CommandHandler::~CommandHandler() = default;

TransferBufferManager::TransferBufferManager() = default;
TransferBufferManager::~TransferBufferManager() = default;

uint64_t TransferBufferManager::CreateBuffer(size_t size) {
    // Use AllocNoThrow to avoid throwing in -fno-exceptions mode when
    // the requested size is too large (e.g. OOM tests).
    auto rawData = std::unique_ptr<uint8_t[]>(AllocNoThrow<uint8_t>(size));
    if (!rawData) {
        return 0;
    }
    // Zero-initialize the allocation.
    std::fill_n(rawData.get(), size, uint8_t{0});

    std::scoped_lock lock(mMutex);
    uint64_t id = mNextId++;
    Buffer buffer = {};
    buffer.data = std::move(rawData);
    buffer.size = size;
    buffer.refCount = 1;
    mBuffers[id] = std::move(buffer);
    return id;
}

bool TransferBufferManager::HasBuffer(uint64_t id) const {
    std::scoped_lock lock(mMutex);
    return mBuffers.contains(id);
}

void TransferBufferManager::AddBufferRef(uint64_t id) {
    std::scoped_lock lock(mMutex);
    auto it = mBuffers.find(id);
    if (it != mBuffers.end()) {
        it->second.refCount++;
    }
}

void TransferBufferManager::ReleaseBuffer(uint64_t id) {
    std::scoped_lock lock(mMutex);
    auto it = mBuffers.find(id);
    if (it != mBuffers.end()) {
        --it->second.refCount;
        if (it->second.refCount == 0) {
            mBuffers.erase(it);
        }
    }
}

std::span<uint8_t> TransferBufferManager::GetBufferPointer(uint64_t id) const {
    std::scoped_lock lock(mMutex);
    auto it = mBuffers.find(id);
    if (it == mBuffers.end()) {
        return {};
    }
    return std::span<uint8_t>(it->second.data.get(), it->second.size);
}

size_t TransferBufferManager::GetBufferSize(uint64_t id) const {
    std::scoped_lock lock(mMutex);
    auto it = mBuffers.find(id);
    if (it == mBuffers.end()) {
        return 0;
    }
    return it->second.size;
}

}  // namespace dawn::wire
