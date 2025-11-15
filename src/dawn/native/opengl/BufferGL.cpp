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
    : BufferBase(device, descriptor) {
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

    TrackUsage();

    Ref<Buffer> self = this;
    Device* device = ToBackend(GetDevice());
    // TODO(451928481): We cannot defer GL buffer's creation yet because it would prevent error
    // scope from checking OOM error immediately. Consider a new feature to allow defer GL
    // resources' creation. That would avoid the overhead of constant context switches.
    return device->ExecuteGLWork(
        [self, device, isMappedAtCreation](const OpenGLFunctions& gl) mutable -> MaybeError {
            DAWN_GL_TRY(gl, GenBuffers(1, &self->mBuffer));
            DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, self->mBuffer));

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

GLuint Buffer::GetHandle() const {
    return mBuffer;
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

    Ref<Buffer> self = this;
    Device* device = ToBackend(GetDevice());
    auto glTask = [self, size, clearValues = std::move(clearValues)](
                      const OpenGLFunctions& gl) mutable -> MaybeError {
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, self->GetHandle()));
        DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, 0, size, clearValues.data()));
        return {};
    };

    DAWN_TRY(device->AddGLWork(glTask));

    device->IncrementLazyClearCountForTesting();

    TrackUsage();
    SetInitialized(true);
    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    // TODO(enga): All buffers in GL can be mapped. Investigate if mapping them will cause the
    // driver to migrate it to shared memory.
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    // Will be handled in FinalizeMapImpl()
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    // Will be handled in FinalizeMapImpl()
    DAWN_TRY(EnsureDataInitialized());
    return {};
}

void Buffer::FinalizeMapImpl(BufferState newState) {
    if (newState != BufferState::Mapped && newState != BufferState::MappedAtCreation) {
        return;
    }

    Device* device = ToBackend(GetDevice());
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

    // TODO(451928481): Consider using shadow copy or persistent map to avoid flushing GL work and
    // blocking the Submit thread.
    auto result = device->ExecuteGLWork(
        [this, &mappedData, size, offset, mode](const OpenGLFunctions& gl) mutable -> MaybeError {
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

            return {};
        });

    if (device->ConsumedError(std::move(result))) {
        // TODO(451928481): propagate error to caller.
        mMappedData = nullptr;
        return;
    }

    // The frontend asks that the pointer returned by GetMappedPointer is from the start of
    // the resource but OpenGL gives us the pointer at offset. Remove the offset.
    mMappedData = static_cast<uint8_t*>(mappedData) - offset;
}

void* Buffer::GetMappedPointerImpl() {
    // The mapping offset has already been removed.
    return mMappedData;
}

void Buffer::UnmapImpl(BufferState oldState) {
    mMappedData = nullptr;

    Ref<Buffer> self = this;
    Device* device = ToBackend(GetDevice());
    IgnoreErrors(device->AddGLWork([self](const OpenGLFunctions& gl) mutable -> MaybeError {
        DAWN_GL_TRY_IGNORE_ERRORS(gl, BindBuffer(GL_ARRAY_BUFFER, self->GetHandle()));
        DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));

        return {};
    }));
}

void Buffer::DestroyImpl() {
    BufferBase::DestroyImpl();

    Device* device = ToBackend(GetDevice());

    IgnoreErrors(device->AddGLWork([handle = mBuffer](const OpenGLFunctions& gl) -> MaybeError {
        DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &handle));
        return {};
    }));
}

}  // namespace dawn::native::opengl
