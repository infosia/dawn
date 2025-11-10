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
#include "dawn/native/DynamicUploader.h"
#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/UtilsGL.h"

namespace dawn::native::opengl {

namespace {
// A small subclass of Buffer that uses the EXT_buffer_storage extension to create a
// persistently mapped buffer. This avoids a shadow copy of the buffer on the CPU and the
// expensive glBufferSubData calls.
class PersistentMappedBuffer final : public Buffer {
  public:
    PersistentMappedBuffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor)
        : Buffer(device, descriptor) {}

  private:
    MaybeError InitializeImpl(bool isMappedAtCreation) override {
        Device* device = ToBackend(GetDevice());
        Ref<PersistentMappedBuffer> self = this;
        const OpenGLFunctions& gl = device->GetGL();

        GLuint glHandle = 0;
        DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));

        GLbitfield flags = GL_MAP_PERSISTENT_BIT_EXT;
        if (GetUsage() & wgpu::BufferUsage::MapRead) {
            flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        }
        if (GetUsage() & wgpu::BufferUsage::MapWrite || isMappedAtCreation) {
            flags |= GL_MAP_WRITE_BIT;
        }

        // TODO(451928481): GL_MAP_COHERENT_BIT_EXT might cause OOM. Fallback to without
        // that bit in future and explicit Flush + barrier
        DAWN_GL_TRY_ALWAYS_CHECK(
            gl, BufferStorageEXT(GL_ARRAY_BUFFER, GetAllocatedSize(), nullptr, flags));
        mMappedData =
            DAWN_GL_TRY_ALWAYS_CHECK(gl, MapBufferRange(GL_ARRAY_BUFFER, 0, GetAllocatedSize(),
                                                        flags | GL_MAP_FLUSH_EXPLICIT_BIT));

        // The buffers with isMappedAtCreation == true will be initialized in
        // BufferBase::MapAtCreation().
        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
            !isMappedAtCreation) {
            memset(mMappedData, 1, GetAllocatedSize());
            DAWN_TRY(FlushWrites(0, GetAllocatedSize()));
        }

        mBuffer = glHandle;
        return {};
    }

    void DestroyImpl() override {
        if (mBuffer != 0 && mMappedData != nullptr) {
            Device* device = ToBackend(GetDevice());
            const OpenGLFunctions& gl = device->GetGL();
            DAWN_GL_TRY_IGNORE_ERRORS(gl, BindBuffer(GL_ARRAY_BUFFER, mBuffer));
            DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));
        }
        Buffer::DestroyImpl();
    }

    void* GetMappedPointerImpl() override { return mMappedData; }

    bool IsCPUWritableAtCreation() const override { return mMappedData != nullptr; }

    MaybeError MapAtCreationImpl() override {
        DAWN_ASSERT(mMappedData != nullptr);
        return {};
    }

    MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override {
        DAWN_ASSERT(mMappedData != nullptr);
        DAWN_TRY(EnsureDataInitialized());
        return {};
    }

    void UnmapImpl(BufferState oldState) override {
        if (MapMode() != wgpu::MapMode::Write) {
            return;
        }
        IgnoreErrors(FlushWrites(0, GetSize()));
    }

    MaybeError InitializeToZero() override {
        Device* device = ToBackend(GetDevice());

        memset(mMappedData, 0, GetAllocatedSize());

        DAWN_TRY(FlushWrites(0, GetAllocatedSize()));

        device->IncrementLazyClearCountForTesting();

        SetInitialized(true);

        return {};
    }

    MaybeError Write(const OpenGLFunctions& gl,
                     uint64_t offset,
                     const void* data,
                     size_t size) override {
        Device* device = ToBackend(GetDevice());

        return device->GetDynamicUploader()->WithUploadReservation(
            size, /*=alignment=*/1, [&](UploadReservation reservation) -> MaybeError {
                auto srcBuffer =
                    static_cast<PersistentMappedBuffer*>(ToBackend(reservation.buffer).Get());
                memcpy(reservation.mappedPointer, data, size);
                DAWN_TRY(srcBuffer->FlushWrites(reservation.offsetInBuffer, size));

                if (device->IsToggleEnabled(Toggle::GLForceMemoryBarrierForPersistentMap)) {
                    DAWN_GL_TRY(gl, MemoryBarrier(GL_ALL_BARRIER_BITS));
                }

                DAWN_GL_TRY(gl, BindBuffer(GL_PIXEL_PACK_BUFFER, srcBuffer->GetHandle()));
                DAWN_GL_TRY(gl, BindBuffer(GL_PIXEL_UNPACK_BUFFER, this->GetHandle()));
                DAWN_GL_TRY(gl, CopyBufferSubData(GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER,
                                                  reservation.offsetInBuffer, offset, size));

                DAWN_GL_TRY(gl, BindBuffer(GL_PIXEL_PACK_BUFFER, 0));
                DAWN_GL_TRY(gl, BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
                return {};
            });
    }

    MaybeError FlushWrites(size_t offset, size_t size) {
        Device* device = ToBackend(GetDevice());

        const OpenGLFunctions& gl = device->GetGL();

        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
        DAWN_GL_TRY(gl, FlushMappedBufferRange(GL_ARRAY_BUFFER, offset, size));
        return {};
    }
};

}  // namespace

