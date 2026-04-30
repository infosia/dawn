// Copyright 2021 The Dawn & Tint Authors
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

#include "dawn/native/TintUtils.h"

#include "dawn/common/MatchVariant.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Device.h"
#include "dawn/native/Pipeline.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/RenderPipeline.h"
#include "tint/tint.h"

namespace dawn::native {

namespace {

TextureComponentType FormatToComponentType(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::R8Unorm:
        case wgpu::TextureFormat::R8Snorm:
        case wgpu::TextureFormat::R16Unorm:
        case wgpu::TextureFormat::R16Snorm:
        case wgpu::TextureFormat::R16Float:
        case wgpu::TextureFormat::RG8Unorm:
        case wgpu::TextureFormat::RG8Snorm:
        case wgpu::TextureFormat::R32Float:
        case wgpu::TextureFormat::RG16Unorm:
        case wgpu::TextureFormat::RG16Snorm:
        case wgpu::TextureFormat::RG16Float:
        case wgpu::TextureFormat::RGBA8Unorm:
        case wgpu::TextureFormat::RGBA8UnormSrgb:
        case wgpu::TextureFormat::RGBA8Snorm:
        case wgpu::TextureFormat::BGRA8Unorm:
        case wgpu::TextureFormat::BGRA8UnormSrgb:
        case wgpu::TextureFormat::RGB10A2Unorm:
        case wgpu::TextureFormat::RG11B10Ufloat:
        case wgpu::TextureFormat::RGB9E5Ufloat:
        case wgpu::TextureFormat::RG32Float:
        case wgpu::TextureFormat::RGBA16Unorm:
        case wgpu::TextureFormat::RGBA16Snorm:
        case wgpu::TextureFormat::RGBA16Float:
        case wgpu::TextureFormat::RGBA32Float:
        case wgpu::TextureFormat::BC1RGBAUnorm:
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
        case wgpu::TextureFormat::BC2RGBAUnorm:
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
        case wgpu::TextureFormat::BC3RGBAUnorm:
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
        case wgpu::TextureFormat::BC4RUnorm:
        case wgpu::TextureFormat::BC4RSnorm:
        case wgpu::TextureFormat::BC5RGUnorm:
        case wgpu::TextureFormat::BC5RGSnorm:
        case wgpu::TextureFormat::BC6HRGBUfloat:
        case wgpu::TextureFormat::BC6HRGBFloat:
        case wgpu::TextureFormat::BC7RGBAUnorm:
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8Unorm:
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8A1Unorm:
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
        case wgpu::TextureFormat::ETC2RGBA8Unorm:
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
        case wgpu::TextureFormat::EACR11Unorm:
        case wgpu::TextureFormat::EACR11Snorm:
        case wgpu::TextureFormat::EACRG11Unorm:
        case wgpu::TextureFormat::EACRG11Snorm:
        case wgpu::TextureFormat::ASTC4x4Unorm:
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x4Unorm:
        case wgpu::TextureFormat::ASTC5x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x5Unorm:
        case wgpu::TextureFormat::ASTC5x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x5Unorm:
        case wgpu::TextureFormat::ASTC6x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x6Unorm:
        case wgpu::TextureFormat::ASTC6x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x5Unorm:
        case wgpu::TextureFormat::ASTC8x5UnormSrgb:
        case wgpu::TextureFormat::ASTC8x6Unorm:
        case wgpu::TextureFormat::ASTC8x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x8Unorm:
        case wgpu::TextureFormat::ASTC8x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x5Unorm:
        case wgpu::TextureFormat::ASTC10x5UnormSrgb:
        case wgpu::TextureFormat::ASTC10x6Unorm:
        case wgpu::TextureFormat::ASTC10x6UnormSrgb:
        case wgpu::TextureFormat::ASTC10x8Unorm:
        case wgpu::TextureFormat::ASTC10x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x10Unorm:
        case wgpu::TextureFormat::ASTC10x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x10Unorm:
        case wgpu::TextureFormat::ASTC12x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x12Unorm:
        case wgpu::TextureFormat::ASTC12x12UnormSrgb:
            return TextureComponentType::Float;
        case wgpu::TextureFormat::R8Uint:
        case wgpu::TextureFormat::R16Uint:
        case wgpu::TextureFormat::RG8Uint:
        case wgpu::TextureFormat::R32Uint:
        case wgpu::TextureFormat::RG16Uint:
        case wgpu::TextureFormat::RGBA8Uint:
        case wgpu::TextureFormat::RGB10A2Uint:
        case wgpu::TextureFormat::RG32Uint:
        case wgpu::TextureFormat::RGBA16Uint:
        case wgpu::TextureFormat::RGBA32Uint:
            return TextureComponentType::Uint;
        case wgpu::TextureFormat::R8Sint:
        case wgpu::TextureFormat::R16Sint:
        case wgpu::TextureFormat::RG8Sint:
        case wgpu::TextureFormat::R32Sint:
        case wgpu::TextureFormat::RG16Sint:
        case wgpu::TextureFormat::RGBA8Sint:
        case wgpu::TextureFormat::RG32Sint:
        case wgpu::TextureFormat::RGBA16Sint:
        case wgpu::TextureFormat::RGBA32Sint:
            return TextureComponentType::Sint;
        case wgpu::TextureFormat::Undefined:
        case wgpu::TextureFormat::Stencil8:
        case wgpu::TextureFormat::Depth16Unorm:
        case wgpu::TextureFormat::Depth24Plus:
        case wgpu::TextureFormat::Depth24PlusStencil8:
        case wgpu::TextureFormat::Depth32Float:
        case wgpu::TextureFormat::Depth32FloatStencil8:
        case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
        case wgpu::TextureFormat::R8BG8A8Triplanar420Unorm:
        case wgpu::TextureFormat::R8BG8Biplanar422Unorm:
        case wgpu::TextureFormat::R8BG8Biplanar444Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar422Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar444Unorm:
        case wgpu::TextureFormat::OpaqueYCbCrAndroid:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::VertexFormat ToTintVertexFormat(wgpu::VertexFormat format) {
    switch (format) {
        case wgpu::VertexFormat::Uint8:
            return tint::VertexFormat::kUint8;
        case wgpu::VertexFormat::Uint8x2:
            return tint::VertexFormat::kUint8x2;
        case wgpu::VertexFormat::Uint8x4:
            return tint::VertexFormat::kUint8x4;
        case wgpu::VertexFormat::Sint8:
            return tint::VertexFormat::kSint8;
        case wgpu::VertexFormat::Sint8x2:
            return tint::VertexFormat::kSint8x2;
        case wgpu::VertexFormat::Sint8x4:
            return tint::VertexFormat::kSint8x4;
        case wgpu::VertexFormat::Unorm8:
            return tint::VertexFormat::kUnorm8;
        case wgpu::VertexFormat::Unorm8x2:
            return tint::VertexFormat::kUnorm8x2;
        case wgpu::VertexFormat::Unorm8x4:
            return tint::VertexFormat::kUnorm8x4;
        case wgpu::VertexFormat::Snorm8:
            return tint::VertexFormat::kSnorm8;
        case wgpu::VertexFormat::Snorm8x2:
            return tint::VertexFormat::kSnorm8x2;
        case wgpu::VertexFormat::Snorm8x4:
            return tint::VertexFormat::kSnorm8x4;
        case wgpu::VertexFormat::Uint16:
            return tint::VertexFormat::kUint16;
        case wgpu::VertexFormat::Uint16x2:
            return tint::VertexFormat::kUint16x2;
        case wgpu::VertexFormat::Uint16x4:
            return tint::VertexFormat::kUint16x4;
        case wgpu::VertexFormat::Sint16:
            return tint::VertexFormat::kSint16;
        case wgpu::VertexFormat::Sint16x2:
            return tint::VertexFormat::kSint16x2;
        case wgpu::VertexFormat::Sint16x4:
            return tint::VertexFormat::kSint16x4;
        case wgpu::VertexFormat::Unorm16:
            return tint::VertexFormat::kUnorm16;
        case wgpu::VertexFormat::Unorm16x2:
            return tint::VertexFormat::kUnorm16x2;
        case wgpu::VertexFormat::Unorm16x4:
            return tint::VertexFormat::kUnorm16x4;
        case wgpu::VertexFormat::Snorm16:
            return tint::VertexFormat::kSnorm16;
        case wgpu::VertexFormat::Snorm16x2:
            return tint::VertexFormat::kSnorm16x2;
        case wgpu::VertexFormat::Snorm16x4:
            return tint::VertexFormat::kSnorm16x4;
        case wgpu::VertexFormat::Float16:
            return tint::VertexFormat::kFloat16;
        case wgpu::VertexFormat::Float16x2:
            return tint::VertexFormat::kFloat16x2;
        case wgpu::VertexFormat::Float16x4:
            return tint::VertexFormat::kFloat16x4;
        case wgpu::VertexFormat::Float32:
            return tint::VertexFormat::kFloat32;
        case wgpu::VertexFormat::Float32x2:
            return tint::VertexFormat::kFloat32x2;
        case wgpu::VertexFormat::Float32x3:
            return tint::VertexFormat::kFloat32x3;
        case wgpu::VertexFormat::Float32x4:
            return tint::VertexFormat::kFloat32x4;
        case wgpu::VertexFormat::Uint32:
            return tint::VertexFormat::kUint32;
        case wgpu::VertexFormat::Uint32x2:
            return tint::VertexFormat::kUint32x2;
        case wgpu::VertexFormat::Uint32x3:
            return tint::VertexFormat::kUint32x3;
        case wgpu::VertexFormat::Uint32x4:
            return tint::VertexFormat::kUint32x4;
        case wgpu::VertexFormat::Sint32:
            return tint::VertexFormat::kSint32;
        case wgpu::VertexFormat::Sint32x2:
            return tint::VertexFormat::kSint32x2;
        case wgpu::VertexFormat::Sint32x3:
            return tint::VertexFormat::kSint32x3;
        case wgpu::VertexFormat::Sint32x4:
            return tint::VertexFormat::kSint32x4;
        case wgpu::VertexFormat::Unorm10_10_10_2:
            return tint::VertexFormat::kUnorm10_10_10_2;
        case wgpu::VertexFormat::Unorm8x4BGRA:
            return tint::VertexFormat::kUnorm8x4BGRA;
    }
    DAWN_UNREACHABLE();
}

tint::VertexStepMode ToTintVertexStepMode(wgpu::VertexStepMode mode) {
    switch (mode) {
        case wgpu::VertexStepMode::Vertex:
            return tint::VertexStepMode::kVertex;
        case wgpu::VertexStepMode::Instance:
            return tint::VertexStepMode::kInstance;
        case wgpu::VertexStepMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

}  // namespace

tint::VertexPullingConfig BuildVertexPullingTransformConfig(
    const RenderPipelineBase& renderPipeline,
    BindGroupIndex pullingBufferBindingSet) {
    tint::VertexPullingConfig cfg;
    cfg.pulling_group = uint32_t(pullingBufferBindingSet);

    cfg.vertex_state.resize(renderPipeline.GetVertexBufferCount());
    for (VertexBufferSlot slot : renderPipeline.GetVertexBuffersUsed()) {
        const VertexBufferInfo& dawnInfo = renderPipeline.GetVertexBuffer(slot);
        tint::VertexBufferLayoutDescriptor* tintInfo =
            &cfg.vertex_state[static_cast<uint8_t>(slot)];

        tintInfo->array_stride = uint32_t(dawnInfo.arrayStride);
        tintInfo->step_mode = ToTintVertexStepMode(dawnInfo.stepMode);
    }

    for (VertexAttributeLocation location : renderPipeline.GetAttributeLocationsUsed()) {
        const VertexAttributeInfo& dawnInfo = renderPipeline.GetAttribute(location);
        tint::VertexAttributeDescriptor tintInfo;
        tintInfo.format = ToTintVertexFormat(dawnInfo.format);
        tintInfo.offset = uint32_t(dawnInfo.offset);
        tintInfo.shader_location = uint32_t(static_cast<uint8_t>(location));

        uint8_t vertexBufferSlot = static_cast<uint8_t>(dawnInfo.vertexBufferSlot);
        cfg.vertex_state[vertexBufferSlot].attributes.push_back(tintInfo);
    }
    return cfg;
}

std::unordered_map<tint::OverrideId, double> BuildSubstituteOverridesTransformConfig(
    const ProgrammableStage& stage) {
    return BuildSubstituteOverridesTransformConfig(*stage.metadata, stage.constants);
}

std::unordered_map<tint::OverrideId, double> BuildSubstituteOverridesTransformConfig(
    const EntryPointMetadata& metadata,
    const PipelineConstantEntries& constants) {
    std::unordered_map<tint::OverrideId, double> map;
    for (const auto& [key, value] : constants) {
        const auto& o = metadata.overrides.at(key);
        map.insert({{o.id.value}, value});
    }
    return map;
}

tint::ResourceType BindingLayoutToResourceType(const BindingInfo& bi) {
    return MatchVariant(
        bi.bindingLayout,
        [&](const BufferBindingInfo& bindingInfo) {
            switch (bindingInfo.type) {
                case wgpu::BufferBindingType::Uniform:
                    return tint::ResourceType::kUniformBuffer;
                case wgpu::BufferBindingType::Storage:
                case wgpu::BufferBindingType::ReadOnlyStorage:
                case kInternalStorageBufferBinding:
                case kInternalReadOnlyStorageBufferBinding:
                    return tint::ResourceType::kStorageBuffer;
                case wgpu::BufferBindingType::BindingNotUsed:
                case wgpu::BufferBindingType::Undefined:
                    break;
            }
            DAWN_UNREACHABLE();
        },
        [&](const SamplerBindingInfo& bindingInfo) {
            switch (bindingInfo.type) {
                case wgpu::SamplerBindingType::BindingNotUsed:
                case wgpu::SamplerBindingType::Undefined:
                case kUnknownFilteringSamplerBindingType:
                    break;

                case wgpu::SamplerBindingType::Filtering:
                    return tint::ResourceType::kSampler_filtering;
                case wgpu::SamplerBindingType::NonFiltering:
                    return tint::ResourceType::kSampler_non_filtering;
                case wgpu::SamplerBindingType::Comparison:
                    return tint::ResourceType::kSampler_comparison;
            }
            DAWN_UNREACHABLE();
        },
        [&](const TextureBindingInfo& bindingInfo) {
            switch (bindingInfo.viewDimension) {
                case wgpu::TextureViewDimension::Undefined:
                    break;
                case wgpu::TextureViewDimension::e1D: {
                    switch (bindingInfo.sampleType) {
                        case wgpu::TextureSampleType::BindingNotUsed:
                        case wgpu::TextureSampleType::Undefined:
                        case wgpu::TextureSampleType::Depth:
                        case kUnknownFilterableFloatSampleType:
                        case kInternalResolveAttachmentSampleType:
                            break;
                        case wgpu::TextureSampleType::Float:
                            return tint::ResourceType::kTexture1d_f32_filterable;
                        case wgpu::TextureSampleType::UnfilterableFloat:
                            return tint::ResourceType::kTexture1d_f32_unfilterable;
                        case wgpu::TextureSampleType::Sint:
                            return tint::ResourceType::kTexture1d_i32;
                        case wgpu::TextureSampleType::Uint:
                            return tint::ResourceType::kTexture1d_u32;
                    }
                    DAWN_UNREACHABLE();
                }
                case wgpu::TextureViewDimension::e2D: {
                    if (bindingInfo.multisampled) {
                        switch (bindingInfo.sampleType) {
                            case wgpu::TextureSampleType::BindingNotUsed:
                            case wgpu::TextureSampleType::Undefined:
                            case wgpu::TextureSampleType::Float:
                            case kUnknownFilterableFloatSampleType:
                            case kInternalResolveAttachmentSampleType:
                                break;
                            case wgpu::TextureSampleType::UnfilterableFloat:
                                return tint::ResourceType::kTextureMultisampled2d_f32;
                            case wgpu::TextureSampleType::Sint:
                                return tint::ResourceType::kTextureMultisampled2d_i32;
                            case wgpu::TextureSampleType::Uint:
                                return tint::ResourceType::kTextureMultisampled2d_u32;
                            case wgpu::TextureSampleType::Depth:
                                return tint::ResourceType::kTextureDepthMultisampled2d;
                        }
                    } else {
                        switch (bindingInfo.sampleType) {
                            case wgpu::TextureSampleType::BindingNotUsed:
                            case wgpu::TextureSampleType::Undefined:
                            case kUnknownFilterableFloatSampleType:
                            case kInternalResolveAttachmentSampleType:
                                break;
                            case wgpu::TextureSampleType::Float:
                                return tint::ResourceType::kTexture2d_f32_filterable;
                            case wgpu::TextureSampleType::UnfilterableFloat:
                                return tint::ResourceType::kTexture2d_f32_unfilterable;
                            case wgpu::TextureSampleType::Sint:
                                return tint::ResourceType::kTexture2d_i32;
                            case wgpu::TextureSampleType::Uint:
                                return tint::ResourceType::kTexture2d_u32;
                            case wgpu::TextureSampleType::Depth:
                                return tint::ResourceType::kTextureDepth2d;
                        }
                    }
                    DAWN_UNREACHABLE();
                }
                case wgpu::TextureViewDimension::e2DArray: {
                    switch (bindingInfo.sampleType) {
                        case wgpu::TextureSampleType::BindingNotUsed:
                        case wgpu::TextureSampleType::Undefined:
                        case kUnknownFilterableFloatSampleType:
                        case kInternalResolveAttachmentSampleType:
                            break;
                        case wgpu::TextureSampleType::Float:
                            return tint::ResourceType::kTexture2dArray_f32_filterable;
                        case wgpu::TextureSampleType::UnfilterableFloat:
                            return tint::ResourceType::kTexture2dArray_f32_unfilterable;
                        case wgpu::TextureSampleType::Sint:
                            return tint::ResourceType::kTexture2dArray_i32;
                        case wgpu::TextureSampleType::Uint:
                            return tint::ResourceType::kTexture2dArray_u32;
                        case wgpu::TextureSampleType::Depth:
                            return tint::ResourceType::kTextureDepth2dArray;
                    }
                    DAWN_UNREACHABLE();
                }
                case wgpu::TextureViewDimension::Cube: {
                    switch (bindingInfo.sampleType) {
                        case wgpu::TextureSampleType::BindingNotUsed:
                        case wgpu::TextureSampleType::Undefined:
                        case kUnknownFilterableFloatSampleType:
                        case kInternalResolveAttachmentSampleType:
                            break;
                        case wgpu::TextureSampleType::Float:
                            return tint::ResourceType::kTextureCube_f32_filterable;
                        case wgpu::TextureSampleType::UnfilterableFloat:
                            return tint::ResourceType::kTextureCube_f32_unfilterable;
                        case wgpu::TextureSampleType::Sint:
                            return tint::ResourceType::kTextureCube_i32;
                        case wgpu::TextureSampleType::Uint:
                            return tint::ResourceType::kTextureCube_u32;
                        case wgpu::TextureSampleType::Depth:
                            return tint::ResourceType::kTextureDepthCube;
                    }
                    DAWN_UNREACHABLE();
                }
                case wgpu::TextureViewDimension::CubeArray: {
                    switch (bindingInfo.sampleType) {
                        case wgpu::TextureSampleType::BindingNotUsed:
                        case wgpu::TextureSampleType::Undefined:
                        case kUnknownFilterableFloatSampleType:
                        case kInternalResolveAttachmentSampleType:
                            break;
                        case wgpu::TextureSampleType::Float:
                            return tint::ResourceType::kTextureCubeArray_f32_filterable;
                        case wgpu::TextureSampleType::UnfilterableFloat:
                            return tint::ResourceType::kTextureCubeArray_f32_unfilterable;
                        case wgpu::TextureSampleType::Sint:
                            return tint::ResourceType::kTextureCubeArray_i32;
                        case wgpu::TextureSampleType::Uint:
                            return tint::ResourceType::kTextureCubeArray_u32;
                        case wgpu::TextureSampleType::Depth:
                            return tint::ResourceType::kTextureDepthCubeArray;
                    }
                    DAWN_UNREACHABLE();
                }
                case wgpu::TextureViewDimension::e3D: {
                    switch (bindingInfo.sampleType) {
                        case wgpu::TextureSampleType::BindingNotUsed:
                        case wgpu::TextureSampleType::Undefined:
                        case wgpu::TextureSampleType::Depth:
                        case kUnknownFilterableFloatSampleType:
                        case kInternalResolveAttachmentSampleType:
                            break;
                        case wgpu::TextureSampleType::Float:
                            return tint::ResourceType::kTexture3d_f32_filterable;
                        case wgpu::TextureSampleType::UnfilterableFloat:
                            return tint::ResourceType::kTexture3d_f32_unfilterable;
                        case wgpu::TextureSampleType::Sint:
                            return tint::ResourceType::kTexture3d_i32;
                        case wgpu::TextureSampleType::Uint:
                            return tint::ResourceType::kTexture3d_u32;
                    }
                    DAWN_UNREACHABLE();
                }
            }
            DAWN_UNREACHABLE();
        },
        [&](const StorageTextureBindingInfo& bindingInfo) {
            switch (bindingInfo.viewDimension) {
                case wgpu::TextureViewDimension::e1D:
                    switch (FormatToComponentType(bindingInfo.format)) {
                        case TextureComponentType::Float:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage1d_f32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage1d_f32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage1d_f32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Uint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage1d_u32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage1d_u32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage1d_u32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Sint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage1d_i32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage1d_i32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage1d_i32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                    }
                    DAWN_UNREACHABLE();
                case wgpu::TextureViewDimension::e2D:
                    switch (FormatToComponentType(bindingInfo.format)) {
                        case TextureComponentType::Float:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2d_f32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2d_f32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage2d_f32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Uint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2d_u32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2d_u32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage2d_u32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Sint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2d_i32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2d_i32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage2d_i32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                    }
                    DAWN_UNREACHABLE();
                case wgpu::TextureViewDimension::e2DArray:
                    switch (FormatToComponentType(bindingInfo.format)) {
                        case TextureComponentType::Float:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_f32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_f32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::
                                        kTextureStorage2dArray_f32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Uint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_u32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_u32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::
                                        kTextureStorage2dArray_u32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Sint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_i32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage2dArray_i32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::
                                        kTextureStorage2dArray_i32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                    }
                    DAWN_UNREACHABLE();
                case wgpu::TextureViewDimension::e3D:
                    switch (FormatToComponentType(bindingInfo.format)) {
                        case TextureComponentType::Float:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage3d_f32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage3d_f32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage3d_f32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Uint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage3d_u32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage3d_u32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage3d_u32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                        case TextureComponentType::Sint:
                            switch (bindingInfo.access) {
                                case wgpu::StorageTextureAccess::ReadOnly:
                                    return tint::ResourceType::kTextureStorage3d_i32_read;
                                case wgpu::StorageTextureAccess::WriteOnly:
                                    return tint::ResourceType::kTextureStorage3d_i32_write;
                                case wgpu::StorageTextureAccess::ReadWrite:
                                    return tint::ResourceType::kTextureStorage3d_i32_read_write;
                                case wgpu::StorageTextureAccess::BindingNotUsed:
                                case wgpu::StorageTextureAccess::Undefined:
                                    break;
                            }
                            DAWN_UNREACHABLE();
                    }
                    DAWN_UNREACHABLE();
                case wgpu::TextureViewDimension::Undefined:
                case wgpu::TextureViewDimension::Cube:
                case wgpu::TextureViewDimension::CubeArray:
                case kInternalInputAttachmentDim:
                    break;
            }
            DAWN_UNREACHABLE();
        },
        [&](const TexelBufferBindingInfo&) {
            // TODO(363031535): Return correct resource type for argument buffers.
            return tint::ResourceType::kEmpty;
        },
        [&](const ExternalTextureBindingInfo&) {
            // TODO(363031535): Return correct resource type for argument buffers.
            return tint::ResourceType::kEmpty;
        },
        [&](const StaticSamplerBindingInfo&) {
            // TODO(363031535): Return correct resource type for argument buffers.
            return tint::ResourceType::kEmpty;
        },
        [&](const InputAttachmentBindingInfo&) {
            // TODO(363031535): Return correct resource type for argument buffers.
            return tint::ResourceType::kEmpty;
        });
}

}  // namespace dawn::native
