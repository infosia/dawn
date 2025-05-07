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

#include "dawn/native/TextureView.h"

#include <algorithm>
#include <utility>

#include "absl/strings/str_format.h"
#include "dawn/common/Assert.h"
#include "dawn/common/Constants.h"
#include "dawn/common/Math.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/BlitBufferToTexture.h"
#include "dawn/native/BlitTextureToBuffer.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandValidation.h"
#include "dawn/native/Device.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/ObjectContentHasher.h"
#include "dawn/native/ObjectType_autogen.h"
#include "dawn/native/PassResourceUsage.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/SharedTextureMemory.h"
#include "dawn/native/ValidationUtils_autogen.h"

namespace dawn::native {

namespace {

MaybeError ValidateCanViewTextureAs(const DeviceBase* device,
                                    const TextureBase* texture,
                                    const Format& viewFormat,
                                    wgpu::TextureAspect aspect) {
    const Format& format = texture->GetFormat();

    if (aspect != wgpu::TextureAspect::All) {
        wgpu::TextureFormat aspectFormat = format.GetAspectInfo(aspect).format;
        if (viewFormat.format == aspectFormat) {
            return {};
        } else {
            return DAWN_VALIDATION_ERROR(
                "The view format (%s) is not compatible with %s of %s (%s).", viewFormat.format,
                aspect, format.format, aspectFormat);
        }
    }

    if (format.format == viewFormat.format) {
        return {};
    }

    const FormatSet& compatibleViewFormats = texture->GetViewFormats();
    if (compatibleViewFormats[viewFormat]) {
        // Validation of this list is done on texture creation, so we don't need to
        // handle the case where a format is in the list, but not compatible.
        return {};
    }

    // |viewFormat| is not in the list. Check compatibility to generate an error message
    // depending on whether it could be compatible, but needs to be explicitly listed,
    // or it could never be compatible.
    if (!format.ViewCompatibleWith(viewFormat)) {
        // The view format isn't compatible with the format at all. Return an error
        // that indicates this, in addition to reporting that it's missing from the
        // list.
        return DAWN_VALIDATION_ERROR(
            "The texture view format (%s) is not compatible with the "
            "texture format (%s)."
            "The formats must be compatible, and the view format "
            "must be passed in the list of view formats on texture creation.",
            viewFormat.format, format.format);
    }

    // The view format is compatible, but not in the list.
    return DAWN_VALIDATION_ERROR(
        "%s was not created with the texture view format (%s) "
        "in the list of compatible view formats.",
        texture, viewFormat.format);
}

bool IsArrayLayerValidForTextureViewDimension(wgpu::TextureViewDimension textureViewDimension,
                                              uint32_t textureViewArrayLayer) {
    switch (textureViewDimension) {
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e3D:
            return textureViewArrayLayer == 1u;
        case wgpu::TextureViewDimension::e2DArray:
            return true;
        case wgpu::TextureViewDimension::Cube:
            return textureViewArrayLayer == 6u;
        case wgpu::TextureViewDimension::CubeArray:
            return textureViewArrayLayer % 6 == 0;
        case wgpu::TextureViewDimension::e1D:
            return textureViewArrayLayer == 1u;

        case wgpu::TextureViewDimension::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MaybeError ValidateTextureViewDimensionCompatibility(
    const DeviceBase* device,
    const TextureBase* texture,
    const UnpackedPtr<TextureViewDescriptor>& descriptor) {
    DAWN_INVALID_IF(!IsArrayLayerValidForTextureViewDimension(descriptor->dimension,
                                                              descriptor->arrayLayerCount),
                    "The dimension (%s) of the texture view is not compatible with the layer count "
                    "(%u) of %s.",
                    descriptor->dimension, descriptor->arrayLayerCount, texture);

    DAWN_INVALID_IF(
        !IsTextureViewDimensionCompatibleWithTextureDimension(descriptor->dimension,
                                                              texture->GetDimension()),
        "The dimension (%s) of the texture view is not compatible with the dimension (%s) "
        "of %s.",
        descriptor->dimension, texture->GetDimension(), texture);

    DAWN_INVALID_IF(
        texture->GetSampleCount() > 1 && descriptor->dimension != wgpu::TextureViewDimension::e2D,
        "The dimension (%s) of the multisampled texture view is not %s.", descriptor->dimension,
        wgpu::TextureViewDimension::e2D);

    switch (descriptor->dimension) {
        case wgpu::TextureViewDimension::Cube:
        case wgpu::TextureViewDimension::CubeArray:
            DAWN_INVALID_IF(
                texture->GetSize(descriptor->aspect).width !=
                    texture->GetSize(descriptor->aspect).height,
                "A %s texture view is not compatible with %s because the texture's width "
                "(%u) and height (%u) are not equal.",
                descriptor->dimension, texture, texture->GetSize(descriptor->aspect).width,
                texture->GetSize(descriptor->aspect).height);
            DAWN_INVALID_IF(descriptor->dimension == wgpu::TextureViewDimension::CubeArray &&
                                device->IsCompatibilityMode(),
                            "A %s texture view for %s is not supported in compatibility mode",
                            descriptor->dimension, texture);
            break;
        case wgpu::TextureViewDimension::e1D:
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e2DArray:
        case wgpu::TextureViewDimension::e3D:
            break;

        case wgpu::TextureViewDimension::Undefined:
            DAWN_UNREACHABLE();
    }

    return {};
}

MaybeError ValidateTextureUsage(const DeviceBase* device,
                                wgpu::TextureDimension textureDimension,
                                wgpu::TextureUsage usage,
                                const Format* format,
                                std::optional<wgpu::TextureUsage> allowedSharedTextureMemoryUsage) {
    DAWN_TRY(dawn::native::ValidateTextureUsage(usage));

    DAWN_INVALID_IF(usage == wgpu::TextureUsage::None, "The texture usage must not be 0.");

    constexpr wgpu::TextureUsage kValidCompressedUsages = wgpu::TextureUsage::TextureBinding |
                                                          wgpu::TextureUsage::CopySrc |
                                                          wgpu::TextureUsage::CopyDst;
    DAWN_INVALID_IF(
        format->isCompressed && !IsSubset(usage, kValidCompressedUsages),
        "The texture usage (%s) is incompatible with the compressed texture format (%s).", usage,
        format->format);

    DAWN_INVALID_IF(
        !format->isRenderable && (usage & wgpu::TextureUsage::RenderAttachment),
        "The texture usage (%s) includes %s, which is incompatible with the non-renderable "
        "format (%s).",
        usage, wgpu::TextureUsage::RenderAttachment, format->format);

    DAWN_INVALID_IF(textureDimension == wgpu::TextureDimension::e1D &&
                        (usage & wgpu::TextureUsage::RenderAttachment),
                    "The texture usage (%s) includes %s, which is incompatible with the texture "
                    "dimension (%s).",
                    usage, wgpu::TextureUsage::RenderAttachment, textureDimension);

    DAWN_INVALID_IF(
        !format->supportsStorageUsage && (usage & wgpu::TextureUsage::StorageBinding),
        "The texture usage (%s) includes %s, which is incompatible with the format (%s).", usage,
        wgpu::TextureUsage::StorageBinding, format->format);

    DAWN_INVALID_IF(
        !format->supportsStorageAttachment && (usage & wgpu::TextureUsage::StorageAttachment),
        "The texture usage (%s) includes %s, which is incompatible with the format (%s).", usage,
        wgpu::TextureUsage::StorageAttachment, format->format);

    const auto kTransientAttachment = wgpu::TextureUsage::TransientAttachment;
    if (usage & kTransientAttachment) {
        DAWN_INVALID_IF(
            !device->HasFeature(Feature::TransientAttachments),
            "The texture usage (%s) includes %s, which requires the %s feature to be set", usage,
            kTransientAttachment, ToAPI(Feature::TransientAttachments));

        DAWN_INVALID_IF(
            usage == kTransientAttachment,
            "The texture usage is only %s (which always requires another attachment usage).",
            kTransientAttachment);
        const auto kAttachmentUsages = kTransientAttachment | wgpu::TextureUsage::RenderAttachment |
                                       wgpu::TextureUsage::StorageAttachment;
        DAWN_INVALID_IF(!IsSubset(usage, kAttachmentUsages),
                        "The texture usage (%s) includes both %s and non-attachment usages (%s).",
                        usage, kTransientAttachment, usage & ~kAttachmentUsages);
    }

    if (!allowedSharedTextureMemoryUsage) {
        // Legacy path
        // TODO(crbug.com/dawn/1795): Remove after migrating all old usages.
        // Only allows simple readonly texture usages.
        wgpu::TextureUsage validMultiPlanarUsages =
            wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc;
        if (device->HasFeature(Feature::MultiPlanarFormatExtendedUsages)) {
            validMultiPlanarUsages |= wgpu::TextureUsage::CopyDst;
        }
        if (device->HasFeature(Feature::MultiPlanarRenderTargets)) {
            validMultiPlanarUsages |= wgpu::TextureUsage::RenderAttachment;
        }
        DAWN_INVALID_IF(format->IsMultiPlanar() && !IsSubset(usage, validMultiPlanarUsages),
                        "The texture usage (%s) is incompatible with the multi-planar format (%s).",
                        usage, format->format);
    } else {
        DAWN_INVALID_IF(
            !IsSubset(usage, *allowedSharedTextureMemoryUsage),
            "The texture usage (%s) is not a subset of the shared texture memory usage (%s).",
            usage, *allowedSharedTextureMemoryUsage);
    }
    return {};
}

wgpu::TextureUsage GetTextureViewUsage(wgpu::TextureUsage sourceTextureUsage,
                                       wgpu::TextureUsage requestedViewUsage) {
    // If a view's requested usage is None, inherit usage from the source texture.
    return (requestedViewUsage != wgpu::TextureUsage::None) ? requestedViewUsage
                                                            : sourceTextureUsage;
}

wgpu::TextureUsage RemoveInvalidViewUsages(wgpu::TextureUsage viewUsage, const Format* viewFormat) {
    wgpu::TextureUsage adjustedUsage = viewUsage;
    if (viewFormat->format == wgpu::TextureFormat::RGBA8UnormSrgb ||
        viewFormat->format == wgpu::TextureFormat::BGRA8UnormSrgb) {
        adjustedUsage = viewUsage & ~wgpu::TextureUsage::StorageBinding;
    }

    return adjustedUsage;
}

MaybeError ValidateTextureViewUsage(const DeviceBase* device,
                                    const TextureBase* texture,
                                    wgpu::TextureUsage usage,
                                    const Format* format) {
    wgpu::TextureUsage inheritedUsage = GetTextureViewUsage(texture->GetUsage(), usage);

    DAWN_INVALID_IF(!IsSubset(inheritedUsage, texture->GetUsage()),
                    "The texture view usage (%s) is not a subset of the texture usage (%s).",
                    inheritedUsage, texture->GetUsage());

    // Validate the view usage only when it is explicitly requested for now because it is not yet
    // possible to request view usage all the way from the WebGPU API.
    if (usage != wgpu::TextureUsage::None) {
        DAWN_TRY(ValidateTextureUsage(device, texture->GetDimension(), inheritedUsage, format, {}));
    }

    return {};
}

// We need to add an internal RenderAttachment usage to some textures that has CopyDst usage as we
// apply a workaround that writes to them with a render pipeline.
bool CopyDstNeedsInternalRenderAttachmentUsage(const DeviceBase* device, const Format& format) {
    // Depth
    if (format.HasDepth() &&
        (device->IsToggleEnabled(Toggle::UseBlitForBufferToDepthTextureCopy) ||
         device->IsToggleEnabled(
             Toggle::UseBlitForDepthTextureToTextureCopyToNonzeroSubresource))) {
        return true;
    }
    // Stencil
    if (format.HasStencil() &&
        device->IsToggleEnabled(Toggle::UseBlitForBufferToStencilTextureCopy)) {
        return true;
    }

    if (device->IsToggleEnabled(Toggle::UseBlitForB2T) &&
        IsFormatSupportedByBufferToTextureBlit(format.format)) {
        return true;
    }
    return false;
}

// We need to add an internal TextureBinding usage to some textures that has CopySrc usage as we
// apply a workaround that binds them to a compute pipeline for their copy operation.
bool CopySrcNeedsInternalTextureBindingUsage(const DeviceBase* device, const Format& format) {
    // Snorm
    if (format.IsSnorm() && device->IsToggleEnabled(Toggle::UseBlitForSnormTextureToBufferCopy)) {
        return true;
    }
    // BGRA8Unorm
    if (format.format == wgpu::TextureFormat::BGRA8Unorm &&
        device->IsToggleEnabled(Toggle::UseBlitForBGRA8UnormTextureToBufferCopy)) {
        return true;
    }
    // RGB9E5Ufloat
    if (format.format == wgpu::TextureFormat::RGB9E5Ufloat &&
        device->IsToggleEnabled(Toggle::UseBlitForRGB9E5UfloatTextureCopy)) {
        return true;
    }
    // RG11B10ufloat
    if (format.format == wgpu::TextureFormat::RG11B10Ufloat &&
        device->IsToggleEnabled(Toggle::UseBlitForRG11B10UfloatTextureCopy)) {
        return true;
    }
    // float16
    if ((format.format == wgpu::TextureFormat::R16Float ||
         format.format == wgpu::TextureFormat::RG16Float ||
         format.format == wgpu::TextureFormat::RGBA16Float) &&
        device->IsToggleEnabled(Toggle::UseBlitForFloat16TextureCopy)) {
        return true;
    }
    // float32
    if ((format.format == wgpu::TextureFormat::R32Float ||
         format.format == wgpu::TextureFormat::RG32Float ||
         format.format == wgpu::TextureFormat::RGBA32Float) &&
        device->IsToggleEnabled(Toggle::UseBlitForFloat32TextureCopy)) {
        return true;
    }

    // Depth
    if (format.HasDepth() &&
        (device->IsToggleEnabled(Toggle::UseBlitForDepthTextureToTextureCopyToNonzeroSubresource) ||
         (format.format == wgpu::TextureFormat::Depth16Unorm &&
          device->IsToggleEnabled(Toggle::UseBlitForDepth16UnormTextureToBufferCopy)) ||
         (format.format == wgpu::TextureFormat::Depth32Float &&
          device->IsToggleEnabled(Toggle::UseBlitForDepth32FloatTextureToBufferCopy)))) {
        return true;
    }
    // Stencil
    if (format.HasStencil() &&
        device->IsToggleEnabled(Toggle::UseBlitForStencilTextureToBufferCopy)) {
        return true;
    }

    if (device->IsToggleEnabled(Toggle::UseBlitForT2B) &&
        IsFormatSupportedByTextureToBufferBlit(format.format)) {
        return true;
    }

    return false;
}

wgpu::TextureUsage AddInternalUsages(const DeviceBase* device,
                                     wgpu::TextureUsage usage,
                                     const Format& format,
                                     uint32_t sampleCount,
                                     uint32_t mipLevelCount,
                                     uint32_t arrayLayerCount) {
    wgpu::TextureUsage internalUsage = usage;

    // dawn:1569: If a texture with multiple array layers or mip levels is specified as a
    // texture attachment when this toggle is active, it needs to be given CopySrc | CopyDst usage
    // internally.
    bool applyAlwaysResolveIntoZeroLevelAndLayerToggle =
        device->IsToggleEnabled(Toggle::AlwaysResolveIntoZeroLevelAndLayer) &&
        (arrayLayerCount > 1 || mipLevelCount > 1) &&
        (internalUsage & wgpu::TextureUsage::RenderAttachment);
    if (applyAlwaysResolveIntoZeroLevelAndLayerToggle) {
        internalUsage |= wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst;
    }

    if (internalUsage & wgpu::TextureUsage::CopyDst) {
        if (CopyDstNeedsInternalRenderAttachmentUsage(device, format)) {
            internalUsage |= wgpu::TextureUsage::RenderAttachment;
        }
    }
    if (internalUsage & wgpu::TextureUsage::CopySrc) {
        if (CopySrcNeedsInternalTextureBindingUsage(device, format)) {
            internalUsage |= wgpu::TextureUsage::TextureBinding;
        }
    }
    if (internalUsage & wgpu::TextureUsage::StorageBinding) {
        internalUsage |= kReadOnlyStorageTexture | kWriteOnlyStorageTexture;
    }

    bool supportsMSAAPartialResolve = device->HasFeature(Feature::DawnPartialLoadResolveTexture) &&
                                      sampleCount > 1 &&
                                      (usage & wgpu::TextureUsage::RenderAttachment);
    if (supportsMSAAPartialResolve) {
        internalUsage |= wgpu::TextureUsage::TextureBinding;
    }

    return internalUsage;
}

}  // anonymous namespace

MaybeError ValidateTextureViewDescriptor(const DeviceBase* device,
                                         const TextureBase* texture,
                                         const UnpackedPtr<TextureViewDescriptor>& descriptor) {
    // Parent texture should have been already validated.
    DAWN_ASSERT(texture);
    DAWN_ASSERT(!texture->IsError());

    DAWN_TRY(ValidateTextureViewDimension(descriptor->dimension));
    DAWN_TRY(ValidateTextureFormat(descriptor->format));
    DAWN_TRY(ValidateTextureAspect(descriptor->aspect));

    const Format& format = texture->GetFormat();
    const Format* viewFormat;
    DAWN_TRY_ASSIGN(viewFormat, device->GetInternalFormat(descriptor->format));

    DAWN_TRY(ValidateTextureViewUsage(device, texture, descriptor->usage, viewFormat));

    const auto aspect = SelectFormatAspects(format, descriptor->aspect);
    DAWN_INVALID_IF(aspect == Aspect::None,
                    "Texture format (%s) does not have the texture view's selected aspect (%s).",
                    format.format, descriptor->aspect);

    DAWN_INVALID_IF(descriptor->arrayLayerCount == 0 || descriptor->mipLevelCount == 0,
                    "The texture view's arrayLayerCount (%u) or mipLevelCount (%u) is zero.",
                    descriptor->arrayLayerCount, descriptor->mipLevelCount);

    DAWN_INVALID_IF(
        uint64_t(descriptor->baseArrayLayer) + uint64_t(descriptor->arrayLayerCount) >
            uint64_t(texture->GetArrayLayers()),
        "Texture view array layer range (baseArrayLayer: %u, arrayLayerCount: %u) exceeds the "
        "texture's array layer count (%u).",
        descriptor->baseArrayLayer, descriptor->arrayLayerCount, texture->GetArrayLayers());

    DAWN_INVALID_IF(
        uint64_t(descriptor->baseMipLevel) + uint64_t(descriptor->mipLevelCount) >
            uint64_t(texture->GetNumMipLevels()),
        "Texture view mip level range (baseMipLevel: %u, mipLevelCount: %u) exceeds the "
        "texture's mip level count (%u).",
        descriptor->baseMipLevel, descriptor->mipLevelCount, texture->GetNumMipLevels());

    if (descriptor.Get<YCbCrVkDescriptor>()) {
        DAWN_INVALID_IF(!device->HasFeature(Feature::YCbCrVulkanSamplers), "%s is not enabled.",
                        wgpu::FeatureName::YCbCrVulkanSamplers);
        DAWN_INVALID_IF(format.format != wgpu::TextureFormat::External,
                        "Texture format (%s) is not (%s).", format.format,
                        wgpu::TextureFormat::External);
    } else if (format.format == wgpu::TextureFormat::External) {
        return DAWN_VALIDATION_ERROR("Invalid TextureViewDescriptor with Texture format (%s).",
                                     wgpu::TextureFormat::External);
    }

    DAWN_TRY(ValidateCanViewTextureAs(device, texture, *viewFormat, descriptor->aspect));
    DAWN_TRY(ValidateTextureViewDimensionCompatibility(device, texture, descriptor));

    return {};
}

ResultOrError<TextureViewDescriptor> GetTextureViewDescriptorWithDefaults(
    const TextureBase* texture,
    const TextureViewDescriptor* descriptor) {
    DAWN_ASSERT(texture);

    TextureViewDescriptor desc = {};
    if (descriptor) {
        desc = descriptor->WithTrivialFrontendDefaults();
    }

    // The default value for the view dimension depends on the texture's dimension with a
    // special case for 2DArray being chosen if texture is 2D but has more than one array layer.
    if (desc.dimension == wgpu::TextureViewDimension::Undefined) {
        switch (texture->GetDimension()) {
            case wgpu::TextureDimension::Undefined:
                DAWN_UNREACHABLE();

            case wgpu::TextureDimension::e1D:
                desc.dimension = wgpu::TextureViewDimension::e1D;
                break;

            case wgpu::TextureDimension::e2D:
                if (texture->GetArrayLayers() == 1) {
                    desc.dimension = wgpu::TextureViewDimension::e2D;
                } else {
                    desc.dimension = wgpu::TextureViewDimension::e2DArray;
                }
                break;

            case wgpu::TextureDimension::e3D:
                desc.dimension = wgpu::TextureViewDimension::e3D;
                break;
        }
    }

    if (desc.format == wgpu::TextureFormat::Undefined) {
        const Format& format = texture->GetFormat();

        // Check the aspect since |SelectFormatAspects| assumes a valid aspect.
        // Creation would have failed validation later since the aspect is invalid.
        DAWN_TRY(ValidateTextureAspect(desc.aspect));

        Aspect aspects = SelectFormatAspects(format, desc.aspect);
        if (HasOneBit(aspects)) {
            desc.format = format.GetAspectInfo(aspects).format;
        } else {
            desc.format = format.format;
        }
    }

    if (desc.arrayLayerCount == wgpu::kArrayLayerCountUndefined) {
        switch (desc.dimension) {
            case wgpu::TextureViewDimension::e1D:
            case wgpu::TextureViewDimension::e2D:
            case wgpu::TextureViewDimension::e3D:
                desc.arrayLayerCount = 1;
                break;
            case wgpu::TextureViewDimension::Cube:
                desc.arrayLayerCount = 6;
                break;
            case wgpu::TextureViewDimension::e2DArray:
            case wgpu::TextureViewDimension::CubeArray:
                desc.arrayLayerCount = texture->GetArrayLayers() - desc.baseArrayLayer;
                break;
            default:
                // We don't put DAWN_UNREACHABLE() here because we validate enums only after this
                // function sets default values. Otherwise, the DAWN_UNREACHABLE() will be hit.
                break;
        }
    }

    if (desc.mipLevelCount == wgpu::kMipLevelCountUndefined) {
        desc.mipLevelCount = texture->GetNumMipLevels() - desc.baseMipLevel;
    }
    return desc;
}

bool IsTextureViewDimensionCompatibleWithTextureDimension(
    wgpu::TextureViewDimension textureViewDimension,
    wgpu::TextureDimension textureDimension) {
    switch (textureViewDimension) {
        case wgpu::TextureViewDimension::e2D:
        case wgpu::TextureViewDimension::e2DArray:
        case wgpu::TextureViewDimension::Cube:
        case wgpu::TextureViewDimension::CubeArray:
            return textureDimension == wgpu::TextureDimension::e2D;

        case wgpu::TextureViewDimension::e3D:
            return textureDimension == wgpu::TextureDimension::e3D;

        case wgpu::TextureViewDimension::e1D:
            return textureDimension == wgpu::TextureDimension::e1D;

        case wgpu::TextureViewDimension::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

// TextureViewInternalBase

TextureViewInternalBase::TextureViewInternalBase(
    TextureBase* texture,
    const UnpackedPtr<TextureViewDescriptor>& descriptor,
    ApiObjectBase::UntrackedByDeviceTag tag)
    : ApiObjectBase(texture->GetDevice(), descriptor->label),
      mTexture(GetWeakRef(texture)),
      mFormat(GetDevice()->GetValidInternalFormat(descriptor->format)),
      mDimension(descriptor->dimension),
      mRange({ConvertViewAspect(*mFormat, descriptor->aspect),
              {descriptor->baseArrayLayer, descriptor->arrayLayerCount},
              {descriptor->baseMipLevel, descriptor->mipLevelCount}}),
      mUsage(RemoveInvalidViewUsages(GetTextureViewUsage(texture->GetUsage(), descriptor->usage),
                                     &mFormat.get())),
      mInternalUsage(
          AddInternalUsages(GetDevice(),
                            GetTextureViewUsage(texture->GetInternalUsage(), descriptor->usage),
                            *mFormat,
                            texture->GetSampleCount(),
                            texture->GetNumMipLevels(),
                            texture->GetArrayLayers())) {
    // Emit a warning if invalid usages were removed for this view.
    // TODO(363903526): Remove this warning after deprecation period.
    wgpu::TextureUsage inheritedUsage = GetTextureViewUsage(texture->GetUsage(), descriptor->usage);
    if (mUsage != inheritedUsage) {
        DAWN_ASSERT(descriptor->usage == wgpu::TextureUsage::None);
        std::string warning = absl::StrFormat(
            "%s with format (%s) and inherited usage (%s) is deprecated. Please request explicit "
            "usages on texture views when the view format is not compatible with all inherited "
            "texture usages.",
            this, mFormat->format, inheritedUsage);
        GetDevice()->EmitWarningOnce(warning.c_str());
    }
}

TextureViewInternalBase::TextureViewInternalBase(
    TextureBase* texture,
    const UnpackedPtr<TextureViewDescriptor>& descriptor)
    : TextureViewInternalBase(texture, descriptor, kUntrackedByDevice) {
    GetObjectTrackingList()->Track(this);
}

TextureViewInternalBase::~TextureViewInternalBase() = default;

void TextureViewInternalBase::DestroyImpl() {}

ObjectType TextureViewInternalBase::GetType() const {
    return ObjectType::TextureViewInternal;
}

const TextureBase* TextureViewInternalBase::GetTexture() const {
    Ref<TextureBase> texture = mTexture.Promote();
    DAWN_ASSERT(texture);
    return texture.Get();
}

TextureBase* TextureViewInternalBase::GetTexture() {
    Ref<TextureBase> texture = mTexture.Promote();
    DAWN_ASSERT(texture);
    return texture.Get();
}

Ref<TextureViewBase> TextureViewInternalBase::CreateTextureView() {
    Ref<TextureBase> texture = mTexture.Promote();
    DAWN_ASSERT(texture);
    return AcquireRef(new TextureViewBase(texture.Get(), this, GetLabel().c_str()));
}

Aspect TextureViewInternalBase::GetAspects() const {
    return mRange.aspects;
}

const Format& TextureViewInternalBase::GetFormat() const {
    return *mFormat;
}

wgpu::TextureViewDimension TextureViewInternalBase::GetDimension() const {
    return mDimension;
}

uint32_t TextureViewInternalBase::GetBaseMipLevel() const {
    return mRange.baseMipLevel;
}

uint32_t TextureViewInternalBase::GetLevelCount() const {
    return mRange.levelCount;
}

uint32_t TextureViewInternalBase::GetBaseArrayLayer() const {
    return mRange.baseArrayLayer;
}

uint32_t TextureViewInternalBase::GetLayerCount() const {
    return mRange.layerCount;
}

const SubresourceRange& TextureViewInternalBase::GetSubresourceRange() const {
    return mRange;
}

Extent3D TextureViewInternalBase::GetSingleSubresourceVirtualSize() const {
    Ref<TextureBase> texture = mTexture.Promote();
    DAWN_ASSERT(texture);
    return texture->GetMipLevelSingleSubresourceVirtualSize(GetBaseMipLevel(), GetAspects());
}

wgpu::TextureUsage TextureViewInternalBase::GetUsage() const {
    return mUsage;
}

wgpu::TextureUsage TextureViewInternalBase::GetInternalUsage() const {
    return mInternalUsage;
}

bool TextureViewInternalBase::IsYCbCr() const {
    return false;
}

YCbCrVkDescriptor TextureViewInternalBase::GetYCbCrVkDescriptor() const {
    DAWN_UNREACHABLE();
    return {};
}

ApiObjectList* TextureViewInternalBase::GetObjectTrackingList() {
    Ref<TextureBase> texture = mTexture.Promote();
    if (texture != nullptr) {
        return texture->GetViewInternalTrackingList();
    }
    return ApiObjectBase::GetObjectTrackingList();
}

// TextureViewBase

TextureViewBase::TextureViewBase(TextureBase* texture,
                                 Ref<TextureViewInternalBase> internal,
                                 StringView label)
    : ApiObjectBase(texture->GetDevice(), label), mTexture(texture), mInternal(internal) {
    GetObjectTrackingList()->Track(this);
}

TextureViewBase::TextureViewBase(DeviceBase* device, ObjectBase::ErrorTag tag, StringView label)
    : ApiObjectBase(device, tag, label) {}

TextureViewBase::~TextureViewBase() = default;

void TextureViewBase::DestroyImpl() {}

// static
Ref<TextureViewBase> TextureViewBase::MakeError(DeviceBase* device, StringView label) {
    return AcquireRef(new TextureViewBase(device, ObjectBase::kError, label));
}

ObjectType TextureViewBase::GetType() const {
    return ObjectType::TextureView;
}

void TextureViewBase::FormatLabel(absl::FormatSink* s) const {
    s->Append(ObjectTypeAsString(GetType()));

    const std::string& label = GetLabel();
    if (!label.empty()) {
        s->Append(absl::StrFormat(" \"%s\"", label));
    }
    if (IsError()) {
        return;
    }

    if (label.empty()) {
        s->Append(" of ");
        GetTexture()->FormatLabel(s);
    }
}

const TextureBase* TextureViewBase::GetTexture() const {
    DAWN_ASSERT(!IsError());
    return mTexture.Get();
}

TextureBase* TextureViewBase::GetTexture() {
    DAWN_ASSERT(!IsError());
    return mTexture.Get();
}

const TextureViewInternalBase* TextureViewBase::GetInternal() const {
    DAWN_ASSERT(!IsError());
    return mInternal.Get();
}

TextureViewInternalBase* TextureViewBase::GetInternal() {
    DAWN_ASSERT(!IsError());
    return mInternal.Get();
}

ApiObjectList* TextureViewBase::GetObjectTrackingList() {
    if (mTexture != nullptr) {
        return mTexture->GetViewTrackingList();
    }
    // Return the base device list for error objects so that
    // the list is never null. Error texture views are never tracked,
    // so liveness checks will always return false.
    DAWN_ASSERT(IsError());
    return ApiObjectBase::GetObjectTrackingList();
}

}  // namespace dawn::native
