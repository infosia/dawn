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

  private:
    Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor);
    ~Buffer() override;

    void DestroyImpl() override;

    MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
    MaybeError FinalizeMapImpl(BufferState newState) override;
    void UnmapImpl(BufferState oldState) override;
    bool IsCPUWritableAtCreation() const override;
    MaybeError MapAtCreationImpl() override;
    void* GetMappedPointerImpl() override;

    MaybeError Initialize(bool shouldLazyClear, bool isMappedAtCreation);
    MaybeError InitializeGLBuffer(bool isMappedAtCreation);
    MaybeError InitializeToZero();
    void DoGLUnmapIfNeeded(const OpenGLFunctions& gl);

    // The mHandle is a future GLuint that will be updated when the buffer is created on the GL
    // thread. This is necessary because the Buffer object can be created on one thread, but the
    // actual GL handle is generated asynchronously on the GL thread.
    //
    // If we were to store a raw GLuint, we could run into a race condition during destruction.
    // For example, if DestroyImpl() is called before the GL handle has been created, a lambda
    // capturing the GLuint would capture an invalid handle (e.g., 0).
    //
    // Incorrect approach example:
    //
    //   // In DestroyImpl() on the main thread:
    //   GLuint handleToDelete = mHandle; // mHandle might be 0 if creation hasn't happened yet.
    //   device->EnqueueGL([handleToDelete](...) {
    //       glDeleteBuffers(1, &handleToDelete); // Deletes buffer 0, leaking the real one.
    //   });
    //
    // By using a Ref<FutureHandle>, we capture a reference to the handle container. The command
    // queue ensures that the destruction task runs after the creation task. By the time the
    // destruction lambda executes, the FutureHandle is guaranteed to have been set with the
    // correct GLuint.
    Ref<FutureHandle> mHandle;
    // Shadow copy is used for MapAtCreation.
    std::vector<uint8_t> mShadowCopy;
    raw_ptr<void> mMappedData = nullptr;
};

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_BUFFERGL_H_
