// Copyright 2019 The Dawn & Tint Authors
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

#include "dawn/native/Format.h"

#include <absl/container/flat_hash_map.h>
#include <bitset>
#include <utility>

#include "dawn/common/MatchVariant.h"
#include "dawn/common/TypedInteger.h"
#include "dawn/native/Device.h"
#include "dawn/native/EnumMaskIterator.h"
#include "dawn/native/Features.h"
#include "dawn/native/Texture.h"

namespace dawn {
enum class Cap : uint16_t {
    None = 0x0,
    Multisample = 0x1,
    Renderable = 0x2,
    Resolve = 0x4,
    StorageR = 0x8,      // Read-Only.
    StorageW = 0x10,     // Write-Only
    StorageROrW = 0x18,  // Read Or Write, but not ReadWrite in the shader.
    StorageRW = 0x20,    // Implies StorageR and StorageW
    PLS = 0x40,
    Blendable = 0x80,
};
}  // namespace dawn

template <>
struct wgpu::IsWGPUBitmask<dawn::Cap> {
    static constexpr bool enable = true;
};

namespace dawn::native {

// Format

SampleTypeBit SampleTypeToSampleTypeBit(wgpu::TextureSampleType sampleType) {
    switch (sampleType) {
        case wgpu::TextureSampleType::Float:
        case wgpu::TextureSampleType::UnfilterableFloat:
        case wgpu::TextureSampleType::Sint:
        case wgpu::TextureSampleType::Uint:
        case wgpu::TextureSampleType::Depth:
        case wgpu::TextureSampleType::BindingNotUsed:
        case wgpu::TextureSampleType::Undefined:
            // When the compiler complains that you need to add a case statement here, please
            // also add a corresponding static assert below!
            break;
    }

    static_assert(static_cast<uint32_t>(wgpu::TextureSampleType::BindingNotUsed) == 0);
    if (sampleType == wgpu::TextureSampleType::BindingNotUsed) {
        return SampleTypeBit::None;
    }

    static_assert(static_cast<uint32_t>(wgpu::TextureSampleType::Undefined) == 1);
    if (sampleType == wgpu::TextureSampleType::Undefined) {
        DAWN_UNREACHABLE();
        return SampleTypeBit::None;
    }

    // Check that SampleTypeBit bits are in the same position / order as the respective
    // wgpu::TextureSampleType value.
    static_assert(SampleTypeBit::Float ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Float) - 2)));
    static_assert(
        SampleTypeBit::UnfilterableFloat ==
        static_cast<SampleTypeBit>(
            1 << (static_cast<uint32_t>(wgpu::TextureSampleType::UnfilterableFloat) - 2)));
    static_assert(SampleTypeBit::Uint ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Uint) - 2)));
    static_assert(SampleTypeBit::Sint ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Sint) - 2)));
    static_assert(SampleTypeBit::Depth ==
                  static_cast<SampleTypeBit>(
                      1 << (static_cast<uint32_t>(wgpu::TextureSampleType::Depth) - 2)));
    return static_cast<SampleTypeBit>(1 << (static_cast<uint32_t>(sampleType) - 2));
}

const UnsupportedReason Format::supported;

bool Format::IsSupported() const {
    return std::holds_alternative<std::monostate>(unsupportedReason);
}

bool Format::IsColor() const {
    return aspects == Aspect::Color;
}

bool Format::HasDepth() const {
    return aspects & Aspect::Depth;
}

bool Format::HasStencil() const {
    return aspects & Aspect::Stencil;
}

bool Format::HasDepthOrStencil() const {
    return aspects & (Aspect::Depth | Aspect::Stencil);
}

bool Format::HasAlphaChannel() const {
    // This is true for current formats. May need revisit if new formats and extensions are added.
    return componentCount == 4 && IsColor();
}

bool Format::IsSnorm() const {
    return format == wgpu::TextureFormat::RGBA8Snorm || format == wgpu::TextureFormat::RG8Snorm ||
           format == wgpu::TextureFormat::R8Snorm;
}

bool Format::IsMultiPlanar() const {
    return aspects & (Aspect::Plane0 | Aspect::Plane1 | Aspect::Plane2);
}

bool Format::CopyCompatibleWith(const Format& otherFormat) const {
    // TODO(crbug.com/dawn/1332): Add a Format compatibility matrix.
    return baseFormat == otherFormat.baseFormat;
}

bool Format::ViewCompatibleWith(const Format& otherFormat) const {
    // TODO(crbug.com/dawn/1332): Add a Format compatibility matrix.
    return baseFormat == otherFormat.baseFormat;
}

const AspectInfo& Format::GetAspectInfo(wgpu::TextureAspect aspect) const {
    return GetAspectInfo(SelectFormatAspects(*this, aspect));
}

const AspectInfo& Format::GetAspectInfo(Aspect aspect) const {
    DAWN_ASSERT(HasOneBit(aspect));
    DAWN_ASSERT(aspects & aspect);
    const size_t aspectIndex = GetAspectIndex(aspect);
    DAWN_ASSERT(aspectIndex < GetAspectCount(aspects));
    return aspectInfo[aspectIndex];
}

FormatIndex Format::GetIndex() const {
    return ComputeFormatIndex(format);
}

// FormatSet implementation

bool FormatSet::operator[](const Format& format) const {
    return Base::operator[](format.GetIndex());
}

typename std::bitset<kKnownFormatCount>::reference FormatSet::operator[](const Format& format) {
    return Base::operator[](format.GetIndex());
}

// Implementation details of the format table of the DeviceBase

