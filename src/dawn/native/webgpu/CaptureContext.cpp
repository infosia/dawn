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

#include "src/dawn/native/webgpu/CaptureContext.h"

#include <concepts>
#include <vector>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/Commands.h"
#include "dawn/native/Device.h"
#include "dawn/native/ObjectBase.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/Forward.h"
#include "dawn/native/webgpu/QueueWGPU.h"
#include "dawn/replay/Serialization.h"

namespace dawn::native::webgpu {

constexpr uint64_t kCopyBufferSize = 1024 * 1024;

namespace {

template <typename T>
concept IsInt32 = std::is_same_v<T, int32_t>;

template <typename T>
concept IsUint32 = std::is_same_v<T, uint32_t>;

template <typename T>
concept IsUint64 = std::is_same_v<T, uint64_t>;

template <typename T>
concept IsSizeT = std::is_same_v<T, size_t>;

template <typename T>
concept IsFloat = std::is_same_v<T, float>;

template <typename T>
concept IsDouble = std::is_same_v<T, double>;

template <typename T>
concept IsBool = std::is_same_v<T, bool>;

template <typename T>
concept IsDerivedFromApiObjectBase = std::is_base_of_v<ApiObjectBase, T>;

void Capture(CaptureContext& captureContext, IsDouble auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsFloat auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsInt32 auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsUint32 auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsUint64 auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsSizeT auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsBool auto value) {
    captureContext.WriteCommandBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, replay::RootCommand command) {
    Capture(captureContext, static_cast<uint32_t>(command));
}

void Capture(CaptureContext& captureContext, wgpu::BufferUsage usage) {
    Capture(captureContext, static_cast<uint64_t>(usage));
}

void Capture(CaptureContext& captureContext, const std::string& value) {
    Capture(captureContext, value.size());
    captureContext.WriteCommandBytes(value.data(), value.size());
}

void Capture(CaptureContext& captureContext, replay::ObjectType objectType) {
    Capture(captureContext, static_cast<uint32_t>(objectType));
}

void Capture(CaptureContext& captureContext, replay::ObjectType objectType, replay::ObjectId id) {
    Capture(captureContext, replay::RootCommand::CreateResource);
    Capture(captureContext, objectType);
    Capture(captureContext, id);
}

template <typename T>
void CaptureRef(CaptureContext& captureContext, const Ref<T>& ref) {
    Capture(captureContext, static_cast<uint64_t>(captureContext.GetId(ref.Get())));
}

template <IsDerivedFromApiObjectBase T>
void CaptureRef(CaptureContext& captureContext, const T* object) {
    Capture(captureContext, static_cast<uint64_t>(captureContext.GetId(object)));
}

}  // namespace

void AddReferenced(CaptureContext& captureContext, const BufferBase* buffer) {}

template <>
void CaptureCreation<BufferBase>(CaptureContext& captureContext,
                                 replay::ObjectId id,
                                 const BufferBase* buffer) {
    Capture(captureContext, replay::ObjectType::Buffer, id);
    Capture(captureContext, buffer->GetLabel());
    Capture(captureContext, buffer->GetSize());
    Capture(captureContext, buffer->GetUsage());
}

CaptureContext::CaptureContext(Device* device,
                               std::ostream& commandStream,
                               std::ostream& contentStream)
    : mDevice(device), mCommandStream(commandStream), mContentStream(contentStream) {}

CaptureContext::~CaptureContext() {
    if (mCopyBuffer) {
        mDevice->wgpu.bufferDestroy(mCopyBuffer);
    }
}

WGPUBuffer CaptureContext::GetCopyBuffer() {
    if (!mCopyBuffer) {
        WGPUBufferDescriptor desc = {};
        desc.size = kCopyBufferSize;
        desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        mCopyBuffer = mDevice->wgpu.deviceCreateBuffer(mDevice->GetInnerHandle(), &desc);
    }
    return mCopyBuffer;
}

void CaptureContext::WriteContentBytes(const void* data, size_t size) {
    mContentStream.write(reinterpret_cast<const char*>(data), size);
}

void CaptureContext::WriteCommandBytes(const void* data, size_t size) {
    mCommandStream.write(reinterpret_cast<const char*>(data), size);
}

void CaptureContext::CaptureQueueWriteBuffer(const BufferBase* buffer,
                                             uint64_t bufferOffset,
                                             const void* data,
                                             size_t size) {
    AddResource(buffer);
    Capture(*this, replay::RootCommand::WriteBuffer);
    CaptureRef(*this, buffer);
    Capture(*this, bufferOffset);
    Capture(*this, size);
    WriteContentBytes(data, size);
}

}  // namespace dawn::native::webgpu
