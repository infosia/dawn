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
    DAWN_TRY(buffer->Initialize(/*shouldLazyClear=*/true, descriptor->mappedAtCreation));
    return buffer;
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
}

MaybeError Buffer::Initialize(bool shouldLazyClear, bool isMappedAtCreation) {
    if (!shouldLazyClear) {
        SetInitialized(true);
    }

    if (isMappedAtCreation) {
        mShadowCopy.resize(mAllocatedSize);
    }

    DAWN_TRY(InitializeGLBuffer({}));

    TrackUsage();

    return {};
}

MaybeError Buffer::InitializeGLBuffer(bool isMappedAtCreation) {
    Device* device = ToBackend(GetDevice());
    Ref<Buffer> self = this;
    return device->EnqueueGL(
        [self, device, isMappedAtCreation](const OpenGLFunctions& gl) mutable -> MaybeError {
            GLuint glHandle = 0;
            DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
            DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));
            self->mHandle->Set(glHandle);

            // The buffers with mappedAtCreation == true will be initialized in
            // BufferBase::MapAtCreation().
            if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
                !isMappedAtCreation) {
                std::vector<uint8_t> clearValues(self->mAllocatedSize, 1u);
                DAWN_GL_TRY_ALWAYS_CHECK(gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize,
                                                        clearValues.data(), GL_STATIC_DRAW));
            } else {
                DAWN_GL_TRY_ALWAYS_CHECK(
                    gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize, nullptr, GL_STATIC_DRAW));
            }

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
    // We use shadow copy for mapAtCreation, so CPU access is always true.
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    // We will need shadow copy to implement mapAtCreation.
    DAWN_ASSERT(!mShadowCopy.empty());
    mMappedData = mShadowCopy.data();
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    DAWN_TRY(EnsureDataInitialized());

    // The real map will be handled in FinalizeMapImpl() when buffer is free of any GPU use.
    return {};
}

MaybeError Buffer::FinalizeMapImpl(BufferState newState) {
    if (newState == BufferState::MappedAtCreation) {
        // mapAtCreation uses shadow copy so nothing to do here.
        return {};
    }

    DAWN_ASSERT(newState == BufferState::Mapped);
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
    // completed, it's safe to use ExecuteGL() to trigger GL command immediately.
    // TODO(451928481): We assume this is called on a thread that can make GL context current, e.g.
    // playback thread in Skia.
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
    // Unmap() might be called in DestroyImpl(), we need to check whether it is the case or not.
    if (!this->TryAddRef()) {
        // TryAddRef() fails mean we are being destroyed, skip further steps.
        return;
    }

    // adopt ref
    Ref<Buffer> self = AcquireRef(this);

    Device* device = ToBackend(GetDevice());

    if (GetState() == BufferState::Mapped) {
        IgnoreErrors(device->EnqueueGL(ExecutionQueueBase::SubmitMode::Passive,
                                       [self](const OpenGLFunctions& gl) -> MaybeError {
                                           self->DoGLUnmapIfNeeded(gl);
                                           return {};
                                       }));

        return;
    }

    DAWN_ASSERT(GetState() == BufferState::MappedAtCreation);
    DAWN_ASSERT(!mShadowCopy.empty());
    // Transfer shadow copy to the real GL buffer.
    IgnoreErrors(device->EnqueueGL([self](const OpenGLFunctions& gl) -> MaybeError {
        self->mMappedData = nullptr;
        auto data = std::move(self->mShadowCopy);
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, self->GetHandle()));
        DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, 0, self->GetAllocatedSize(), data.data()));
        return {};
    }));
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
                                       GLuint realHandle = handle->GetOrDefault();
                                       DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &realHandle));
                                       return {};
                                   }));
}

}  // namespace dawn::native::opengl
