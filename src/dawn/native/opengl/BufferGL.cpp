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

    DAWN_TRY(InitializeImpl(isMappedAtCreation));

    TrackUsage();
    return {};
}

MaybeError Buffer::InitializeImpl(bool isMappedAtCreation) {
    Device* device = ToBackend(GetDevice());

    if ((GetUsage() & kMappableBufferUsages) || isMappedAtCreation) {
        mShadowCopy.resize(mAllocatedSize);
    }

    auto gl = device->GetGL();

    DAWN_GL_TRY(gl, GenBuffers(1, &mHandle));
    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, mHandle));

    // The buffers with mappedAtCreation == true will be initialized in
    // BufferBase::MapAtCreation().
    if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
        !isMappedAtCreation) {
        std::vector<uint8_t> clearValues(mAllocatedSize, 1u);
        if (!mShadowCopy.empty()) {
            memcpy(mShadowCopy.data(), clearValues.data(), mAllocatedSize);
        }
        DAWN_GL_TRY_ALWAYS_CHECK(
            gl, BufferData(GL_ARRAY_BUFFER, mAllocatedSize, clearValues.data(), GL_STATIC_DRAW));
    } else {
        DAWN_GL_TRY_ALWAYS_CHECK(
            gl, BufferData(GL_ARRAY_BUFFER, mAllocatedSize, nullptr, GL_STATIC_DRAW));
    }
    return {};
}

Buffer::~Buffer() = default;

GLuint Buffer::GetHandle() const {
    return mHandle;
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

    ToBackend(GetDevice())->IncrementLazyClearCountForTesting();

    TrackUsage();
    SetInitialized(true);
    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    // We use shadow copy for mappable buffers or mapAtCreation, so CPU access is always true.
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    // We will need shadow copy to implement mapAtCreation.
    DAWN_ASSERT(!mShadowCopy.empty());
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    DAWN_TRY(EnsureDataInitialized());
    // Do nothing, shadow copy is already mapped.
    DAWN_ASSERT(!mShadowCopy.empty());
    return {};
}

MaybeError Buffer::FinalizeMapImpl(BufferState newState) { return {}; }

void* Buffer::GetMappedPointerImpl() {
    // The mapping offset has already been removed.
    return mShadowCopy.data();
}

void Buffer::UnmapImpl(BufferState oldState) {
    // Skip flushing the shadow copy if the buffer is neither mapped for write nor mapped at
    // creation.
    if (!(GetUsage() & wgpu::BufferUsage::MapWrite) &&
        GetState() != BufferState::MappedAtCreation) {
        return;
    }

    // Transfer shadow copy's data to the real buffer.
    // If shadow copy is only needed for mapAtCreation, then delete it after the data is
    // transferred.
    const bool noNeedShadowCopyAnymore = !(GetUsage() & kMappableBufferUsages);
    Device* device = ToBackend(GetDevice());
    auto gl = device->GetGL();
    IgnoreErrors(WriteRealBuffer(gl, 0, mShadowCopy.data(), mShadowCopy.size()));
    if (noNeedShadowCopyAnymore) {
        mShadowCopy.clear();
        mShadowCopy.shrink_to_fit();
    }
}

MaybeError Buffer::Write(uint64_t offset, const void* data, size_t size) {
    Device* device = ToBackend(GetDevice());

    std::vector<uint8_t> copyData(size);
    memcpy(copyData.data(), data, size);

    if (!mShadowCopy.empty()) {
        memcpy(mShadowCopy.data() + offset, data, size);
    }

    auto gl = device->GetGL();

    return WriteRealBuffer(gl, offset, copyData.data(), copyData.size());
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
    auto gl = device->GetGL();

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
    BufferBase::DestroyImpl();

    Device* device = ToBackend(GetDevice());

    auto gl = device->GetGL();
    DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &mHandle));
}

}  // namespace dawn::native::opengl