// For the enum for formats are packed but this might change when we have a broader feature
// mechanism for webgpu.h. Formats start at 1 because 0 is the undefined format.
// Dawn internal formats start with a prefix. We strip the prefix and then pack
// them after the last WebGPU format.
FormatIndex ComputeFormatIndex(wgpu::TextureFormat format) {
    uint32_t formatValue = static_cast<uint32_t>(format);
    switch (formatValue & kEnumPrefixMask) {
        case 0:
            // This takes advantage of overflows to make the index of TextureFormat::Undefined
            // outside of the range of the FormatTable.
            static_assert(static_cast<uint32_t>(wgpu::TextureFormat::Undefined) - 1 >
                          kKnownFormatCount);
            return static_cast<FormatIndex>(formatValue - 1);
        case kDawnEnumPrefix: {
            uint32_t dawnIndex = formatValue & ~kEnumPrefixMask;
            if (dawnIndex < kDawnFormatCount) {
                return static_cast<FormatIndex>(dawnIndex + kWebGPUFormatCount);
            }
            break;
        }
        default:
            break;
    }
    // Invalid format. Return an index outside the format table.
    return FormatIndex(~0u);
}

// Helper struct for color format entry
struct ColorEntry {
    Format format;
    uint32_t renderTargetPixelCost = 0;
    uint32_t renderTargetComponentAlign = 0;
    bool decisionMade = false;
};
// Apply capability flags to a format entry
void ApplyColorCapabilities(ColorEntry& entry, Cap caps) {
    Format& fmt = entry.format;

    if (caps & Cap::Renderable) {
        if (!fmt.isRenderable) {
            DAWN_ASSERT(entry.renderTargetPixelCost != 0 && entry.renderTargetComponentAlign != 0);
            fmt.isRenderable = true;
            fmt.renderTargetPixelByteCost = entry.renderTargetPixelCost;
            fmt.renderTargetComponentAlignment = entry.renderTargetComponentAlign;
        }
    }
    if (caps & Cap::Blendable) {
        fmt.isBlendable = true;
    }
    if (caps & Cap::Multisample) {
        DAWN_ASSERT(fmt.isRenderable);
        fmt.supportsMultisample = true;
    }
    if (caps & Cap::Resolve) {
        fmt.supportsResolveTarget = true;
    }
    if (caps & Cap::StorageR) {
        fmt.supportsReadOnlyStorageUsage = true;
    }
    if (caps & Cap::StorageW) {
        fmt.supportsWriteOnlyStorageUsage = true;
    }
    if (caps & Cap::StorageRW) {
        fmt.supportsReadWriteStorageUsage = true;
    }
    if (caps & Cap::PLS) {
        fmt.supportsStorageAttachment = true;
    }
}

