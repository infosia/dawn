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

#ifndef SRC_DAWN_NATIVE_SWAPCHAIN_H_
#define SRC_DAWN_NATIVE_SWAPCHAIN_H_

#include <vector>

#include "dawn/native/Error.h"
#include "dawn/native/Format.h"
#include "dawn/native/Forward.h"
#include "dawn/native/ObjectBase.h"
#include "dawn/native/dawn_platform.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native {

TextureDescriptor GetSwapChainBaseTextureDescriptor(SwapChainBase* swapChain);

struct SwapChainTextureInfo {
    Ref<TextureBase> texture;
    wgpu::Bool suboptimal;
    wgpu::SurfaceGetCurrentTextureStatus status;
};

class SwapChainBase : public RefCounted {
  public:
    SwapChainBase(DeviceBase* device, Surface* surface, const SurfaceConfiguration* config);
    ~SwapChainBase() override;

    DeviceBase* GetDevice() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    wgpu::TextureFormat GetFormat() const;
    const std::vector<wgpu::TextureFormat>& GetViewFormats() const;
    wgpu::TextureUsage GetUsage() const;
    wgpu::PresentMode GetPresentMode() const;
    wgpu::CompositeAlphaMode GetAlphaMode() const;
    Surface* GetSurface() const;
    wgpu::BackendType GetBackendType() const;

    // Get a format set from mViewFormats (equivalent information, but easier to validate the
    // current texture)
    FormatSet ComputeViewFormatSet() const;

    // The returned texture must match the swapchain descriptor exactly.
    virtual ResultOrError<SwapChainTextureInfo> GetCurrentTexture() = 0;
    virtual MaybeError Present() = 0;

  private:
    Ref<DeviceBase> mDevice;

    uint32_t mWidth;
    uint32_t mHeight;
    wgpu::TextureFormat mFormat;
    wgpu::TextureUsage mUsage;
    wgpu::PresentMode mPresentMode;
    // This is not stored as a FormatSet so that it can hold the data pointed to by the
    // descriptor returned by GetSwapChainBaseTextureDescriptor():
    std::vector<wgpu::TextureFormat> mViewFormats;
    wgpu::CompositeAlphaMode mAlphaMode;

    // Swapchains are owned by their parent wgpu::Surface so it is safe to store a raw_ptr.
    raw_ptr<Surface> mSurface = nullptr;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_SWAPCHAIN_H_