// Buffer

// static
ResultOrError<Ref<Buffer>> Buffer::CreateUninitializedBuffer(
    Device* device,
    const UnpackedPtr<BufferDescriptor>& descriptor) {
    if (((descriptor->usage & kMappableBufferUsages) || descriptor->mappedAtCreation) &&
        !device->IsToggleEnabled(Toggle::GLForceDisablePersistentMap)) {
        return AcquireRef(new PersistentMappedBuffer(device, descriptor));
    }
    return AcquireRef(new Buffer(device, descriptor));
}

// static
ResultOrError<Ref<Buffer>> Buffer::CreateInternalBuffer(Device* device,
                                                        const BufferDescriptor* descriptor,
                                                        bool shouldLazyClear) {
    Ref<Buffer> buffer;
    DAWN_TRY_ASSIGN(buffer, CreateUninitializedBuffer(device, Unpack(descriptor)));
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
    Ref<Buffer> buffer;
    DAWN_TRY_ASSIGN(buffer, CreateUninitializedBuffer(device, descriptor));
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

    DAWN_TRY(InitializeImpl(isMappedAtCreation));

    TrackUsage();
    return {};
}

MaybeError Buffer::InitializeImpl(bool isMappedAtCreation) {
    Device* device = ToBackend(GetDevice());

    const OpenGLFunctions& gl = device->GetGL();
    GLuint glHandle = 0;
    DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));

    // The buffers with mappedAtCreation == true will be initialized in
    // BufferBase::MapAtCreation().
    if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
        !isMappedAtCreation) {
        std::vector<uint8_t> clearValues(mAllocatedSize, 1u);
        DAWN_GL_TRY_ALWAYS_CHECK(
            gl, BufferData(GL_ARRAY_BUFFER, mAllocatedSize, clearValues.data(), GL_STATIC_DRAW));
    } else {
        DAWN_GL_TRY_ALWAYS_CHECK(
            gl, BufferData(GL_ARRAY_BUFFER, mAllocatedSize, nullptr, GL_STATIC_DRAW));
    }
    mBuffer = glHandle;
    return {};
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
    Device* device = ToBackend(GetDevice());

    const auto& gl = device->GetGL();
    const std::vector<uint8_t> clearValues(size, 0u);
    DAWN_TRY(Write(gl, 0, clearValues.data(), size));

    device->IncrementLazyClearCountForTesting();

    TrackUsage();
    SetInitialized(true);
    return {};
}

MaybeError Buffer::Write(const OpenGLFunctions& gl,
                         uint64_t offset,
                         const void* data,
                         size_t size) {
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, mBuffer));
    DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, offset, size, data));

    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    // TODO(enga): All buffers in GL can be mapped. Investigate if mapping them will cause the
    // driver to migrate it to shared memory.
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    const OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, mBuffer));
    mMappedData = DAWN_GL_TRY_ALWAYS_CHECK(
        gl, MapBufferRange(GL_ARRAY_BUFFER, 0, GetSize(), GL_MAP_WRITE_BIT));
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    const OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();

    // It is an error to map an empty range in OpenGL. We always have at least a 4-byte buffer
    // so we extend the range to be 4 bytes.
    if (size == 0) {
        if (offset != 0) {
            offset -= 4;
        }
        size = 4;
    }

    DAWN_TRY(EnsureDataInitialized());

    // This does GPU->CPU synchronization, we could require a high
    // version of OpenGL that would let us map the buffer unsynchronized.
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, mBuffer));
    void* mappedData = nullptr;
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
}

void Buffer::FinalizeMapImpl(BufferState newState) {}

void* Buffer::GetMappedPointerImpl() {
    // The mapping offset has already been removed.
    return mMappedData;
}

void Buffer::UnmapImpl(BufferState oldState) {
    const OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();

    DAWN_GL_TRY_IGNORE_ERRORS(gl, BindBuffer(GL_ARRAY_BUFFER, mBuffer));
    DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));
    mMappedData = nullptr;
}

void Buffer::DestroyImpl() {
    const OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();

    BufferBase::DestroyImpl();
    DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &mBuffer));
    mBuffer = 0;
}

}  // namespace dawn::native::opengl
