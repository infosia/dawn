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
    Ref<Buffer> buffer;
    DAWN_TRY_ASSIGN(buffer, Buffer::Create(device, Unpack(descriptor)));

    if (!shouldLazyClear) {
        buffer->SetInitialized(true);
    }

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

    DAWN_TRY(device->EnqueueGL([device, buffer, isMappedAtCreation = descriptor->mappedAtCreation](
                                   const OpenGLFunctions& gl) -> MaybeError {
        GLuint glHandle = 0;
        DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
        buffer->mHandle->Set(glHandle);
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));

        // The buffers with mappedAtCreation == true will be initialized in
        // BufferBase::MapAtCreation().
        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
            !isMappedAtCreation) {
            std::vector<uint8_t> clearValues(buffer->mAllocatedSize, 1u);
            DAWN_GL_TRY_ALWAYS_CHECK(gl, BufferData(GL_ARRAY_BUFFER, buffer->mAllocatedSize,
                                                    clearValues.data(), GL_STATIC_DRAW));
        } else {
            DAWN_GL_TRY_ALWAYS_CHECK(
                gl, BufferData(GL_ARRAY_BUFFER, buffer->mAllocatedSize, nullptr, GL_STATIC_DRAW));
        }

        return {};
    }));

    buffer->TrackUsage();

    return std::move(buffer);
}

Buffer::Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor)
    : BufferBase(device, descriptor), mHandle(AcquireRef(new FutureHandle())) {
    // Allocate at least 4 bytes so clamped accesses are always in bounds.
    // Align with 4 byte to avoid out-of-bounds access issue in compute emulation for 2 byte
    // element.
    uint64_t alignment = 4u;
    // Round uniform buffer sizes up to a multiple of 16 bytes since Tint will polyfill them as
    // array<vec4u, ...>.
    if (GetUsage() & wgpu::BufferUsage::Uniform) {
        alignment = 16u;
    }
    mAllocatedSize = Align(std::max(GetSize(), uint64_t(4u)), alignment);

    if (descriptor->mappedAtCreation && GetSize() != 0 &&
        device->IsToggleEnabled(Toggle::GLDefer)) {
        // Allocate shadow copy for mapAtCreation. However, we only do so if GetSize() is non-zero.
        // This is because the front-end never actually calls into backend's MapAtCreationImpl &
        // UnmapImpl for zero-sized buffers. Thus shadow copy is basically unused in that case.
        mShadowCopy.resize(mAllocatedSize);
    }
}

Buffer::~Buffer() = default;

GLuint Buffer::GetHandle() const {
    return mHandle->Get();
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

    Device* device = ToBackend(GetDevice());
    Ref<Buffer> self = this;
    DAWN_TRY(device->EnqueueGL([self](const OpenGLFunctions& gl) -> MaybeError {
        const uint64_t size = self->GetAllocatedSize();
        const std::vector<uint8_t> clearValues(size, 0u);

        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, self->GetHandle()));
        DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, 0, size, clearValues.data()));

        return {};
    }));

    ToBackend(GetDevice())->IncrementLazyClearCountForTesting();

    TrackUsage();
    SetInitialized(true);
    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    // Will be handled in FinalizeMapImpl().
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    DAWN_TRY(EnsureDataInitialized());

    // The real map will be handled in FinalizeMapImpl() when buffer is free of any GPU use.
    return {};
}

