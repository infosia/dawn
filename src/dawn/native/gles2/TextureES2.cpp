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

#include "dawn/native/gles2/TextureES2.h"

#include "dawn/native/gles2/DeviceES2.h"
#include "dawn/native/gles2/Forward.h"

namespace dawn::native::gles2 {

Texture::Texture(Device* device, const UnpackedPtr<TextureDescriptor>& descriptor)
    : TextureBase(device, descriptor) {
    const gl::OpenGLFunctions& gl = device->GetGL();
    gl.GenTextures(1, &mHandle);
    gl.CheckErrors("GenTextures");
    gl.BindTexture(GL_TEXTURE_2D, mHandle);
    gl.CheckErrors("BindTexture");

    // TODO format options are much more limited in es2 than es3
    // TODO not sure what type (byte packing) to use
    // TODO clear texture
    const Extent3D& size = GetBaseSize();
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.CheckErrors("TexImage2D");
}

void Texture::DestroyImpl() {
    TextureBase::DestroyImpl();

    const gl::OpenGLFunctions& gl = ToBackend(GetDevice())->GetGL();
    gl.DeleteTextures(1, &mHandle);
    gl.CheckErrors("DeleteTextures");
    mHandle = 0;
}

GLuint Texture::GetHandle() const {
    return mHandle;
}

TextureView::TextureView(TextureBase* texture, const UnpackedPtr<TextureViewDescriptor>& descriptor)
    : TextureViewBase(texture, descriptor) {
}

GLuint TextureView::GetHandle() const {
    return ToBackend(GetTexture())->GetHandle();
}

}  // namespace dawn::native::gles2
