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

#ifndef SRC_TINT_API_COMMON_RESOURCE_TYPE_H_
#define SRC_TINT_API_COMMON_RESOURCE_TYPE_H_

#include "src/tint/utils/reflection.h"

namespace tint {

enum class TextureViewDimension : uint8_t { k1d, k2d, k2dArray, kCube, kCubeArray, k3d };

enum class BindingTypeBuffer : uint8_t { kUniform, kStorage };

enum class BindingTypeSampler : uint8_t { kComparison, kNonComparison };

struct BindingTypeTexture {
    enum class SampleType : uint8_t { kFloat, kSInt, kUInt, kDepth };

    bool multisampled;
    TextureViewDimension viewDimension;
    SampleType sampleType;

    TINT_REFLECT(BindingTypeTexture, multisampled, viewDimension, sampleType);
    TINT_REFLECT_HASH_CODE(BindingTypeTexture);
};

struct BindingTypeStorageTexture {
    enum class Format : uint8_t {
        kBgra8Unorm,
        kR16Float,
        kR16Sint,
        kR16Snorm,
        kR16Uint,
        kR16Unorm,
        kR32Float,
        kR32Sint,
        kR32Uint,
        kR8Sint,
        kR8Snorm,
        kR8Uint,
        kR8Unorm,
        kRg11B10Ufloat,
        kRg16Float,
        kRg16Sint,
        kRg16Snorm,
        kRg16Uint,
        kRg16Unorm,
        kRg32Float,
        kRg32Sint,
        kRg32Uint,
        kRg8Sint,
        kRg8Snorm,
        kRg8Uint,
        kRg8Unorm,
        kRgb10A2Uint,
        kRgb10A2Unorm,
        kRgba16Float,
        kRgba16Sint,
        kRgba16Snorm,
        kRgba16Uint,
        kRgba16Unorm,
        kRgba32Float,
        kRgba32Sint,
        kRgba32Uint,
        kRgba8Sint,
        kRgba8Snorm,
        kRgba8Uint,
        kRgba8Unorm,
    };
    enum class Access : uint8_t { kRead, kWrite, kReadWrite };

    TextureViewDimension viewDimension;
    Format format;
    Access access;

    TINT_REFLECT(BindingTypeStorageTexture, viewDimension, format, access);
    TINT_REFLECT_HASH_CODE(BindingTypeStorageTexture);
};

using BindingType = std::
    variant<BindingTypeBuffer, BindingTypeSampler, BindingTypeTexture, BindingTypeStorageTexture>;

enum class ResourceType : uint32_t {
    kEmpty,

    kTexture1d_f32_filterable,
    kTexture1d_f32_unfilterable,
    kTexture1d_i32,
    kTexture1d_u32,
    kTexture2d_f32_filterable,
    kTexture2d_f32_unfilterable,
    kTexture2d_i32,
    kTexture2d_u32,
    kTexture2dArray_f32_filterable,
    kTexture2dArray_f32_unfilterable,
    kTexture2dArray_i32,
    kTexture2dArray_u32,
    kTexture3d_f32_filterable,
    kTexture3d_f32_unfilterable,
    kTexture3d_i32,
    kTexture3d_u32,
    kTextureCube_f32_filterable,
    kTextureCube_f32_unfilterable,
    kTextureCube_i32,
    kTextureCube_u32,
    kTextureCubeArray_f32_filterable,
    kTextureCubeArray_f32_unfilterable,
    kTextureCubeArray_i32,
    kTextureCubeArray_u32,

    kTextureMultisampled2d_f32,
    kTextureMultisampled2d_i32,
    kTextureMultisampled2d_u32,

    kTextureDepth2d,
    kTextureDepth2dArray,
    kTextureDepthCube,
    kTextureDepthCubeArray,
    kTextureDepthMultisampled2d,

    kSampler_filtering,
    kSampler_non_filtering,
    kSampler_comparison,
};
TINT_REFLECT_ENUM_RANGE(tint::ResourceType, kEmpty, kSampler_comparison);

}  // namespace tint

#endif  // SRC_TINT_API_COMMON_RESOURCE_TYPE_H_
