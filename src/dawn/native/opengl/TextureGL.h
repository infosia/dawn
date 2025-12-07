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

#ifndef SRC_DAWN_NATIVE_OPENGL_TEXTUREGL_H_
#define SRC_DAWN_NATIVE_OPENGL_TEXTUREGL_H_

#include <functional>

#include "dawn/native/Texture.h"

#include "dawn/native/opengl/FutureHandle.h"
#include "dawn/native/opengl/opengl_platform.h"

namespace dawn::native::opengl {

class Device;
struct GLFormat;
struct OpenGLFunctions;
class SharedTextureMemory;

enum class OwnsHandle : uint8_t {
    Yes,
    No,
};

class Texture final : public TextureBase {
  public:
    static ResultOrError<Ref<Texture>> Create(Device* device,
                                              const UnpackedPtr<TextureDescriptor>& descriptor);
    static ResultOrError<Ref<Texture>> CreateFromExistingHandle(
        Device* device,
        const UnpackedPtr<TextureDescriptor>& descriptor,
        GLuint handle);
    static ResultOrError<Ref<Texture>> CreateFromSharedTextureMemory(
        SharedTextureMemory* memory,
        const UnpackedPtr<TextureDescriptor>& descriptor);

    GLuint GetHandle() const;
    Ref<FutureHandle> GetHandleRef() const;
    GLenum GetGLTarget() const;
    const GLFormat& GetGLFormat() const;

    MaybeError EnsureSubresourceContentInitialized(const OpenGLFunctions& gl,
                                                   const SubresourceRange& range);

    MaybeError SynchronizeTextureBeforeUse();

  private:
    Texture(Device* device,
            const UnpackedPtr<TextureDescriptor>& descriptor,
            OwnsHandle ownsHandle);
    ~Texture() override;

    using HandleGenerator = std::function<ResultOrError<GLuint>(const OpenGLFunctions&)>;
    MaybeError InitializeHandle(const HandleGenerator& handleGenerator);

    void DestroyImpl() override;
    MaybeError ClearTexture(const OpenGLFunctions& gl,
                            const SubresourceRange& range,
                            TextureBase::ClearValue clearValue);

    Ref<FutureHandle> mHandle;
    OwnsHandle mOwnsHandle = OwnsHandle::No;
    GLenum mTarget;
};

class TextureView final : public TextureViewBase {
  public:
    static ResultOrError<Ref<TextureView>> Create(
        TextureBase* texture,
        const UnpackedPtr<TextureViewDescriptor>& descriptor);

    GLuint GetHandle() const;
    GLenum GetGLTarget() const;
    MaybeError BindToFramebuffer(const OpenGLFunctions& gl,
                                 GLenum target,
                                 GLenum attachment,
                                 GLuint depthLayer = 0);

  private:
    TextureView(TextureBase* texture, const UnpackedPtr<TextureViewDescriptor>& descriptor);

    ~TextureView() override;

    MaybeError Initialize(const UnpackedPtr<TextureViewDescriptor>& descriptor);
    void DestroyImpl() override;
    GLenum GetInternalFormat() const;

    Ref<FutureHandle> mHandle;
    GLenum mTarget;
    OwnsHandle mOwnsHandle = OwnsHandle::No;
};

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_TEXTUREGL_H_
