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

#include "src/tint/utils/reflection/reflection.h"

namespace tint {

enum class ResourceType : uint32_t {
    kEmpty = 0,

    kTexture1d_f32_filterable = 1,
    kTexture1d_f32_unfilterable = 2,
    kTexture1d_f32_unknown_filterable = 3,
    kTexture1d_i32 = 4,
    kTexture1d_u32 = 5,
    kTexture2d_f32_filterable = 6,
    kTexture2d_f32_unfilterable = 7,
    kTexture2d_f32_unknown_filterable = 8,
    kTexture2d_i32 = 9,
    kTexture2d_u32 = 10,
    kTexture2dArray_f32_filterable = 11,
    kTexture2dArray_f32_unfilterable = 12,
    kTexture2dArray_f32_unknown_filterable = 13,
    kTexture2dArray_i32 = 14,
    kTexture2dArray_u32 = 15,
    kTexture3d_f32_filterable = 16,
    kTexture3d_f32_unfilterable = 17,
    kTexture3d_f32_unknown_filterable = 18,
    kTexture3d_i32 = 19,
    kTexture3d_u32 = 20,
    kTextureCube_f32_filterable = 21,
    kTextureCube_f32_unfilterable = 22,
    kTextureCube_f32_unknown_filterable = 23,
    kTextureCube_i32 = 24,
    kTextureCube_u32 = 25,
    kTextureCubeArray_f32_filterable = 26,
    kTextureCubeArray_f32_unfilterable = 27,
    kTextureCubeArray_f32_unknown_filterable = 28,
    kTextureCubeArray_i32 = 29,
    kTextureCubeArray_u32 = 30,

    kTextureMultisampled2d_f32 = 31,
    kTextureMultisampled2d_i32 = 32,
    kTextureMultisampled2d_u32 = 33,

    kTextureDepth2d = 34,
    kTextureDepth2dArray = 35,
    kTextureDepthCube = 36,
    kTextureDepthCubeArray = 37,
    kTextureDepthMultisampled2d = 38,

    // Uniform buffer type (we don't need to know the data type of the pointee).
    kUniformBuffer = 39,

    // Storage buffer type (we don't need to know the data type of the pointee, or the access mode).
    kStorageBuffer = 40,

    // Format-less storage texture types (we don't need to know the format).
    kTextureStorage1d_f32_read = 41,
    kTextureStorage1d_f32_read_write = 42,
    kTextureStorage1d_f32_write = 43,
    kTextureStorage1d_i32_read = 44,
    kTextureStorage1d_i32_read_write = 45,
    kTextureStorage1d_i32_write = 46,
    kTextureStorage1d_u32_read = 47,
    kTextureStorage1d_u32_read_write = 48,
    kTextureStorage1d_u32_write = 49,
    kTextureStorage2dArray_f32_read = 50,
    kTextureStorage2dArray_f32_read_write = 51,
    kTextureStorage2dArray_f32_write = 52,
    kTextureStorage2dArray_i32_read = 53,
    kTextureStorage2dArray_i32_read_write = 54,
    kTextureStorage2dArray_i32_write = 55,
    kTextureStorage2dArray_u32_read = 56,
    kTextureStorage2dArray_u32_read_write = 57,
    kTextureStorage2dArray_u32_write = 58,
    kTextureStorage2d_f32_read = 59,
    kTextureStorage2d_f32_read_write = 60,
    kTextureStorage2d_f32_write = 61,
    kTextureStorage2d_i32_read = 62,
    kTextureStorage2d_i32_read_write = 63,
    kTextureStorage2d_i32_write = 64,
    kTextureStorage2d_u32_read = 65,
    kTextureStorage2d_u32_read_write = 66,
    kTextureStorage2d_u32_write = 67,
    kTextureStorage3d_f32_read = 68,
    kTextureStorage3d_f32_read_write = 69,
    kTextureStorage3d_f32_write = 70,
    kTextureStorage3d_i32_read = 71,
    kTextureStorage3d_i32_read_write = 72,
    kTextureStorage3d_i32_write = 73,
    kTextureStorage3d_u32_read = 74,
    kTextureStorage3d_u32_read_write = 75,
    kTextureStorage3d_u32_write = 76,

    kSampler = 77,
    kSampler_filtering = 78,
    kSampler_non_filtering = 79,
    kSampler_comparison = 80,
    // NOTE: If adding more items here, be sure to update the range below.
};
TINT_REFLECT_ENUM_RANGE(tint::ResourceType, kEmpty, kSampler_comparison);

constexpr bool IsSampler(ResourceType res_type) {
    switch (res_type) {
        case ResourceType::kSampler:
        case ResourceType::kSampler_filtering:
        case ResourceType::kSampler_non_filtering:
        case ResourceType::kSampler_comparison:
            return true;

        default:
            break;
    }
    return false;
}

}  // namespace tint

#endif  // SRC_TINT_API_COMMON_RESOURCE_TYPE_H_