// Configure color format support based on device features
void ConfigureColorFormatSupport(
    const DeviceBase* device,
    absl::flat_hash_map<wgpu::TextureFormat, ColorEntry>& colorFormats) {
    auto SupportFormat = [&](wgpu::TextureFormat fmt, Cap caps) {
        auto it = colorFormats.find(fmt);
        DAWN_ASSERT(it != colorFormats.end());
        ColorEntry& entry = it->second;
        if (!entry.decisionMade) {
            entry.decisionMade = true;
            entry.format.unsupportedReason = Format::supported;
        } else {
            DAWN_ASSERT(entry.format.IsSupported());
        }
        ApplyColorCapabilities(entry, caps);
    };

    auto AddCapability = [&](wgpu::TextureFormat fmt, Cap caps) {
        auto it = colorFormats.find(fmt);
        DAWN_ASSERT(it != colorFormats.end());
        ColorEntry& entry = it->second;
        DAWN_ASSERT(entry.decisionMade);
        DAWN_ASSERT(entry.format.IsSupported());
        ApplyColorCapabilities(entry, caps);
    };

    auto ExplainUnsupported = [&](wgpu::TextureFormat fmt, UnsupportedReason reason) {
        auto it = colorFormats.find(fmt);
        DAWN_ASSERT(it != colorFormats.end());
        ColorEntry& entry = it->second;
        DAWN_ASSERT(!entry.decisionMade);
        entry.decisionMade = true;
        entry.format.unsupportedReason = reason;
    };

    const Cap plsCaps = (device->HasFeature(Feature::PixelLocalStorageCoherent) ||
                         device->HasFeature(Feature::PixelLocalStorageNonCoherent))
                            ? Cap::PLS
                            : Cap::None;
    const Cap float32BlendableCaps =
        device->HasFeature(Feature::Float32Blendable) ? Cap::Blendable : Cap::None;
    const bool textureFormatsTier1 = device->HasFeature(Feature::TextureFormatsTier1);
    const bool textureFormatsTier2 = device->HasFeature(Feature::TextureFormatsTier2);
    const bool norm16 = device->HasFeature(Feature::Norm16TextureFormats);
    const bool unorm16Feature = device->HasFeature(Feature::Unorm16TextureFormats);
    const bool snorm16Feature = device->HasFeature(Feature::Snorm16TextureFormats);
    const bool compatMode = device->IsCompatibilityMode();
    const Cap multisampleCaps = compatMode ? Cap::None : Cap::Multisample;
    const Cap storageCaps = compatMode ? Cap::None : Cap::StorageROrW;

    // Core renderable formats
    for (wgpu::TextureFormat fmt :
         {wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm,
          wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureFormat::RGBA8UnormSrgb,
          wgpu::TextureFormat::BGRA8Unorm}) {
        SupportFormat(fmt, Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    }

    if (compatMode) {
        ExplainUnsupported(wgpu::TextureFormat::BGRA8UnormSrgb, CompatibilityMode{});
    } else {
        SupportFormat(wgpu::TextureFormat::BGRA8UnormSrgb,
                      Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    }

    // Integer formats
    SupportFormat(wgpu::TextureFormat::R8Uint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::R8Sint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::R16Uint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::R16Sint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RG8Uint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RG8Sint, Cap::Renderable | multisampleCaps);

    // Float and mixed formats
    SupportFormat(wgpu::TextureFormat::R16Float,
                  Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    SupportFormat(wgpu::TextureFormat::RG16Uint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RG16Sint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RG16Float,
                  Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);

    // 32-bit formats with storage
    SupportFormat(wgpu::TextureFormat::R32Uint,
                  Cap::Renderable | Cap::StorageROrW | Cap::StorageRW | plsCaps);
    SupportFormat(wgpu::TextureFormat::R32Sint,
                  Cap::Renderable | Cap::StorageROrW | Cap::StorageRW | plsCaps);
    SupportFormat(wgpu::TextureFormat::R32Float, Cap::Renderable | Cap::StorageROrW |
                                                     Cap::StorageRW | plsCaps |
                                                     float32BlendableCaps | multisampleCaps);

    // RGBA formats
    SupportFormat(wgpu::TextureFormat::RGBA8Unorm, Cap::Renderable | Cap::Multisample |
                                                       Cap::Resolve | Cap::Blendable |
                                                       Cap::StorageROrW);
    SupportFormat(wgpu::TextureFormat::RGBA8UnormSrgb,
                  Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    SupportFormat(wgpu::TextureFormat::RGBA8Uint,
                  Cap::Renderable | Cap::StorageROrW | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RGBA8Sint,
                  Cap::Renderable | Cap::StorageROrW | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::BGRA8Unorm,
                  Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    SupportFormat(wgpu::TextureFormat::RGB10A2Uint, Cap::Renderable | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RGB10A2Unorm,
                  Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);

    // Special formats
    SupportFormat(wgpu::TextureFormat::RG11B10Ufloat, Cap::None);
    SupportFormat(wgpu::TextureFormat::RGB9E5Ufloat, Cap::None);

    if (device->HasFeature(Feature::YCbCrVulkanSamplers)) {
        SupportFormat(wgpu::TextureFormat::External, Cap::None);
    } else {
        ExplainUnsupported(wgpu::TextureFormat::External,
                           RequiresFeature{wgpu::FeatureName::YCbCrVulkanSamplers});
    }

    // 64-bit and larger formats
    SupportFormat(wgpu::TextureFormat::RG32Uint, Cap::Renderable | storageCaps);
    SupportFormat(wgpu::TextureFormat::RG32Sint, Cap::Renderable | storageCaps);
    SupportFormat(wgpu::TextureFormat::RG32Float,
                  Cap::Renderable | storageCaps | float32BlendableCaps);
    SupportFormat(wgpu::TextureFormat::RGBA16Uint,
                  Cap::Renderable | Cap::StorageROrW | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RGBA16Sint,
                  Cap::Renderable | Cap::StorageROrW | multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RGBA16Float, Cap::Renderable | Cap::StorageROrW |
                                                        Cap::Resolve | Cap::Blendable |
                                                        multisampleCaps);
    SupportFormat(wgpu::TextureFormat::RGBA32Uint, Cap::Renderable | Cap::StorageROrW);
    SupportFormat(wgpu::TextureFormat::RGBA32Sint, Cap::Renderable | Cap::StorageROrW);
    SupportFormat(wgpu::TextureFormat::RGBA32Float,
                  Cap::Renderable | Cap::StorageROrW | float32BlendableCaps);

    // Snorm8 tier support
    if (textureFormatsTier1) {
        SupportFormat(wgpu::TextureFormat::RGBA8Snorm, Cap::Renderable | Cap::Multisample |
                                                           Cap::Resolve | Cap::Blendable |
                                                           Cap::StorageROrW);
        SupportFormat(wgpu::TextureFormat::R8Snorm, Cap::Renderable | Cap::Multisample |
                                                        Cap::Resolve | Cap::Blendable |
                                                        Cap::StorageROrW);
        SupportFormat(wgpu::TextureFormat::RG8Snorm, Cap::Renderable | Cap::Multisample |
                                                         Cap::Resolve | Cap::Blendable |
                                                         Cap::StorageROrW);
        for (wgpu::TextureFormat fmt :
             {wgpu::TextureFormat::R8Uint, wgpu::TextureFormat::R8Sint,
              wgpu::TextureFormat::R16Uint, wgpu::TextureFormat::R16Sint,
              wgpu::TextureFormat::R16Float, wgpu::TextureFormat::RG8Unorm,
              wgpu::TextureFormat::RG8Uint, wgpu::TextureFormat::RG8Sint,
              wgpu::TextureFormat::RG16Uint, wgpu::TextureFormat::RG16Sint,
              wgpu::TextureFormat::RG16Float, wgpu::TextureFormat::RGB10A2Uint,
              wgpu::TextureFormat::RGB10A2Unorm, wgpu::TextureFormat::RG11B10Ufloat}) {
            AddCapability(fmt, Cap::StorageROrW);
        }
    } else {
        SupportFormat(wgpu::TextureFormat::RGBA8Snorm, Cap::StorageROrW);
        SupportFormat(wgpu::TextureFormat::R8Snorm, Cap::None);
        SupportFormat(wgpu::TextureFormat::RG8Snorm, Cap::None);
    }

    // Storage tier upgrades
    if (textureFormatsTier1 || device->HasFeature(Feature::R8UnormStorage)) {
        AddCapability(wgpu::TextureFormat::R8Unorm, Cap::StorageROrW);
    }

    if (textureFormatsTier2) {
        for (wgpu::TextureFormat fmt :
             {wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::R8Uint,
              wgpu::TextureFormat::R8Sint, wgpu::TextureFormat::R16Uint,
              wgpu::TextureFormat::R16Sint, wgpu::TextureFormat::R16Float,
              wgpu::TextureFormat::RGBA8Unorm, wgpu::TextureFormat::RGBA8Uint,
              wgpu::TextureFormat::RGBA8Sint, wgpu::TextureFormat::RGBA16Uint,
              wgpu::TextureFormat::RGBA16Sint, wgpu::TextureFormat::RGBA16Float,
              wgpu::TextureFormat::RGBA32Uint, wgpu::TextureFormat::RGBA32Sint,
              wgpu::TextureFormat::RGBA32Float}) {
            AddCapability(fmt, Cap::StorageRW);
        }
    }

    if (device->HasFeature(Feature::BGRA8UnormStorage)) {
        AddCapability(wgpu::TextureFormat::BGRA8Unorm, Cap::StorageW);
    }

    if (device->HasFeature(Feature::RG11B10UfloatRenderable)) {
        AddCapability(wgpu::TextureFormat::RG11B10Ufloat,
                      Cap::Renderable | Cap::Multisample | Cap::Resolve | Cap::Blendable);
    }

    // Norm16 / Snorm16 conditional support
    auto Handle16 = [&](wgpu::TextureFormat fmt, bool isSnorm) {
        if (textureFormatsTier1) {
            SupportFormat(fmt,
                          Cap::Renderable | Cap::Multisample | Cap::Blendable | Cap::StorageROrW);
        } else {
            bool supported = norm16 || (isSnorm ? snorm16Feature : unorm16Feature);
            if (supported) {
                SupportFormat(fmt, Cap::Renderable | Cap::Multisample | Cap::Resolve);
            } else {
                ExplainUnsupported(
                    fmt, RequiresFeature{isSnorm ? wgpu::FeatureName::Snorm16TextureFormats
                                                 : wgpu::FeatureName::Unorm16TextureFormats});
            }
        }
    };

    Handle16(wgpu::TextureFormat::R16Unorm, false);
    Handle16(wgpu::TextureFormat::RG16Unorm, false);
    Handle16(wgpu::TextureFormat::RGBA16Unorm, false);
    Handle16(wgpu::TextureFormat::R16Snorm, true);
    Handle16(wgpu::TextureFormat::RG16Snorm, true);
    Handle16(wgpu::TextureFormat::RGBA16Snorm, true);
}

FormatTable BuildFormatTable(const DeviceBase* device) {
    FormatTable table;
    FormatSet formatsSet;

    static constexpr SampleTypeBit kAnyFloat =
        SampleTypeBit::Float | SampleTypeBit::UnfilterableFloat;

    auto AddFormat = [&table, &formatsSet](Format format) {
        FormatIndex index = ComputeFormatIndex(format.format);
        DAWN_ASSERT(index < table.size());

        // This checks that each format is set at most once, the first part of checking that all
        // formats are set exactly once.
        DAWN_ASSERT(!formatsSet[index]);

        // Vulkan describes bytesPerRow in units of texels. If there's any format for which this
        // DAWN_ASSERT isn't true, then additional validation on bytesPerRow must be added.
        const bool hasMultipleAspects = !HasOneBit(format.aspects);
        DAWN_ASSERT(hasMultipleAspects ||
                    (kTextureBytesPerRowAlignment % format.aspectInfo[0].block.byteSize) == 0);

        table[index] = format;
        formatsSet.set(index);
    };

    using Width = TypedInteger<struct WidthT, uint32_t>;
    using Height = TypedInteger<struct HeightT, uint32_t>;
    using ByteSize = TypedInteger<struct ByteSizeT, uint32_t>;
    using ComponentCount = TypedInteger<struct ComponentCountT, uint32_t>;

    absl::flat_hash_map<wgpu::TextureFormat, ColorEntry> colorFormats;

    // Helper to define a color format entry
    auto DefineColorFormat = [&](wgpu::TextureFormat fmt, uint32_t byteSize,
                                 SampleTypeBit sampleTypes, uint32_t componentCount,
                                 uint32_t rtPixelCost = 0, uint32_t rtComponentAlign = 0,
                                 wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
        Format internalFormat;
        internalFormat.format = fmt;
        internalFormat.baseFormat =
            (baseFormat == wgpu::TextureFormat::Undefined) ? fmt : baseFormat;
        internalFormat.isCompressed = false;
        internalFormat.aspects = Aspect::Color;
        internalFormat.componentCount = componentCount;
        internalFormat.unsupportedReason = std::monostate{};

        AspectInfo* aspect = internalFormat.aspectInfo.data();
        aspect->block.byteSize = byteSize;
        aspect->block.width = 1;
        aspect->block.height = 1;
        aspect->supportedSampleTypes = sampleTypes;
        aspect->format = fmt;

        if (HasOneBit(sampleTypes)) {
            switch (sampleTypes) {
                case SampleTypeBit::Float:
                case SampleTypeBit::UnfilterableFloat:
                case SampleTypeBit::External:
                    aspect->baseType = TextureComponentType::Float;
                    break;
                case SampleTypeBit::Uint:
                    aspect->baseType = TextureComponentType::Uint;
                    break;
                case SampleTypeBit::Sint:
                    aspect->baseType = TextureComponentType::Sint;
                    break;
                default:
                    DAWN_UNREACHABLE();
            }
        } else {
            aspect->baseType = TextureComponentType::Float;
        }

        colorFormats[fmt] = ColorEntry{internalFormat, rtPixelCost, rtComponentAlign, false};
    };

    SampleTypeBit sampleTypeFor32BitFloatFormats = device->HasFeature(Feature::Float32Filterable)
                                                       ? kAnyFloat
                                                       : SampleTypeBit::UnfilterableFloat;
    // 1 byte
    DefineColorFormat(wgpu::TextureFormat::R8Unorm, 1, kAnyFloat, 1, 1, 1);
    DefineColorFormat(wgpu::TextureFormat::R8Snorm, 1, kAnyFloat, 1, 1, 1);
    DefineColorFormat(wgpu::TextureFormat::R8Uint, 1, SampleTypeBit::Uint, 1, 1, 1);
    DefineColorFormat(wgpu::TextureFormat::R8Sint, 1, SampleTypeBit::Sint, 1, 1, 1);

    // 2 bytes
    DefineColorFormat(wgpu::TextureFormat::R16Uint, 2, SampleTypeBit::Uint, 1, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::R16Sint, 2, SampleTypeBit::Sint, 1, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::R16Float, 2, kAnyFloat, 1, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::RG8Unorm, 2, kAnyFloat, 2, 2, 1);
    DefineColorFormat(wgpu::TextureFormat::RG8Snorm, 2, kAnyFloat, 2, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::RG8Uint, 2, SampleTypeBit::Uint, 2, 2, 1);
    DefineColorFormat(wgpu::TextureFormat::RG8Sint, 2, SampleTypeBit::Sint, 2, 2, 1);

    // 4 bytes
    DefineColorFormat(wgpu::TextureFormat::R32Uint, 4, SampleTypeBit::Uint, 1, 4, 4);
    DefineColorFormat(wgpu::TextureFormat::R32Sint, 4, SampleTypeBit::Sint, 1, 4, 4);
    DefineColorFormat(wgpu::TextureFormat::R32Float, 4, sampleTypeFor32BitFloatFormats, 1, 4, 4);
    DefineColorFormat(wgpu::TextureFormat::RG16Uint, 4, SampleTypeBit::Uint, 2, 4, 2);
    DefineColorFormat(wgpu::TextureFormat::RG16Sint, 4, SampleTypeBit::Sint, 2, 4, 2);
    DefineColorFormat(wgpu::TextureFormat::RG16Float, 4, kAnyFloat, 2, 4, 2);
    DefineColorFormat(wgpu::TextureFormat::RGBA8Unorm, 4, kAnyFloat, 4, 8, 1);
    DefineColorFormat(wgpu::TextureFormat::RGBA8UnormSrgb, 4, kAnyFloat, 4, 8, 1,
                      wgpu::TextureFormat::RGBA8Unorm);
    DefineColorFormat(wgpu::TextureFormat::RGBA8Snorm, 4, kAnyFloat, 4, 4, 4);
    DefineColorFormat(wgpu::TextureFormat::RGBA8Uint, 4, SampleTypeBit::Uint, 4, 4, 1);
    DefineColorFormat(wgpu::TextureFormat::RGBA8Sint, 4, SampleTypeBit::Sint, 4, 4, 1);
    DefineColorFormat(wgpu::TextureFormat::BGRA8Unorm, 4, kAnyFloat, 4, 8, 1);
    DefineColorFormat(wgpu::TextureFormat::BGRA8UnormSrgb, 4, kAnyFloat, 4, 8, 1,
                      wgpu::TextureFormat::BGRA8Unorm);
    DefineColorFormat(wgpu::TextureFormat::RGB10A2Uint, 4, SampleTypeBit::Uint, 4, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RGB10A2Unorm, 4, kAnyFloat, 4, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RG11B10Ufloat, 4, kAnyFloat, 3, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RGB9E5Ufloat, 4, kAnyFloat, 3);
    DefineColorFormat(wgpu::TextureFormat::External, 1, SampleTypeBit::External, 0);

    // 8 bytes
    DefineColorFormat(wgpu::TextureFormat::RG32Uint, 8, SampleTypeBit::Uint, 2, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RG32Sint, 8, SampleTypeBit::Sint, 2, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RG32Float, 8, sampleTypeFor32BitFloatFormats, 2, 8, 4);
    DefineColorFormat(wgpu::TextureFormat::RGBA16Uint, 8, SampleTypeBit::Uint, 4, 8, 2);
    DefineColorFormat(wgpu::TextureFormat::RGBA16Sint, 8, SampleTypeBit::Sint, 4, 8, 2);
    DefineColorFormat(wgpu::TextureFormat::RGBA16Float, 8, kAnyFloat, 4, 8, 2);

    // 16 bytes
    DefineColorFormat(wgpu::TextureFormat::RGBA32Uint, 16, SampleTypeBit::Uint, 4, 16, 4);
    DefineColorFormat(wgpu::TextureFormat::RGBA32Sint, 16, SampleTypeBit::Sint, 4, 16, 4);
    DefineColorFormat(wgpu::TextureFormat::RGBA32Float, 16, sampleTypeFor32BitFloatFormats, 4, 16,
                      4);

    // Norm16
    DefineColorFormat(wgpu::TextureFormat::R16Unorm, 2, kAnyFloat, 1, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::RG16Unorm, 4, kAnyFloat, 2, 4, 2);
    DefineColorFormat(wgpu::TextureFormat::RGBA16Unorm, 8, kAnyFloat, 4, 8, 2);
    DefineColorFormat(wgpu::TextureFormat::R16Snorm, 2, kAnyFloat, 1, 2, 2);
    DefineColorFormat(wgpu::TextureFormat::RG16Snorm, 4, kAnyFloat, 2, 4, 2);
    DefineColorFormat(wgpu::TextureFormat::RGBA16Snorm, 8, kAnyFloat, 4, 8, 2);

    // Now that all color formats are defined, configure their support based on device features
    ConfigureColorFormatSupport(device, colorFormats);

    for (const auto& [fmt, entry] : colorFormats) {
        DAWN_ASSERT(entry.decisionMade);
        AddFormat(entry.format);
    }

    auto AddDepthFormat = [&AddFormat](wgpu::TextureFormat format, uint32_t byteSize,
                                       UnsupportedReason unsupportedReason) {
        Format internalFormat;
        internalFormat.format = format;
        internalFormat.baseFormat = format;
        internalFormat.isRenderable = true;
        internalFormat.isBlendable = false;
        internalFormat.isCompressed = false;
        internalFormat.unsupportedReason = unsupportedReason;
        internalFormat.supportsReadOnlyStorageUsage = false;
        internalFormat.supportsWriteOnlyStorageUsage = false;
        internalFormat.supportsReadWriteStorageUsage = false;
        internalFormat.supportsMultisample = true;
        internalFormat.supportsResolveTarget = false;
        internalFormat.aspects = Aspect::Depth;
        internalFormat.componentCount = 1;

        AspectInfo* firstAspect = internalFormat.aspectInfo.data();
        firstAspect->block.byteSize = byteSize;
        firstAspect->block.width = 1;
        firstAspect->block.height = 1;
        firstAspect->baseType = TextureComponentType::Float;
        firstAspect->supportedSampleTypes = SampleTypeBit::Depth | SampleTypeBit::UnfilterableFloat;
        firstAspect->format = format;
        AddFormat(internalFormat);
    };

    auto AddStencilFormat = [&AddFormat](wgpu::TextureFormat format,
                                         UnsupportedReason unsupportedReason) {
        Format internalFormat;
        internalFormat.format = format;
        internalFormat.baseFormat = format;
        internalFormat.isRenderable = true;
        internalFormat.isBlendable = false;
        internalFormat.isCompressed = false;
        internalFormat.unsupportedReason = unsupportedReason;
        internalFormat.supportsReadOnlyStorageUsage = false;
        internalFormat.supportsWriteOnlyStorageUsage = false;
        internalFormat.supportsReadWriteStorageUsage = false;
        internalFormat.supportsMultisample = true;
        internalFormat.supportsResolveTarget = false;
        internalFormat.aspects = Aspect::Stencil;
        internalFormat.componentCount = 1;

        // Duplicate the data for the stencil aspect in both the first and second aspect info.
        //  - aspectInfo[0] is used by AddMultiAspectFormat to copy the info for the whole
        //    stencil8 aspect of depth-stencil8 formats.
        //  - aspectInfo[1] is the actual info used in the rest of Dawn since
        //    GetAspectIndex(Aspect::Stencil) is 1.
        DAWN_ASSERT(GetAspectIndex(Aspect::Stencil) == 1);

        internalFormat.aspectInfo[0].block.byteSize = 1;
        internalFormat.aspectInfo[0].block.width = 1;
        internalFormat.aspectInfo[0].block.height = 1;
        internalFormat.aspectInfo[0].baseType = TextureComponentType::Uint;
        internalFormat.aspectInfo[0].supportedSampleTypes = SampleTypeBit::Uint;
        internalFormat.aspectInfo[0].format = format;

        internalFormat.aspectInfo[1] = internalFormat.aspectInfo[0];

        AddFormat(internalFormat);
    };

    auto BaseCompressedFormat = [](wgpu::TextureFormat format, ByteSize byteSize, Width width,
                                   Height height, UnsupportedReason unsupportedReason,
                                   ComponentCount componentCount, wgpu::TextureFormat baseFormat) {
        Format internalFormat;
        internalFormat.format = format;
        internalFormat.isRenderable = false;
        internalFormat.isBlendable = false;
        internalFormat.isCompressed = true;
        internalFormat.unsupportedReason = unsupportedReason;
        internalFormat.supportsReadOnlyStorageUsage = false;
        internalFormat.supportsWriteOnlyStorageUsage = false;
        internalFormat.supportsReadWriteStorageUsage = false;
        internalFormat.supportsMultisample = false;
        internalFormat.supportsResolveTarget = false;
        internalFormat.aspects = Aspect::Color;
        internalFormat.componentCount = static_cast<uint32_t>(componentCount);

        // Default baseFormat of each compressed formats should be themselves.
        if (baseFormat == wgpu::TextureFormat::Undefined) {
            internalFormat.baseFormat = format;
        } else {
            internalFormat.baseFormat = baseFormat;
        }

        AspectInfo* firstAspect = internalFormat.aspectInfo.data();
        firstAspect->block.byteSize = static_cast<uint32_t>(byteSize);
        firstAspect->block.width = static_cast<uint32_t>(width);
        firstAspect->block.height = static_cast<uint32_t>(height);
        firstAspect->baseType = TextureComponentType::Float;
        firstAspect->supportedSampleTypes = kAnyFloat;
        firstAspect->format = format;
        return internalFormat;
    };

    auto AddETCCompressedFormat =
        [&BaseCompressedFormat, &AddFormat](
            wgpu::TextureFormat format, ByteSize byteSize, Width width, Height height,
            UnsupportedReason unsupportedReason, ComponentCount componentCount,
            wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat = BaseCompressedFormat(
                format, byteSize, width, height, unsupportedReason, componentCount, baseFormat);
            AddFormat(internalFormat);
        };

    auto AddBCCompressedFormat =
        [&BaseCompressedFormat, &AddFormat](
            wgpu::TextureFormat format, ByteSize byteSize, Width width, Height height,
            UnsupportedReason unsupportedReason, ComponentCount componentCount,
            wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat = BaseCompressedFormat(
                format, byteSize, width, height, unsupportedReason, componentCount, baseFormat);
            internalFormat.isBC = true;
            AddFormat(internalFormat);
        };

    auto AddASTCCompressedFormat =
        [&BaseCompressedFormat, &AddFormat](
            wgpu::TextureFormat format, ByteSize byteSize, Width width, Height height,
            UnsupportedReason unsupportedReason, ComponentCount componentCount,
            wgpu::TextureFormat baseFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat = BaseCompressedFormat(
                format, byteSize, width, height, unsupportedReason, componentCount, baseFormat);
            internalFormat.isASTC = true;
            AddFormat(internalFormat);
        };

    auto AddMultiAspectFormat =
        [&AddFormat, &table](wgpu::TextureFormat format, TextureSubsampling subSampling,
                             Aspect aspects, Cap capabilites, UnsupportedReason unsupportedReason,
                             ComponentCount componentCount, wgpu::TextureFormat firstFormat,
                             wgpu::TextureFormat secondFormat,
                             wgpu::TextureFormat thirdFormat = wgpu::TextureFormat::Undefined) {
            Format internalFormat;
            internalFormat.format = format;
            internalFormat.baseFormat = format;
            internalFormat.subSampling = subSampling;
            internalFormat.isRenderable = capabilites & Cap::Renderable;
            internalFormat.isBlendable = false;
            internalFormat.isCompressed = false;
            internalFormat.unsupportedReason = unsupportedReason;
            internalFormat.supportsReadOnlyStorageUsage = false;
            internalFormat.supportsWriteOnlyStorageUsage = false;
            internalFormat.supportsReadWriteStorageUsage = false;
            internalFormat.supportsMultisample = capabilites & Cap::Multisample;
            internalFormat.supportsResolveTarget = false;
            internalFormat.aspects = aspects;
            internalFormat.componentCount = static_cast<uint32_t>(componentCount);

            // Multi aspect formats just copy information about single-aspect formats. This
            // means that the single-plane formats must have been added before multi-aspect
            // ones. (it is ASSERTed below).
            const FormatIndex firstFormatIndex = ComputeFormatIndex(firstFormat);
            const FormatIndex secondFormatIndex = ComputeFormatIndex(secondFormat);

            DAWN_ASSERT(table[firstFormatIndex].aspectInfo[0].format !=
                        wgpu::TextureFormat::Undefined);
            DAWN_ASSERT(table[secondFormatIndex].aspectInfo[0].format !=
                        wgpu::TextureFormat::Undefined);

            internalFormat.aspectInfo[0] = table[firstFormatIndex].aspectInfo[0];
            internalFormat.aspectInfo[1] = table[secondFormatIndex].aspectInfo[0];
            if (thirdFormat != wgpu::TextureFormat::Undefined) {
                const FormatIndex thirdFormatIndex = ComputeFormatIndex(thirdFormat);
                internalFormat.aspectInfo[2] = table[thirdFormatIndex].aspectInfo[0];
            }

            AddFormat(internalFormat);
        };

    // Depth-stencil formats
    AddStencilFormat(wgpu::TextureFormat::Stencil8, Format::supported);
    AddDepthFormat(wgpu::TextureFormat::Depth16Unorm, 2, Format::supported);
    // TODO(crbug.com/dawn/843): This is 4 because we read this to perform zero initialization,
    // and textures are always use depth32float. We should improve this to be more robust. Perhaps,
    // using 0 here to mean "unsized" and adding a backend-specific query for the block size.
    AddDepthFormat(wgpu::TextureFormat::Depth24Plus, 4, Format::supported);
    AddMultiAspectFormat(wgpu::TextureFormat::Depth24PlusStencil8, TextureSubsampling::Undefined,
                          Aspect::Depth | Aspect::Stencil, Cap::Renderable | Cap::Multisample, Format::supported, ComponentCount(2), wgpu::TextureFormat::Depth24Plus, wgpu::TextureFormat::Stencil8);
    AddDepthFormat(wgpu::TextureFormat::Depth32Float, 4, Format::supported);
    UnsupportedReason d32s8UnsupportedReason = device->HasFeature(Feature::Depth32FloatStencil8) ? Format::supported : RequiresFeature{wgpu::FeatureName::Depth32FloatStencil8};
    AddMultiAspectFormat(wgpu::TextureFormat::Depth32FloatStencil8, TextureSubsampling::Undefined,
                          Aspect::Depth | Aspect::Stencil, Cap::Renderable | Cap::Multisample, d32s8UnsupportedReason, ComponentCount(2), wgpu::TextureFormat::Depth32Float, wgpu::TextureFormat::Stencil8);

    // BC compressed formats
    UnsupportedReason bcFormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionBC) ? Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionBC};
    AddBCCompressedFormat(wgpu::TextureFormat::BC1RGBAUnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddBCCompressedFormat(wgpu::TextureFormat::BC1RGBAUnormSrgb, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC1RGBAUnorm);
    AddBCCompressedFormat(wgpu::TextureFormat::BC4RSnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(1));
    AddBCCompressedFormat(wgpu::TextureFormat::BC4RUnorm, ByteSize(8), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(1));
    AddBCCompressedFormat(wgpu::TextureFormat::BC2RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddBCCompressedFormat(wgpu::TextureFormat::BC2RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC2RGBAUnorm);
    AddBCCompressedFormat(wgpu::TextureFormat::BC3RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddBCCompressedFormat(wgpu::TextureFormat::BC3RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC3RGBAUnorm);
    AddBCCompressedFormat(wgpu::TextureFormat::BC5RGSnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(2));
    AddBCCompressedFormat(wgpu::TextureFormat::BC5RGUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(2));
    AddBCCompressedFormat(wgpu::TextureFormat::BC6HRGBFloat, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(3));
    AddBCCompressedFormat(wgpu::TextureFormat::BC6HRGBUfloat, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(3));
    AddBCCompressedFormat(wgpu::TextureFormat::BC7RGBAUnorm, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4));
    AddBCCompressedFormat(wgpu::TextureFormat::BC7RGBAUnormSrgb, ByteSize(16), Width(4), Height(4), bcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::BC7RGBAUnorm);

    // ETC2/EAC compressed formats
    UnsupportedReason etc2FormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionETC2) ?  Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionETC2};
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGB8Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(3));
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGB8UnormSrgb, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(3), wgpu::TextureFormat::ETC2RGB8Unorm);
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGB8A1Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4));
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGB8A1UnormSrgb, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ETC2RGB8A1Unorm);
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGBA8Unorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4));
    AddETCCompressedFormat(wgpu::TextureFormat::ETC2RGBA8UnormSrgb, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ETC2RGBA8Unorm);
    AddETCCompressedFormat(wgpu::TextureFormat::EACR11Unorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(1));
    AddETCCompressedFormat(wgpu::TextureFormat::EACR11Snorm, ByteSize(8), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(1));
    AddETCCompressedFormat(wgpu::TextureFormat::EACRG11Unorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(2));
    AddETCCompressedFormat(wgpu::TextureFormat::EACRG11Snorm, ByteSize(16), Width(4), Height(4), etc2FormatUnsupportedReason, ComponentCount(2));

    // ASTC compressed formats
    UnsupportedReason astcFormatUnsupportedReason = device->HasFeature(Feature::TextureCompressionASTC) ?  Format::supported : RequiresFeature{wgpu::FeatureName::TextureCompressionASTC};
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC4x4Unorm, ByteSize(16), Width(4), Height(4), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC4x4UnormSrgb, ByteSize(16), Width(4), Height(4), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC4x4Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC5x4Unorm, ByteSize(16), Width(5), Height(4), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC5x4UnormSrgb, ByteSize(16), Width(5), Height(4), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC5x4Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC5x5Unorm, ByteSize(16), Width(5), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC5x5UnormSrgb, ByteSize(16), Width(5), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC5x5Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC6x5Unorm, ByteSize(16), Width(6), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC6x5UnormSrgb, ByteSize(16), Width(6), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC6x5Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC6x6Unorm, ByteSize(16), Width(6), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC6x6UnormSrgb, ByteSize(16), Width(6), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC6x6Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x5Unorm, ByteSize(16), Width(8), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x5UnormSrgb, ByteSize(16), Width(8), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x5Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x6Unorm, ByteSize(16), Width(8), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x6UnormSrgb, ByteSize(16), Width(8), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x6Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x8Unorm, ByteSize(16), Width(8), Height(8), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC8x8UnormSrgb, ByteSize(16), Width(8), Height(8), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC8x8Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x5Unorm, ByteSize(16), Width(10), Height(5), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x5UnormSrgb, ByteSize(16), Width(10), Height(5), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x5Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x6Unorm, ByteSize(16), Width(10), Height(6), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x6UnormSrgb, ByteSize(16), Width(10), Height(6), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x6Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x8Unorm, ByteSize(16), Width(10), Height(8), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x8UnormSrgb, ByteSize(16), Width(10), Height(8), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x8Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x10Unorm, ByteSize(16), Width(10), Height(10), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC10x10UnormSrgb, ByteSize(16), Width(10), Height(10), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC10x10Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC12x10Unorm, ByteSize(16), Width(12), Height(10), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC12x10UnormSrgb, ByteSize(16), Width(12), Height(10), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC12x10Unorm);
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC12x12Unorm, ByteSize(16), Width(12), Height(12), astcFormatUnsupportedReason, ComponentCount(4));
    AddASTCCompressedFormat(wgpu::TextureFormat::ASTC12x12UnormSrgb, ByteSize(16), Width(12), Height(12), astcFormatUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::ASTC12x12Unorm);

    // multi-planar formats
    auto multiPlanarCapabilities = device->HasFeature(Feature::MultiPlanarRenderTargets) ? Cap::Renderable : Cap::None;
    const UnsupportedReason multiPlanarFormatNv12UnsupportedReason = device->HasFeature(Feature::DawnMultiPlanarFormats) ?  Format::supported : RequiresFeature{wgpu::FeatureName::DawnMultiPlanarFormats};
    AddMultiAspectFormat(wgpu::TextureFormat::R8BG8Biplanar420Unorm, TextureSubsampling::e420, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatNv12UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm);
    const UnsupportedReason multiPlanarFormatNv16UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatNv16) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatNv16};
    AddMultiAspectFormat(wgpu::TextureFormat::R8BG8Biplanar422Unorm, TextureSubsampling::e422, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatNv16UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm);
    const UnsupportedReason multiPlanarFormatNv24UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatNv24) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatNv24};
    AddMultiAspectFormat(wgpu::TextureFormat::R8BG8Biplanar444Unorm, TextureSubsampling::e444, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatNv24UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm);
    const UnsupportedReason multiPlanarFormatP010UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatP010) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatP010};
    AddMultiAspectFormat(wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm, TextureSubsampling::e420, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatP010UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R16Unorm, wgpu::TextureFormat::RG16Unorm);
    const UnsupportedReason multiPlanarFormatP210UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatP210) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatP210};
    AddMultiAspectFormat(wgpu::TextureFormat::R10X6BG10X6Biplanar422Unorm, TextureSubsampling::e422, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatP210UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R16Unorm, wgpu::TextureFormat::RG16Unorm);
    const UnsupportedReason multiPlanarFormatP410UnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatP410) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatP410};
    AddMultiAspectFormat(wgpu::TextureFormat::R10X6BG10X6Biplanar444Unorm, TextureSubsampling::e444, Aspect::Plane0 | Aspect::Plane1,
        multiPlanarCapabilities, multiPlanarFormatP410UnsupportedReason, ComponentCount(3), wgpu::TextureFormat::R16Unorm, wgpu::TextureFormat::RG16Unorm);
    const UnsupportedReason multiPlanarFormatNv12aUnsupportedReason = device->HasFeature(Feature::MultiPlanarFormatNv12a) ?  Format::supported : RequiresFeature{wgpu::FeatureName::MultiPlanarFormatNv12a};
    AddMultiAspectFormat(wgpu::TextureFormat::R8BG8A8Triplanar420Unorm, TextureSubsampling::e420, Aspect::Plane0 | Aspect::Plane1 | Aspect::Plane2,
        multiPlanarCapabilities, multiPlanarFormatNv12aUnsupportedReason, ComponentCount(4), wgpu::TextureFormat::R8Unorm, wgpu::TextureFormat::RG8Unorm, wgpu::TextureFormat::R8Unorm);
    // clang-format on

    // This checks that each format is set at least once, the second part of checking that all
    // formats are checked exactly once. If this assertion is failing and texture formats have
    // been added or removed recently, check that kKnownFormatCount has been updated.
    DAWN_ASSERT(formatsSet.all());

    for (const Format& f : table) {
        if (f.format != f.baseFormat) {
            auto& baseViewFormat = table[ComputeFormatIndex(f.baseFormat)].baseViewFormat;
            // Currently, Dawn only supports sRGB reinterpretation, so there should only be one
            // view format.
            DAWN_ASSERT(baseViewFormat == wgpu::TextureFormat::Undefined);
            baseViewFormat = f.format;
        }
    }
    return table;
}

absl::FormatConvertResult<absl::FormatConversionCharSet::kString> AbslFormatConvert(
    const UnsupportedReason& value,
    const absl::FormatConversionSpec& spec,
    absl::FormatSink* s) {
    MatchVariant(
        value, [](const std::monostate&) { DAWN_UNREACHABLE(); },
        [s](const RequiresFeature& requiresFeature) {
            s->Append(absl::StrFormat("requires feature %s", requiresFeature.feature));
        },
        [s](const CompatibilityMode&) { s->Append("not supported in compatibility mode"); });
    return {true};
}

}  // namespace dawn::native
