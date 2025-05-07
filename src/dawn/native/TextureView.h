// Copyright 2025 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_TEXTURE_VIEW_H_
#define SRC_DAWN_NATIVE_TEXTURE_VIEW_H_

#include <string>
#include <vector>

#include "dawn/common/WeakRef.h"
#include "dawn/common/ityp_array.h"
#include "dawn/common/ityp_bitset.h"
#include "dawn/native/Error.h"
#include "dawn/native/Format.h"
#include "dawn/native/Forward.h"
#include "dawn/native/ObjectBase.h"
#include "dawn/native/SharedTextureMemory.h"
#include "dawn/native/Subresource.h"
#include "partition_alloc/pointers/raw_ref.h"

#include "dawn/native/dawn_platform.h"

namespace dawn::native {

class TextureBase;

MaybeError ValidateTextureViewDescriptor(const DeviceBase* device,
                                         const TextureBase* texture,
                                         const UnpackedPtr<TextureViewDescriptor>& descriptor);
ResultOrError<TextureViewDescriptor> GetTextureViewDescriptorWithDefaults(
    const TextureBase* texture,
    const TextureViewDescriptor* descriptor);
bool IsTextureViewDimensionCompatibleWithTextureDimension(
    wgpu::TextureViewDimension textureViewDimension,
    wgpu::TextureDimension textureDimension);

class TextureViewInternalBase : public ApiObjectBase {
  public:
    TextureViewInternalBase(TextureBase* texture,
                            const UnpackedPtr<TextureViewDescriptor>& descriptor,
                            ApiObjectBase::UntrackedByDeviceTag tag);
    TextureViewInternalBase(TextureBase* texture,
                            const UnpackedPtr<TextureViewDescriptor>& descriptor);
    ~TextureViewInternalBase() override;

    ObjectType GetType() const override;

    const WeakRef<TextureBase>& GetTexture() const;

    // Create a TextureViewBase that wraps to this TextureViewInternalBase
    Ref<TextureViewBase> CreateTextureView();

    Aspect GetAspects() const;
    const Format& GetFormat() const;
    wgpu::TextureViewDimension GetDimension() const;
    uint32_t GetBaseMipLevel() const;
    uint32_t GetLevelCount() const;
    uint32_t GetBaseArrayLayer() const;
    uint32_t GetLayerCount() const;

    const SubresourceRange& GetSubresourceRange() const;
    Extent3D GetSingleSubresourceVirtualSize() const;

    // |GetUsage| returns the usage with which the texture view was created using the base WebGPU
    // API. The dawn-internal-usages extension may add additional usages. |GetInternalUsage|
    // returns the union of base usage and the usages added by the extension.
    wgpu::TextureUsage GetUsage() const;
    wgpu::TextureUsage GetInternalUsage() const;

    virtual bool IsYCbCr() const;
    // Valid to call only if `IsYCbCr()` is true.
    virtual YCbCrVkDescriptor GetYCbCrVkDescriptor() const;

  protected:
    void DestroyImpl() override;

  private:
    ApiObjectList* GetObjectTrackingList() override;

    // A pointer to the parent Texture that's weak to prevent potential ref cycles.
    WeakRef<TextureBase> mTexture;

    const raw_ref<const Format> mFormat;
    wgpu::TextureViewDimension mDimension;
    SubresourceRange mRange;
    const wgpu::TextureUsage mUsage = wgpu::TextureUsage::None;
    const wgpu::TextureUsage mInternalUsage = wgpu::TextureUsage::None;
};

class TextureViewBase : public ApiObjectBase {
  public:
    TextureViewBase(TextureBase* texture, Ref<TextureViewInternalBase> internal, StringView label);
    ~TextureViewBase() override;

    static Ref<TextureViewBase> MakeError(DeviceBase* device, StringView label = nullptr);

    ObjectType GetType() const override;
    void FormatLabel(absl::FormatSink* s) const override;

    const TextureBase* GetTexture() const;
    TextureBase* GetTexture();

    const TextureViewInternalBase* GetInternal() const;
    TextureViewInternalBase* GetInternal();

    Aspect GetAspects() const { return mInternal->GetAspects(); }
    const Format& GetFormat() const { return mInternal->GetFormat(); }
    wgpu::TextureViewDimension GetDimension() const { return mInternal->GetDimension(); }
    uint32_t GetBaseMipLevel() const { return mInternal->GetBaseMipLevel(); }
    uint32_t GetLevelCount() const { return mInternal->GetLevelCount(); }
    uint32_t GetBaseArrayLayer() const { return mInternal->GetBaseArrayLayer(); }
    uint32_t GetLayerCount() const { return mInternal->GetLayerCount(); }

    const SubresourceRange& GetSubresourceRange() const { return mInternal->GetSubresourceRange(); }
    Extent3D GetSingleSubresourceVirtualSize() const {
        return mInternal->GetSingleSubresourceVirtualSize();
    }

    wgpu::TextureUsage GetUsage() const { return mInternal->GetUsage(); }
    wgpu::TextureUsage GetInternalUsage() const { return mInternal->GetInternalUsage(); }

    bool IsYCbCr() const { return mInternal->IsYCbCr(); }
    // Valid to call only if `IsYCbCr()` is true.
    YCbCrVkDescriptor GetYCbCrVkDescriptor() const { return mInternal->GetYCbCrVkDescriptor(); }

  protected:
    void DestroyImpl() override;

  private:
    TextureViewBase(DeviceBase* device, ObjectBase::ErrorTag tag, StringView label);

    ApiObjectList* GetObjectTrackingList() override;

    Ref<TextureBase> mTexture;
    Ref<TextureViewInternalBase> mInternal;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_TEXTURE_VIEW_H_
