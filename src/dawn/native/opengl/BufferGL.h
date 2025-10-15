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

#ifndef SRC_DAWN_NATIVE_OPENGL_BUFFERGL_H_
#define SRC_DAWN_NATIVE_OPENGL_BUFFERGL_H_

#include <vector>

#include "dawn/native/Buffer.h"
#include "partition_alloc/pointers/raw_ptr.h"

#include "dawn/native/opengl/FutureHandle.h"
#include "dawn/native/opengl/opengl_platform.h"

namespace dawn::native::opengl {

struct OpenGLFunctions;
class Device;

class Buffer final : public BufferBase {
  public:
    static ResultOrError<Ref<Buffer>> CreateInternalBuffer(Device* device,
                                                           const BufferDescriptor* descriptor,
                                                           bool shouldLazyClear);
    static ResultOrError<Ref<Buffer>> Create(Device* device,
                                             const UnpackedPtr<BufferDescriptor>& descriptor);

    const FutureHandle& GetHandle() const;

    MaybeError EnsureDataInitialized(bool* outDidDataInitialization = nullptr);
    MaybeError EnsureDataInitializedAsDestination(uint64_t offset,
                                                  uint64_t size,
                                                  bool* outDidDataInitialization = nullptr);
    MaybeError EnsureDataInitializedAsDestination(const CopyTextureToBufferCmd* copy,
                                                  bool* outDidDataInitialization = nullptr);

    void TrackUsage() { MarkUsedInPendingCommands(); }

    MaybeError Write(uint64_t offset, const void* data, size_t size);

    // Notify this object that a copy to this buffer has been completed.
    // This is typically used to sync the shadow copy.
    MaybeError OnCopiedTo();

    // Return whether to track the completion of a copy command if this buffer is used as a
    // destination. This will be true if the buffer has a shadow copy.
    bool NeedsCopyDstTracking() const;

  private:
    Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor);
    ~Buffer() override;

    MaybeError Initialize(bool shouldLazyClear, bool isMappedAtCreation);

    MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
    void FinalizeMapImpl() override;
    void UnmapImpl() override;
    void DestroyImpl() override;
    bool IsCPUWritableAtCreation() const override;
    MaybeError MapAtCreationImpl() override;
    void* GetMappedPointerImpl() override;

    MaybeError InitializeToZero();

    MaybeError WriteRealBuffer(const OpenGLFunctions& gl,
                               uint64_t offset,
                               const void* data,
                               size_t size);

    Ref<FutureHandle> mHandle;

    // TODO(451928481): Use persistent map extension GL_EXT_buffer_storage to avoid shadow
    // copies.
    std::vector<uint8_t> mShadowCopy;
};

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_BUFFERGL_H_