MaybeError Buffer::FinalizeMapImpl(BufferState newState) {
    if (newState == BufferState::MappedAtCreation) {
        if (GetSize() == 0) {
            // Return early if MappedAtCreation() is called with size = 0. Front-end skips calling
            // MapAtCreationImpl() & UnmapImpl() if size=0. However, it still calls
            // FinalizeMapImpl(). MaptAtCreation with zero-size is supposed to be no-op.
            return {};
        }

        if (!mShadowCopy.empty()) {
            // If shadow copy is enabled, use its pointer directly.
            mMappedData = mShadowCopy.data();
            return {};
        }
    }

    DAWN_ASSERT(newState == BufferState::Mapped || newState == BufferState::MappedAtCreation);
    DAWN_ASSERT(mShadowCopy.empty());

    Device* device = ToBackend(GetDevice());
    auto deviceGuard = device->GetGuard();
    void* mappedData = nullptr;
    auto size = MapSize();
    auto offset = MapOffset();
    const auto mode = MapMode();

    // It is an error to map an empty range in OpenGL. We always have at least a 4-byte
    // buffer
    // so we extend the range to be 4 bytes.
    if (size == 0) {
        if (offset != 0) {
            offset -= 4;
        }
        size = 4;
    }

    // Since FinalizeMap() is called when EventManager detects that all GPU uses of this buffer have
    // completed, it's safe to use ExecuteGL() to trigger GL command immediately without waiting for
    // pending GL commands.
    // TODO(451928481):
    // - We assume this is called on a thread that can make GL context current, e.g. playback thread
    // in Skia (denoted as GL thread).
    // - This requires client to call MapAsync(AllowSpontaneous) or Tick()/WaitAny()/ProcessEvents()
    // on GL thread. The latter is required if non-AllowSpontaneous callback mode is used.
    return device->ExecuteGL(
        ExecutionQueueBase::SubmitMode::Passive,
        [&](const OpenGLFunctions& gl) mutable -> MaybeError {
            // UnmapImpl enqueues an asynchronous unmap operation, which may not have executed when
            // this function is called. We need to ensure the buffer is unmapped before we can map
            // it again. Calling DoGLUnmapIfNeeded() directly handles this by unmapping only if it
            // is currently mapped.
            DoGLUnmapIfNeeded(gl);

            // This does GPU->CPU synchronization, we could require a high
            // version of OpenGL that would let us map the buffer unsynchronized.
            DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
            if (mode & wgpu::MapMode::Read) {
                mappedData = DAWN_GL_TRY_ALWAYS_CHECK(
                    gl, MapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_READ_BIT));
            } else {
                DAWN_ASSERT(mode & wgpu::MapMode::Write);
                mappedData = DAWN_GL_TRY_ALWAYS_CHECK(
                    gl, MapBufferRange(GL_ARRAY_BUFFER, offset, size,
                                       GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
            }

            // The frontend asks that the pointer returned by GetMappedPointer is from the start of
            // the resource but OpenGL gives us the pointer at offset. Remove the offset.
            mMappedData = static_cast<uint8_t*>(mappedData) - offset;

            return {};
        });
}

void* Buffer::GetMappedPointerImpl() {
    return mMappedData;
}

void Buffer::UnmapImpl(BufferState oldState) {
    auto deviceGuard = GetDevice()->GetGuard();

    // Unmap() might be called in DestroyImpl(), we need to check whether it is the case or not.
    if (!this->TryAddRef()) {
        // TryAddRef() fails mean we are being destroyed, skip further steps.
        mMappedData = nullptr;
        return;
    }

    // adopt ref
    Ref<Buffer> self = AcquireRef(this);

    Device* device = ToBackend(GetDevice());

    if (mShadowCopy.empty()) {
        IgnoreErrors(device->EnqueueGL(ExecutionQueueBase::SubmitMode::Passive,
                                       [self](const OpenGLFunctions& gl) -> MaybeError {
                                           self->DoGLUnmapIfNeeded(gl);
                                           return {};
                                       }));

        return;
    }

    // Transfer shadow copy to the real GL buffer.
    IgnoreErrors(device->EnqueueGL([self](const OpenGLFunctions& gl) -> MaybeError {
        self->mMappedData = nullptr;
        auto data = std::move(self->mShadowCopy);
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, self->GetHandle()));
        DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, 0, self->GetAllocatedSize(), data.data()));
        return {};
    }));

    // Mark the buffer as being used in pending command buffer.
    TrackUsage();
    return;
}

void Buffer::DoGLUnmapIfNeeded(const OpenGLFunctions& gl) {
    if (!mMappedData) {
        return;
    }

    DAWN_GL_TRY_IGNORE_ERRORS(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
    DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));

    mMappedData = nullptr;
}

void Buffer::DestroyImpl() {
    BufferBase::DestroyImpl();

    Device* device = ToBackend(GetDevice());
    Ref<FutureHandle> handle = mHandle;

    IgnoreErrors(device->EnqueueGL(ExecutionQueueBase::SubmitMode::Passive,
                                   [handle](const OpenGLFunctions& gl) -> MaybeError {
                                       GLuint realHandle = handle->Get();
                                       DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &realHandle));
                                       return {};
                                   }));
}

}  // namespace dawn::native::opengl
