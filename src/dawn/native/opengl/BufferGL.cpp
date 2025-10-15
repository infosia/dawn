// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/opengl/BufferGL.h"

#include <algorithm>
#include <future>
#include <utility>
#include <vector>

#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/QueueGL.h"
#include "dawn/native/opengl/UtilsGL.h"

namespace dawn::native::opengl {

// Buffer

// static
ResultOrError<Ref<Buffer>> Buffer::CreateInternalBuffer(Device* device,
                                                        const BufferDescriptor* descriptor,
                                                        bool shouldLazyClear) {
    Ref<Buffer> buffer = AcquireRef(new Buffer(device, Unpack(descriptor)));
    DAWN_TRY(buffer->Initialize(shouldLazyClear, descriptor->mappedAtCreation));

    if (descriptor->mappedAtCreation) {
        [[maybe_unused]] bool usingStagingBuffer;
        DAWN_TRY_ASSIGN(usingStagingBuffer, buffer->MapAtCreationInternal());
    }

    return std::move(buffer);
}

// static
ResultOrError<Ref<Buffer>> Buffer::Create(Device* device,
                                          const UnpackedPtr<BufferDescriptor>& descriptor) {
    Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor));
    DAWN_TRY(buffer->Initialize(false, descriptor->mappedAtCreation));
    return buffer;
}

Buffer::Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor)
    : BufferBase(device, descriptor), mHandle(AcquireRef(new FutureHandle())) {
    // Allocate at least 4 bytes so clamped accesses are always in bounds.
    // Align with 4 byte to avoid out-of-bounds access issue in compute emulation for 2 byte
    // element.
    mAllocatedSize = Align(std::max(GetSize(), uint64_t(4u)), uint64_t(4u));
}

MaybeError Buffer::Initialize(bool shouldLazyClear, bool isMappedAtCreation) {
    Device* device = ToBackend(GetDevice());

    if (!shouldLazyClear) {
        SetInitialized(true);
    }

    if (GetUsage() & kMappableBufferUsages) {
        mShadowCopy.resize(mAllocatedSize);
    }

    TrackUsage();

    Ref<Buffer> self = this;
    // TODO(451928481): We cannot defer GL buffer's creation yet because it would prevent error
    // scope from checking OOM error immediately. Consider a new feature to allow defer GL
    // resources' creation.
    return device->ExecuteGLWork(
        [self, device, isMappedAtCreation](const OpenGLFunctions& gl) mutable -> MaybeError {
            GLuint glHandle = 0;
            DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
            DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));

            if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
                !isMappedAtCreation) {
                std::vector<uint8_t> clearValues(self->mAllocatedSize, 1u);
                if (!self->mShadowCopy.empty()) {
                    memcpy(self->mShadowCopy.data(), clearValues.data(), self->mAllocatedSize);
                }
                DAWN_GL_TRY_ALWAYS_CHECK(gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize,
                                                        clearValues.data(), GL_STATIC_DRAW));
            } else {
                DAWN_GL_TRY_ALWAYS_CHECK(
                    gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize, nullptr, GL_STATIC_DRAW));
            }
            self->mHandle->Set(glHandle);
            return {};
        });
}

Buffer::~Buffer() = default;

const FutureHandle& Buffer::GetHandle() const {
    return *mHandle.Get();
}

MaybeError Buffer::EnsureDataInitialized(bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero());
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::EnsureDataInitializedAsDestination(uint64_t offset,
                                                      uint64_t size,
                                                      bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    if (IsFullBufferRange(offset, size)) {
        SetInitialized(true);
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero());
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::EnsureDataInitializedAsDestination(const CopyTextureToBufferCmd* copy,
                                                      bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    if (IsFullBufferOverwrittenInTextureToBufferCopy(copy)) {
        SetInitialized(true);
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero());
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::InitializeToZero() {
    DAWN_ASSERT(NeedsInitialization());

    const uint64_t size = GetAllocatedSize();
    const std::vector<uint8_t> clearValues(size, 0u);

    DAWN_TRY(Write(0, clearValues.data(), size));

    TrackUsage();
    SetInitialized(true);
    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    return mShadowCopy.size() > 0;
}

MaybeError Buffer::MapAtCreationImpl() {
    DAWN_ASSERT(IsCPUWritableAtCreation());
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    // Do nothing, shadow copy is already mapped.
    return {};
}

void Buffer::FinalizeMapImpl() {}

void* Buffer::GetMappedPointerImpl() {
    // The mapping offset has already been removed.
    return mShadowCopy.data();
}

void Buffer::UnmapImpl() {
    Device* device = ToBackend(GetDevice());
    // Unmap() might be called in DestroyImpl(), we need to check whether it is the case or not.
    if (!this->TryAddRef()) {
        // TryAddRef() fails mean we are being destroyed, skip further steps.
        return;
    }

    // adopt ref
    Ref<Buffer> self = AcquireRef(this);

    if (!(GetUsage() & wgpu::BufferUsage::MapWrite)) {
        return;
    }

    // Copy shadow copy to real buffer
    IgnoreErrors(device->AddGLWork([self](const OpenGLFunctions& gl) -> MaybeError {
        return self->WriteRealBuffer(gl, 0, self->mShadowCopy.data(), self->mShadowCopy.size());
    }));
}

MaybeError Buffer::Write(uint64_t offset, const void* data, size_t size) {
    Device* device = ToBackend(GetDevice());
    Ref<Buffer> self = this;

    std::vector<uint8_t> copyData(size);
    memcpy(copyData.data(), data, size);

    if (!self->mShadowCopy.empty()) {
        memcpy(self->mShadowCopy.data() + offset, data, size);
    }

    return device->AddGLWork(
        [self, offset, copyData = std::move(copyData)](const OpenGLFunctions& gl) -> MaybeError {
            return self->WriteRealBuffer(gl, offset, copyData.data(), copyData.size());
        });
}

MaybeError Buffer::WriteRealBuffer(const OpenGLFunctions& gl,
                                   uint64_t offset,
                                   const void* data,
                                   size_t size) {
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
    DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, offset, size, data));

    return {};
}

bool Buffer::NeedsCopyDstTracking() const {
    return !mShadowCopy.empty();
}

MaybeError Buffer::OnCopiedTo() {
    if (mShadowCopy.empty()) {
        return {};
    }

    Device* device = ToBackend(GetDevice());
    const OpenGLFunctions& gl = device->GetGL();

    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));

    void* data = DAWN_GL_TRY_ALWAYS_CHECK(
        gl, MapBufferRange(GL_ARRAY_BUFFER, 0, GetSize(), GL_MAP_READ_BIT));

    DAWN_ASSERT(mShadowCopy.size());
    DAWN_ASSERT(data);
    memcpy(mShadowCopy.data(), data, GetSize());

    DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));
    return {};
}

void Buffer::DestroyImpl() {
    Device* device = ToBackend(GetDevice());
    Ref<FutureHandle> handle = mHandle;

    IgnoreErrors(device->AddGLWork([handle](const OpenGLFunctions& gl) -> MaybeError {
        GLuint realHandle = handle->GetOrDefault();
        DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &realHandle));
        return {};
    }));

    BufferBase::DestroyImpl();
}

}  // namespace dawn::native::opengl
