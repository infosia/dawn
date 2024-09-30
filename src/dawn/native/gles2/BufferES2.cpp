// Copyright 2024 The Dawn & Tint Authors
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

#include "dawn/native/gles2/BufferES2.h"

#include "dawn/native/gl/OpenGLFunctions.h"
#include "dawn/native/gles2/DeviceES2.h"
#include "dawn/native/gles2/Forward.h"

namespace dawn::native::gles2 {

Buffer::Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor)
    : BufferBase(device, descriptor) {
    // TODO see history of BufferGL.cpp for context about how zero-length allocations are a problem
    mAllocatedSize = GetSize();
    mBufferData.resize(mAllocatedSize);

    // Create uninitialized buffer.
    const gl::OpenGLFunctions& gl = device->GetGL();
    gl.GenBuffers(1, &mBuffer);
    gl.CheckErrors("GenBuffers");
    gl.BindBuffer(GL_ARRAY_BUFFER, mBuffer);
    gl.CheckErrors("BindBuffer");
    gl.BufferData(GL_ARRAY_BUFFER, mAllocatedSize, nullptr, GL_STATIC_DRAW);
    gl.CheckErrors("BufferData");
}

MaybeError Buffer::MapSync(size_t offset, size_t size) {
    if (offset + size > mBufferData.size()) {
        return DAWN_INTERNAL_ERROR("Invalid buffer mapping");
    }
    mMappedOffset = offset;
    mMappedSize = size;
    return {};
}

void Buffer::UnmapSync() {
    const gl::OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();
    gl.BindBuffer(GL_ARRAY_BUFFER, mBuffer);
    gl.CheckErrors("BindBuffer");
    gl.BufferSubData(GL_ARRAY_BUFFER, mMappedOffset, mMappedSize, mBufferData.data() + mMappedOffset);
    gl.CheckErrors("BufferSubData");
}

void* Buffer::GetOffsetPointer() {
    return mBufferData.data() + mMappedOffset;
}

void* Buffer::GetMappedPointer() {
    return mBufferData.data();
}

MaybeError Buffer::MapAtCreationImpl() {
    mMappedOffset = 0;
    mMappedSize = mAllocatedSize;
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    // TODO how will this work if this is supposed to be async? multithreading concern here?
    return MapSync(offset, size);
}

void Buffer::UnmapImpl() {
    UnmapSync();
}

void Buffer::DestroyImpl() {
    BufferBase::DestroyImpl();

    const gl::OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();
    gl.DeleteBuffers(1, &mBuffer);
    gl.CheckErrors("DeleteBuffers");
    mBuffer = 0;
}

bool Buffer::IsCPUWritableAtCreation() const {
    return true;
}

}  // namespace dawn::native::gles2
