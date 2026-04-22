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

#include "dawn/wire/WireServer.h"

#include "dawn/wire/server/Server.h"
#include "src/dawn/common/Compiler.h"

namespace dawn::wire {

WireServer::WireServer(const WireServerDescriptor& descriptor)
    : mImpl(server::Server::Create(*descriptor.procs,
                                   descriptor.serializer,
                                   descriptor.memoryTransferService,
                                   descriptor.useSpontaneousCallbacks)) {}

WireServer::~WireServer() {
    mImpl.reset();
}

const volatile char* WireServer::HandleCommands(const volatile char* commands, size_t size) {
    return mImpl->HandleCommands(commands, size);
}

bool WireServer::InjectBuffer(WGPUBuffer buffer, const Handle& handle, const Handle& deviceHandle) {
    return mImpl->InjectBuffer(buffer, handle, deviceHandle) == WireResult::Success;
}

bool WireServer::InjectTexture(WGPUTexture texture,
                               const Handle& handle,
                               const Handle& deviceHandle) {
    return mImpl->InjectTexture(texture, handle, deviceHandle) == WireResult::Success;
}

bool WireServer::InjectSurface(WGPUSurface surface,
                               const Handle& handle,
                               const Handle& instanceHandle) {
    return mImpl->InjectSurface(surface, handle, instanceHandle) == WireResult::Success;
}

bool WireServer::InjectInstance(WGPUInstance instance, const Handle& handle) {
    return mImpl->InjectInstance(instance, handle) == WireResult::Success;
}

WGPUDevice WireServer::GetDevice(uint32_t id, uint32_t generation) {
    return mImpl->GetDevice(id, generation);
}

bool WireServer::IsDeviceKnown(WGPUDevice device) const {
    return mImpl->IsDeviceKnown(device);
}

namespace server {
MemoryTransferService::MemoryTransferService() = default;

MemoryTransferService::~MemoryTransferService() = default;

MemoryTransferService::ReadHandle::ReadHandle() = default;

MemoryTransferService::ReadHandle::~ReadHandle() = default;

MemoryTransferService::WriteHandle::WriteHandle() = default;

MemoryTransferService::WriteHandle::~WriteHandle() = default;

bool MemoryTransferService::WriteHandle::DeserializeDataUpdate(
    std::span<const uint8_t> deserializeData,
    std::span<uint8_t> target,
    size_t offset) {
    if (offset > std::numeric_limits<size_t>::max() - target.size()) {
        return false;
    }

    if (target.data() != nullptr && reinterpret_cast<uintptr_t>(target.data()) < offset) {
        return false;
    }

    // In the new `DeserializeDataUpdate` (with 3 parameters) `target` should already be the
    // correct subspan of the buffer to write into, so we just need to check for OOB and
    // then write into it.
    // However, in the old `DeserializeDataUpdate` (with 4 parameters) implementation in
    // Chromium, `target` is always the full buffer and `offset` is used to offset into both
    // the target data and the shmem pointer, so we need to do the same offsetting here to
    // be compatible with both implementations.
    uint8_t* bufferStart = DAWN_UNSAFE_TODO(target.data() - offset);
    size_t lengthFromStart = target.size() + offset;
    SetTarget(bufferStart);
    SetDataLength(lengthFromStart);
    return DeserializeDataUpdate(deserializeData.data(), deserializeData.size(), offset,
                                 target.size());
}

void MemoryTransferService::WriteHandle::SetTarget(void* data) {
    mTargetData = static_cast<uint8_t*>(data);
}
void MemoryTransferService::WriteHandle::SetDataLength(size_t dataLength) {
    mDataLength = dataLength;
}
std::span<uint8_t> MemoryTransferService::WriteHandle::GetTarget() const {
    DAWN_ASSERT(mTargetData != nullptr);
    return std::span<uint8_t>(mTargetData, mDataLength);
}

}  // namespace server

}  // namespace dawn::wire
