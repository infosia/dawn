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

#include "dawn/native/webgpu/BufferWGPU.h"

#include <string>
#include <utility>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/Buffer.h"
#include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/PhysicalDeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"

namespace dawn::native::webgpu {

// static
ResultOrError<Ref<Buffer>> Buffer::Create(Device* device,
                                          const UnpackedPtr<BufferDescriptor>& descriptor) {
    auto desc = ToAPI(*descriptor);
    WGPUBuffer innerBuffer = device->wgpu.deviceCreateBuffer(device->GetInnerHandle(), desc);

    Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor, innerBuffer));
    return std::move(buffer);
}

Buffer::Buffer(Device* device,
               const UnpackedPtr<BufferDescriptor>& descriptor,
               WGPUBuffer innerBuffer)
    : BufferBase(device, descriptor), mInnerBuffer(innerBuffer) {
    mAllocatedSize = GetSize();
}

WGPUBuffer Buffer::GetInnerHandle() const {
    return mInnerBuffer;
}

bool Buffer::IsCPUWritableAtCreation() const {
    return false;
}

MaybeError Buffer::MapAtCreationImpl() {
    DAWN_UNREACHABLE();
}

void Buffer::CopyFromStaging(BufferBase* staging,
                             uint64_t sourceOffset,
                             uint64_t destinationOffset,
                             uint64_t size) {
    DAWN_UNREACHABLE();
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    GetDevice()->GetQueue()->IncrementLastSubmittedCommandSerial();

    struct MapAsyncResult {
        WGPUMapAsyncStatus status;
        std::string message;
    } mapAsyncResult = {};

    WGPUBufferMapCallbackInfo innerCallbackInfo = {};
    innerCallbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    innerCallbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                                    void* result_param, void* userdata_param) {
        MapAsyncResult* result = reinterpret_cast<MapAsyncResult*>(result_param);
        result->status = status;
        result->message = ToString(message);
    };
    innerCallbackInfo.userdata1 = &mapAsyncResult;
    innerCallbackInfo.userdata2 = this;

    auto& wgpu = ToBackend(GetDevice())->wgpu;

    // For now we simply wait on the future.
    WGPUFutureWaitInfo waitInfo = {};
    waitInfo.future = wgpu.bufferMapAsync(mInnerBuffer, static_cast<WGPUMapMode>(mode), offset,
                                          size, innerCallbackInfo);
    wgpu.instanceWaitAny(
        ToBackend(GetDevice()->GetPhysicalDevice())->GetBackend()->GetInnerInstance(), 1, &waitInfo,
        UINT64_MAX);

    if (mapAsyncResult.status != WGPUMapAsyncStatus_Success) {
        return DAWN_INTERNAL_ERROR(mapAsyncResult.message);
    }

    mMappedData = const_cast<void*>(wgpu.bufferGetConstMappedRange(mInnerBuffer, 0, GetSize()));
    return {};
}

void* Buffer::GetMappedPointer() {
    return mMappedData;
}

void Buffer::UnmapImpl() {
    if (mInnerBuffer) {
        ToBackend(GetDevice())->wgpu.bufferUnmap(mInnerBuffer);
    }
    mMappedData = nullptr;
}

void Buffer::DestroyImpl() {
    // TODO(crbug.com/dawn/831): DestroyImpl is called from two places.
    // - It may be called if the buffer is explicitly destroyed with APIDestroy.
    //   This case is NOT thread-safe and needs proper synchronization with other
    //   simultaneous uses of the buffer.
    // - It may be called when the last ref to the buffer is dropped and the buffer
    //   is implicitly destroyed. This case is thread-safe because there are no
    //   other threads using the buffer since there are no other live refs.

    if (mInnerBuffer) {
        ToBackend(GetDevice())->wgpu.bufferRelease(mInnerBuffer);
        mInnerBuffer = nullptr;
    }

    BufferBase::DestroyImpl();
}

}  // namespace dawn::native::webgpu
