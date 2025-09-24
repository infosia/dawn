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

#include "src/tint/lang/core/type/parameter_usage.h"

namespace tint::core::type {

ParamIndices LookupParameterIndices(core::BuiltinFn fn,
                                    core::type::TextureDimension dim,
                                    bool is_depth,
                                    uint32_t arg_count) {
    switch (fn) {
        case BuiltinFn::kTextureDimensions:
        case BuiltinFn::kTextureNumLayers:
        case BuiltinFn::kTextureNumLevels:
        case BuiltinFn::kTextureNumSamples:
        case BuiltinFn::kTextureStore:
        case BuiltinFn::kTextureLoad: {
            return {
                .texture = 0,
            };
        }

        case BuiltinFn::kTextureSampleBaseClampToEdge: {
            return {
                .texture = 0,
                .sampler = 1,
            };
        }

        case BuiltinFn::kTextureGather: {
            ParamIndices indices{};
            if (is_depth) {
                indices.texture = 0;
                indices.sampler = 1;
            } else {
                indices.component = 0;
                indices.texture = 1;
                indices.sampler = 2;
            }

            if (is_depth) {
                if (dim == core::type::TextureDimension::k2d && arg_count == 4) {
                    indices.offset = 3;
                } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 5) {
                    indices.offset = 4;
                }
            } else {
                if (dim == core::type::TextureDimension::k2d && arg_count == 5) {
                    indices.offset = 4;
                } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 6) {
                    indices.offset = 5;
                }
            }

            return indices;
        }

        case BuiltinFn::kTextureGatherCompare: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };

            if (is_depth) {
                if (dim == core::type::TextureDimension::k2d && arg_count == 5) {
                    indices.offset = 4;
                } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 6) {
                    indices.offset = 5;
                }
            }

            return indices;
        }

        case BuiltinFn::kTextureSample: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };

            if ((dim == core::type::TextureDimension::k2d ||
                 dim == core::type::TextureDimension::k3d) &&
                arg_count == 4) {
                indices.offset = 3;
            } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 5) {
                indices.offset = 4;
            }

            return indices;
        }

        case BuiltinFn::kTextureSampleBias: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };
            if ((dim == core::type::TextureDimension::k2d ||
                 dim == core::type::TextureDimension::k3d) &&
                arg_count == 5) {
                indices.offset = 4;
            } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 6) {
                indices.offset = 5;
            }
            return indices;
        }

        case BuiltinFn::kTextureSampleCompare:
        case BuiltinFn::kTextureSampleCompareLevel: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };
            if (dim == core::type::TextureDimension::k2d && arg_count == 5) {
                indices.offset = 4;
            } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 6) {
                indices.offset = 5;
            }
            return indices;
        }

        case BuiltinFn::kTextureSampleGrad: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };
            if ((dim == core::type::TextureDimension::k2d ||
                 dim == core::type::TextureDimension::k3d) &&
                arg_count == 6) {
                indices.offset = 5;
            } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 7) {
                indices.offset = 6;
            }
            return indices;
        }

        case BuiltinFn::kTextureSampleLevel: {
            ParamIndices indices{
                .texture = 0,
                .sampler = 1,
            };
            if ((dim == core::type::TextureDimension::k2d ||
                 dim == core::type::TextureDimension::k3d) &&
                arg_count == 5) {
                indices.offset = 4;
            } else if (dim == core::type::TextureDimension::k2dArray && arg_count == 6) {
                indices.offset = 5;
            }
            return indices;
        }
        default:
            break;
    }
    return {};
}

}  // namespace tint::core::type
