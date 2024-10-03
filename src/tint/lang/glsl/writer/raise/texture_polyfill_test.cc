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

#include "src/tint/lang/glsl/writer/raise/texture_polyfill.h"

#include <vector>

#include "gtest/gtest.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/core/type/depth_multisampled_texture.h"
#include "src/tint/lang/core/type/depth_texture.h"
#include "src/tint/lang/core/type/multisampled_texture.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/storage_texture.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::glsl::writer::raise {
namespace {

using GlslWriter_TexturePolyfillTest = core::ir::transform::TransformTest;

TEST_F(GlslWriter_TexturePolyfillTest, TextureDimensions_1d) {
    auto* var =
        b.Var("v", handle,
              ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k1d, ty.f32()),
              core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] {
        auto* result = b.Call<u32>(core::BuiltinFn::kTextureDimensions, b.Load(var));
        b.Return(func, result);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_1d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func():u32 {
  $B2: {
    %3:texture_1d<f32> = load %v
    %4:u32 = textureDimensions %3
    ret %4
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d<f32>, read_write> = var
}

%foo = func():u32 {
  $B2: {
    %3:texture_2d<f32> = load %v
    %4:vec2<i32> = glsl.textureSize %3, 0i
    %5:vec2<u32> = bitcast %4
    %6:u32 = swizzle %5, x
    ret %6
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureDimensions_2d_WithoutLod) {
    auto* var =
        b.Var("v", handle,
              ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32()),
              core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.vec2<u32>());
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, b.Load(var));
        b.Return(func, result);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func():vec2<u32> {
  $B2: {
    %3:texture_2d<f32> = load %v
    %4:vec2<u32> = textureDimensions %3
    ret %4
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d<f32>, read_write> = var
}

%foo = func():vec2<u32> {
  $B2: {
    %3:texture_2d<f32> = load %v
    %4:vec2<i32> = glsl.textureSize %3, 0i
    %5:vec2<u32> = bitcast %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureDimensions_2d_WithU32Lod) {
    auto* var =
        b.Var("v", handle,
              ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32()),
              core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.vec2<u32>());
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, b.Load(var), 3_u);
        b.Return(func, result);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func():vec2<u32> {
  $B2: {
    %3:texture_2d<f32> = load %v
    %4:vec2<u32> = textureDimensions %3, 3u
    ret %4
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d<f32>, read_write> = var
}

%foo = func():vec2<u32> {
  $B3: {
    %3:texture_2d<f32> = load %v
    %4:i32 = bitcast 3u
    %5:vec2<i32> = glsl.textureSize %3, %4
    %6:vec2<u32> = bitcast %5
    ret %6
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureDimensions_2dArray) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2dArray, ty.f32()));
    auto* func = b.Function("foo", ty.vec2<u32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, t);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_2d_array<f32>):vec2<u32> {
  $B1: {
    %3:vec2<u32> = textureDimensions %t
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_2d_array<f32>):vec2<u32> {
  $B1: {
    %3:vec3<i32> = glsl.textureSize %t, 0i
    %4:vec2<i32> = swizzle %3, xy
    %5:vec2<u32> = bitcast %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureDimensions_Storage2D) {
    auto* t = b.FunctionParam(
        "t",
        ty.Get<core::type::StorageTexture>(
            core::type::TextureDimension::k2d, core::TexelFormat::kRg32Float, core::Access::kRead,
            core::type::StorageTexture::SubtypeFor(core::TexelFormat::kRg32Float, ty)));
    auto* func = b.Function("foo", ty.vec2<u32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, t);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_storage_2d<rg32float, read>):vec2<u32> {
  $B1: {
    %3:vec2<u32> = textureDimensions %t
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_storage_2d<rg32float, read>):vec2<u32> {
  $B1: {
    %3:vec2<i32> = glsl.imageSize %t
    %4:vec2<u32> = bitcast %3
    ret %4
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureDimensions_DepthMultisampled) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::DepthMultisampledTexture>(core::type::TextureDimension::k2d));
    auto* func = b.Function("foo", ty.vec2<u32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, t);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_depth_multisampled_2d):vec2<u32> {
  $B1: {
    %3:vec2<u32> = textureDimensions %t
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_depth_multisampled_2d):vec2<u32> {
  $B1: {
    %3:vec2<i32> = glsl.textureSize %t
    %4:vec2<u32> = bitcast %3
    ret %4
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureNumLayers_2DArray) {
    auto* var =
        b.Var("v", handle,
              ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2dArray, ty.f32()),
              core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.u32(), core::BuiltinFn::kTextureNumLayers, b.Load(var)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d_array<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_2d_array<f32> = load %v
    %4:u32 = textureNumLayers %3
    %x:u32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_2d_array<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_2d_array<f32> = load %v
    %4:vec3<i32> = glsl.textureSize %3, 0i
    %5:i32 = swizzle %4, z
    %6:u32 = bitcast %5
    %x:u32 = let %6
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureNumLayers_Depth2DArray) {
    auto* var =
        b.Var("v", handle, ty.Get<core::type::DepthTexture>(core::type::TextureDimension::k2dArray),
              core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.u32(), core::BuiltinFn::kTextureNumLayers, b.Load(var)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_depth_2d_array, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_depth_2d_array = load %v
    %4:u32 = textureNumLayers %3
    %x:u32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_depth_2d_array, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_depth_2d_array = load %v
    %4:vec3<i32> = glsl.textureSize %3, 0i
    %5:i32 = swizzle %4, z
    %6:u32 = bitcast %5
    %x:u32 = let %6
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureNumLayers_CubeArray) {
    auto* var = b.Var(
        "v", handle,
        ty.Get<core::type::SampledTexture>(core::type::TextureDimension::kCubeArray, ty.f32()),
        core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.u32(), core::BuiltinFn::kTextureNumLayers, b.Load(var)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_cube_array<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_cube_array<f32> = load %v
    %4:u32 = textureNumLayers %3
    %x:u32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_cube_array<f32>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_cube_array<f32> = load %v
    %4:vec3<i32> = glsl.textureSize %3, 0i
    %5:i32 = swizzle %4, z
    %6:u32 = bitcast %5
    %x:u32 = let %6
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureNumLayers_DepthCubeArray) {
    auto* var = b.Var("v", handle,
                      ty.Get<core::type::DepthTexture>(core::type::TextureDimension::kCubeArray),
                      core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.u32(), core::BuiltinFn::kTextureNumLayers, b.Load(var)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_depth_cube_array, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_depth_cube_array = load %v
    %4:u32 = textureNumLayers %3
    %x:u32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_depth_cube_array, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_depth_cube_array = load %v
    %4:vec3<i32> = glsl.textureSize %3, 0i
    %5:i32 = swizzle %4, z
    %6:u32 = bitcast %5
    %x:u32 = let %6
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureNumLayers_Storage2DArray) {
    auto* storage_ty = ty.Get<core::type::StorageTexture>(
        core::type::TextureDimension::k2dArray, core::TexelFormat::kRg32Float, core::Access::kRead,
        core::type::StorageTexture::SubtypeFor(core::TexelFormat::kRg32Float, ty));

    auto* var = b.Var("v", handle, storage_ty, core::Access::kReadWrite);
    var->SetBindingPoint(0, 0);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        b.Let("x", b.Call(ty.u32(), core::BuiltinFn::kTextureNumLayers, b.Load(var)));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<handle, texture_storage_2d_array<rg32float, read>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_2d_array<rg32float, read> = load %v
    %4:u32 = textureNumLayers %3
    %x:u32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<handle, texture_storage_2d_array<rg32float, read>, read_write> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_2d_array<rg32float, read> = load %v
    %4:vec3<i32> = glsl.imageSize %3
    %5:i32 = swizzle %4, z
    %6:u32 = bitcast %5
    %x:u32 = let %6
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureLoad_1DF32) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k1d, ty.f32()));
    auto* func = b.Function("foo", ty.vec4<f32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* coords = b.Zero<i32>();
        auto* level = b.Zero<u32>();
        auto* result = b.Call<vec4<f32>>(core::BuiltinFn::kTextureLoad, t, coords, level);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_1d<f32>):vec4<f32> {
  $B1: {
    %3:vec4<f32> = textureLoad %t, 0i, 0u
    ret %3
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_2d<f32>):vec4<f32> {
  $B1: {
    %3:vec2<i32> = construct 0i, 0i
    %4:vec2<i32> = convert %3
    %5:i32 = convert 0u
    %6:vec4<f32> = glsl.texelFetch %t, %4, %5
    ret %6
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureLoad_2DLevelI32) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.i32()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* coords = b.Zero<vec2<i32>>();
        auto* level = b.Zero<i32>();
        auto* result = b.Call<vec4<i32>>(core::BuiltinFn::kTextureLoad, t, coords, level);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_2d<i32>):vec4<i32> {
  $B1: {
    %3:vec4<i32> = textureLoad %t, vec2<i32>(0i), 0i
    ret %3
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_2d<i32>):vec4<i32> {
  $B1: {
    %3:vec2<i32> = convert vec2<i32>(0i)
    %4:i32 = convert 0i
    %5:vec4<i32> = glsl.texelFetch %t, %3, %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureLoad_3DLevelU32) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k3d, ty.f32()));
    auto* func = b.Function("foo", ty.vec4<f32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* coords = b.Zero<vec3<i32>>();
        auto* level = b.Zero<u32>();
        auto* result = b.Call<vec4<f32>>(core::BuiltinFn::kTextureLoad, t, coords, level);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_3d<f32>):vec4<f32> {
  $B1: {
    %3:vec4<f32> = textureLoad %t, vec3<i32>(0i), 0u
    ret %3
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_3d<f32>):vec4<f32> {
  $B1: {
    %3:vec3<i32> = convert vec3<i32>(0i)
    %4:i32 = convert 0u
    %5:vec4<f32> = glsl.texelFetch %t, %3, %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureLoad_Multisampled2DI32) {
    auto* t = b.FunctionParam(
        "t", ty.Get<core::type::MultisampledTexture>(core::type::TextureDimension::k2d, ty.i32()));
    auto* func = b.Function("foo", ty.vec4<i32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* coords = b.Zero<vec2<i32>>();
        auto* sample_idx = b.Zero<u32>();
        auto* result = b.Call<vec4<i32>>(core::BuiltinFn::kTextureLoad, t, coords, sample_idx);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_multisampled_2d<i32>):vec4<i32> {
  $B1: {
    %3:vec4<i32> = textureLoad %t, vec2<i32>(0i), 0u
    ret %3
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_multisampled_2d<i32>):vec4<i32> {
  $B1: {
    %3:vec2<i32> = convert vec2<i32>(0i)
    %4:i32 = convert 0u
    %5:vec4<i32> = glsl.texelFetch %t, %3, %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_TextureLoad_Storage2D) {
    auto* t = b.FunctionParam(
        "t",
        ty.Get<core::type::StorageTexture>(
            core::type::TextureDimension::k2d, core::TexelFormat::kRg32Float, core::Access::kRead,
            core::type::StorageTexture::SubtypeFor(core::TexelFormat::kRg32Float, ty)));
    auto* func = b.Function("foo", ty.vec4<f32>());
    func->SetParams({t});
    b.Append(func->Block(), [&] {
        auto* coords = b.Zero<vec2<i32>>();
        auto* result = b.Call<vec4<f32>>(core::BuiltinFn::kTextureLoad, t, coords);
        b.Return(func, result);
    });

    auto* src = R"(
%foo = func(%t:texture_storage_2d<rg32float, read>):vec4<f32> {
  $B1: {
    %3:vec4<f32> = textureLoad %t, vec2<i32>(0i)
    ret %3
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
%foo = func(%t:texture_storage_2d<rg32float, read>):vec4<f32> {
  $B1: {
    %3:vec2<i32> = convert vec2<i32>(0i)
    %4:vec4<f32> = glsl.imageLoad %t, %3
    ret %4
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureStore1D) {
    auto* t = b.Var(ty.ptr(
        handle, ty.Get<core::type::StorageTexture>(
                    core::type::TextureDimension::k1d, core::TexelFormat::kR32Float,
                    core::Access::kReadWrite,
                    core::type::StorageTexture::SubtypeFor(core::TexelFormat::kR32Float, ty))));
    t->SetBindingPoint(0, 0);
    b.ir.root_block->Append(t);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* coords = b.Value(1_i);
        auto* value = b.Composite(ty.vec4<f32>(), .5_f, 0_f, 0_f, 1_f);
        b.Call(ty.void_(), core::BuiltinFn::kTextureStore, b.Load(t), coords, value);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_1d<r32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_1d<r32float, read_write> = load %1
    %4:void = textureStore %3, 1i, vec4<f32>(0.5f, 0.0f, 0.0f, 1.0f)
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_2d<r32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_2d<r32float, read_write> = load %1
    %4:vec2<i32> = construct 1i, 0i
    %5:void = glsl.imageStore %3, %4, vec4<f32>(0.5f, 0.0f, 0.0f, 1.0f)
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureStore3D) {
    auto* t = b.Var(ty.ptr(
        handle, ty.Get<core::type::StorageTexture>(
                    core::type::TextureDimension::k3d, core::TexelFormat::kR32Float,
                    core::Access::kReadWrite,
                    core::type::StorageTexture::SubtypeFor(core::TexelFormat::kR32Float, ty))));
    t->SetBindingPoint(0, 0);
    b.ir.root_block->Append(t);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* coords = b.Composite(ty.vec3<i32>(), 1_i, 2_i, 3_i);
        auto* value = b.Composite(ty.vec4<f32>(), .5_f, 0_f, 0_f, 1_f);
        b.Call(ty.void_(), core::BuiltinFn::kTextureStore, b.Load(t), coords, value);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_3d<r32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_3d<r32float, read_write> = load %1
    %4:void = textureStore %3, vec3<i32>(1i, 2i, 3i), vec4<f32>(0.5f, 0.0f, 0.0f, 1.0f)
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_3d<r32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_3d<r32float, read_write> = load %1
    %4:void = glsl.imageStore %3, vec3<i32>(1i, 2i, 3i), vec4<f32>(0.5f, 0.0f, 0.0f, 1.0f)
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, TextureStoreArray) {
    auto* t = b.Var(ty.ptr(
        handle, ty.Get<core::type::StorageTexture>(
                    core::type::TextureDimension::k2dArray, core::TexelFormat::kRgba32Float,
                    core::Access::kReadWrite,
                    core::type::StorageTexture::SubtypeFor(core::TexelFormat::kR32Float, ty))));
    t->SetBindingPoint(0, 0);
    b.ir.root_block->Append(t);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(func->Block(), [&] {
        auto* coords = b.Composite(ty.vec2<i32>(), 1_i, 2_i);
        auto* value = b.Composite(ty.vec4<f32>(), .5_f, .4_f, .3_f, 1_f);
        b.Call(ty.void_(), core::BuiltinFn::kTextureStore, b.Load(t), coords, 3_u, value);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_2d_array<rgba32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_2d_array<rgba32float, read_write> = load %1
    %4:void = textureStore %3, vec2<i32>(1i, 2i), 3u, vec4<f32>(0.5f, 0.40000000596046447754f, 0.30000001192092895508f, 1.0f)
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %1:ptr<handle, texture_storage_2d_array<rgba32float, read_write>, read> = var @binding_point(0, 0)
}

%foo = @fragment func():void {
  $B2: {
    %3:texture_storage_2d_array<rgba32float, read_write> = load %1
    %4:i32 = convert 3u
    %5:vec3<i32> = construct vec2<i32>(1i, 2i), %4
    %6:void = glsl.imageStore %3, %5, vec4<f32>(0.5f, 0.40000000596046447754f, 0.30000001192092895508f, 1.0f)
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, CombineSamplers_GlobalTextureNoSampler) {
    auto* t = b.Var(ty.ptr(
        handle, ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32()),
        read_write));
    t->SetBindingPoint(0, 0);
    b.ir.root_block->Append(t);

    auto* func = b.Function("foo", ty.vec2<u32>());
    b.Append(func->Block(), [&] {
        auto* tex = b.Load(t);
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, tex);
        b.Return(func, result);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func():vec2<u32> {
  $B2: {
    %3:texture_2d<f32> = load %1
    %4:vec2<u32> = textureDimensions %3
    ret %4
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %new_tex:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func():vec2<u32> {
  $B2: {
    %3:texture_2d<f32> = load %new_tex
    %4:vec2<i32> = glsl.textureSize %3, 0i
    %5:vec2<u32> = bitcast %4
    ret %5
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    cfg.placeholder_sampler_bind_point = {2, 2};

    binding::CombinedTextureSamplerPair pair;
    pair.texture = {0, 0};
    pair.sampler = {2, 2};
    cfg.sampler_texture_to_name[pair] = "my_tex";

    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

TEST_F(GlslWriter_TexturePolyfillTest, DISABLED_CombineSamplers_LocalTextureNoSampler) {
    auto* t = b.Var(ty.ptr(
        handle, ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32()),
        read_write));
    t->SetBindingPoint(0, 0);
    b.ir.root_block->Append(t);

    auto* p = b.FunctionParam(
        "p", ty.Get<core::type::SampledTexture>(core::type::TextureDimension::k2d, ty.f32()));
    auto* func = b.Function("foo", ty.vec2<u32>());
    func->SetParams({p});
    b.Append(func->Block(), [&] {
        auto* result = b.Call<vec2<u32>>(core::BuiltinFn::kTextureDimensions, p);
        b.Return(func, result);
    });

    auto* main = b.Function("main", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    main->SetWorkgroupSize(1, 1, 1);
    b.Append(main->Block(), [&] {
        auto* tex = b.Load(t);
        b.Let("a", b.Call(ty.vec2<u32>(), func, tex));
        b.Return(main);
    });

    auto* src = R"(
$B1: {  # root
  %1:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func(p:texture_2d<f32>):vec2<u32> {
  $B2: {
    %4:vec2<u32> = textureDimensions %p
    ret %4
  }
}
%main = @compute @workgroup_size(1, 1, 1) func():void {
  $B3: {
    %6:texture_2d<f32> = load %1
    %7:vec2<u32> = call %foo, %6
    %a:vec2<u32> = let %7
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %my_tex:ptr<handle, texture_2d<f32>, read_write> = var @binding_point(0, 0)
}

%foo = func(p:texture_2d<f32>):vec2<u32> {
  $B2: {
    %4:vec2<i32> = glsl.textureSize %p, 0i
    %5:vec2<u32> = bitcast %4
    ret %5
  }
}
%main = @compute @workgroup_size(1, 1, 1) func():void {
  $B3: {
    %7:texture_2d<f32> = load %my_tex
    %8:vec2<u32> = call %foo, %7
    %a:vec2<u32> = let %8
    ret
  }
}
)";

    capabilities = core::ir::Capabilities{core::ir::Capability::kAllowHandleVarsWithoutBindings};

    TexturePolyfillConfig cfg;
    cfg.placeholder_sampler_bind_point = {2, 2};

    binding::CombinedTextureSamplerPair pair;
    pair.texture = {0, 0};
    pair.sampler = {2, 2};
    cfg.sampler_texture_to_name[pair] = "my_tex";

    Run(TexturePolyfill, cfg);
    EXPECT_EQ(expect, str());
}

// GlobalTextureGlobalSampler
// GlobalTextureLocalSampler
// LocalTextureGlobalSampler
// LocaltextureLocalSampler

/*
TEST_F(CombineSamplersTest, SimplePair) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn main() -> vec4<f32> {
  return textureSample(t, s, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn main() -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePair_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return textureSample(t, s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn main() -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairInAFunction) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

fn main() -> vec4<f32> {
  return sample(t, s, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, coords);
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s_1 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return sample(t_s_1, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairInAFunction_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return sample(t, s, vec2<f32>(1.0, 2.0));
}

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

@group(0) @binding(1) var s : sampler;

@group(0) @binding(0) var t : texture_2d<f32>;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return sample(t_s, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s_1 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s_1, placeholder_sampler, coords);
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairRename) {
    auto* src = R"(
@group(0) @binding(1) var t : texture_2d<f32>;

@group(2) @binding(3) var s : sampler;

fn main() -> vec4<f32> {
  return textureSample(t, s, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var fuzzy :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn main() -> vec4<f32> {
  return textureSample(fuzzy, placeholder_sampler, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    binding.placeholder_sampler_bind_point = {1024, 0};

    binding::CombinedTextureSamplerPair pair;
    pair.texture = {0, 1};
    pair.sampler = {2, 3};
    binding.sampler_texture_to_name[pair] = "fuzzy";

    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairRenameMiss) {
    auto* src = R"(
@group(0) @binding(1) var t : texture_2d<f32>;

@group(2) @binding(3) var s : sampler;

fn main() -> vec4<f32> {
  return textureSample(t, s, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn main() -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;

    Bindings binding;
    binding.placeholder_sampler_bind_point = {1024, 0};

    binding::CombinedTextureSamplerPair pair;
    pair.texture = {3, 2};
    pair.sampler = {1, 0};
    binding.sampler_texture_to_name[pair] = "fuzzy";

    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairInTwoFunctions) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn g(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

fn f(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return g(t, s, coords);
}

fn main() -> vec4<f32> {
  return f(t, s, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn g(t_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, coords);
}

fn f(t_s_1 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return g(t_s_1, coords);
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s_2 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(t_s_2, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, SimplePairInTwoFunctions_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return f(t, s, vec2<f32>(1.0, 2.0));
}

fn f(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return g(t, s, coords);
}

fn g(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

@group(0) @binding(1) var s : sampler;
@group(0) @binding(0) var t : texture_2d<f32>;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t_s :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(t_s, vec2<f32>(1.0, 2.0));
}

fn f(t_s_1 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return g(t_s_1, coords);
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn g(t_s_2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s_2, placeholder_sampler, coords);
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TwoFunctionsGenerateSamePair) {
    auto* src = R"(
@group(1) @binding(0) var tex : texture_2d<f32>;

@group(1) @binding(1) var samp : sampler;

fn f() -> vec4<f32> {
  return textureSample(tex, samp, vec2<f32>(1.0, 2.0));
}

fn g() -> vec4<f32> {
  return textureSample(tex, samp, vec2<f32>(3.0, 4.0));
}

fn main() -> vec4<f32> {
  return f() + g();
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn f() -> vec4<f32> {
  return textureSample(tex_samp, placeholder_sampler, vec2<f32>(1.0, 2.0));
}

fn g() -> vec4<f32> {
  return textureSample(tex_samp, placeholder_sampler, vec2<f32>(3.0, 4.0));
}

fn main() -> vec4<f32> {
  return (f() + g());
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, ThreeTexturesThreeSamplers) {
    auto* src = R"(
@group(0) @binding(0) var tex1 : texture_2d<f32>;
@group(0) @binding(1) var tex2 : texture_2d<f32>;
@group(0) @binding(2) var tex3 : texture_2d<f32>;

@group(1) @binding(0) var samp1 : sampler;
@group(1) @binding(1) var samp2: sampler;
@group(1) @binding(2) var samp3: sampler;

fn sample(t : texture_2d<f32>, s : sampler) -> vec4<f32> {
  return textureSample(t, s, vec2<f32>(1.0, 2.0));
}

fn main() -> vec4<f32> {
  return sample(tex1, samp1)
       + sample(tex1, samp2)
       + sample(tex1, samp3)
       + sample(tex2, samp1)
       + sample(tex2, samp2)
       + sample(tex2, samp3)
       + sample(tex3, samp1)
       + sample(tex3, samp2)
       + sample(tex3, samp3);
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s : texture_2d<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp2 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp3 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp2 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp3 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex3_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex3_samp2 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex3_samp3 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return ((((((((sample(tex1_samp1) + sample(tex1_samp2)) + sample(tex1_samp3)) +
sample(tex2_samp1)) + sample(tex2_samp2)) + sample(tex2_samp3)) + sample(tex3_samp1)) +
sample(tex3_samp2)) + sample(tex3_samp3));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TwoFunctionsTwoTexturesDiamond) {
    auto* src = R"(
@group(0) @binding(0) var tex1 : texture_2d<f32>;

@group(0) @binding(1) var tex2 : texture_2d<f32>;

@group(0) @binding(2) var samp : sampler;

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

fn f(t1 : texture_2d<f32>, t2 : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return sample(t1, s, coords) + sample(t2, s, coords);
}

fn main() -> vec4<f32> {
  return f(tex1, tex2, samp, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, coords);
}

fn f(t1_s : texture_2d<f32>, t2_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (sample(t1_s, coords) + sample(t2_s, coords));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex1_samp, tex2_samp, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TwoFunctionsTwoSamplersDiamond) {
    auto* src = R"(
@group(0) @binding(0) var tex : texture_2d<f32>;

@group(0) @binding(1) var samp1 : sampler;

@group(0) @binding(2) var samp2 : sampler;

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

fn f(t : texture_2d<f32>, s1 : sampler, s2 : sampler, coords : vec2<f32>) -> vec4<f32> {
  return sample(t, s1, coords) + sample(t, s2, coords);
}

fn main() -> vec4<f32> {
  return f(tex, samp1, samp2, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, coords);
}

fn f(t_s1 : texture_2d<f32>, t_s2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (sample(t_s1, coords) + sample(t_s2, coords));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp2 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex_samp1, tex_samp2, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, GlobalTextureLocalSampler) {
    auto* src = R"(
@group(0) @binding(0) var tex : texture_2d<f32>;

@group(0) @binding(1) var samp1 : sampler;

@group(0) @binding(2) var samp2 : sampler;

fn f(s1 : sampler, s2 : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(tex, s1, coords) + textureSample(tex, s2, coords);
}

fn main() -> vec4<f32> {
  return f(samp1, samp2, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn f(tex_s1 : texture_2d<f32>, tex_s2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (textureSample(tex_s1, placeholder_sampler, coords) + textureSample(tex_s2,
placeholder_sampler, coords));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp2 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex_samp1, tex_samp2, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, GlobalTextureLocalSampler_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return f(samp1, samp2, vec2<f32>(1.0, 2.0));
}

fn f(s1 : sampler, s2 : sampler, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(tex, s1, coords) + textureSample(tex, s2, coords);
}

@group(0) @binding(1) var samp1 : sampler;
@group(0) @binding(2) var samp2 : sampler;
@group(0) @binding(0) var tex : texture_2d<f32>;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp1 :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp2 :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex_samp1, tex_samp2, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn f(tex_s1 : texture_2d<f32>, tex_s2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (textureSample(tex_s1, placeholder_sampler, coords) + textureSample(tex_s2,
placeholder_sampler, coords));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, LocalTextureGlobalSampler) {
    auto* src = R"(
@group(0) @binding(0) var tex1 : texture_2d<f32>;

@group(0) @binding(1) var tex2 : texture_2d<f32>;

@group(0) @binding(2) var samp : sampler;

fn f(t1 : texture_2d<f32>, t2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t1, samp, coords) + textureSample(t2, samp, coords);
}

fn main() -> vec4<f32> {
  return f(tex1, tex2, vec2<f32>(1.0, 2.0));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn f(t1_samp : texture_2d<f32>, t2_samp : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (textureSample(t1_samp, placeholder_sampler, coords) + textureSample(t2_samp,
placeholder_sampler, coords));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex1_samp, tex2_samp, vec2<f32>(1.0, 2.0));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, LocalTextureGlobalSampler_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return f(tex1, tex2, vec2<f32>(1.0, 2.0));
}

fn f(t1 : texture_2d<f32>, t2 : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t1, samp, coords) + textureSample(t2, samp, coords);
}

@group(0) @binding(2) var samp : sampler;
@group(0) @binding(0) var tex1 : texture_2d<f32>;
@group(0) @binding(1) var tex2 : texture_2d<f32>;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex1_samp :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex2_samp :
texture_2d<f32>;

fn main() -> vec4<f32> {
  return f(tex1_samp, tex2_samp, vec2<f32>(1.0, 2.0));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn f(t1_samp : texture_2d<f32>, t2_samp : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return (textureSample(t1_samp, placeholder_sampler, coords) + textureSample(t2_samp,
placeholder_sampler, coords));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TextureWithAndWithoutSampler) {
    auto* src = R"(
@group(0) @binding(0) var tex : texture_2d<f32>;
@group(0) @binding(1) var samp : sampler;

fn main() -> vec4<f32> {
  return textureLoad(tex, vec2<i32>(), 0) +
         textureSample(tex, samp, vec2<f32>());
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var barney :
texture_2d<f32>;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn main() -> vec4<f32> {
  return (textureLoad(barney, vec2<i32>(), 0) + textureSample(barney, placeholder_sampler,
vec2<f32>()));
}
)";

    BindingPoint tex{0, 0};
    BindingPoint samp{0, 1};

    Bindings binding;
    binding.placeholder_sampler_bind_point = {1024, 0};

    binding::CombinedTextureSamplerPair pair;
    pair.texture = {tex.group, tex.binding};
    pair.sampler = {samp.group, samp.binding};

    binding::CombinedTextureSamplerPair placeholder_pair;
    placeholder_pair.texture = {tex.group, tex.binding};
    placeholder_pair.sampler = {binding.placeholder_sampler_bind_point.group,
                                binding.placeholder_sampler_bind_point.binding};

    binding.sampler_texture_to_name[pair] = "barney";
    binding.sampler_texture_to_name[placeholder_pair] = "fred";

    ast::transform::DataMap data;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TextureSampleCompare) {
    auto* src = R"(
@group(0) @binding(0) var tex : texture_depth_2d;

@group(0) @binding(1) var samp : sampler_comparison;

fn main() -> vec4<f32> {
  return vec4<f32>(textureSampleCompare(tex, samp, vec2<f32>(1.0, 2.0), 0.5));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp :
texture_depth_2d;

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var
placeholder_comparison_sampler : sampler_comparison;

fn main() -> vec4<f32> {
  return vec4<f32>(textureSampleCompare(tex_samp, placeholder_comparison_sampler,
vec2<f32>(1.0, 2.0), 0.5));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TextureSampleCompareInAFunction) {
    auto* src = R"(
@group(0) @binding(0) var tex : texture_depth_2d;

@group(0) @binding(1) var samp : sampler_comparison;

fn f(t : texture_depth_2d, s : sampler_comparison, coords : vec2<f32>) -> f32 {
  return textureSampleCompare(t, s, coords, 5.0f);
}

fn main() -> vec4<f32> {
  return vec4<f32>(f(tex, samp, vec2<f32>(1.0, 2.0)));
}
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var
placeholder_comparison_sampler : sampler_comparison;

fn f(t_s : texture_depth_2d, coords : vec2<f32>) -> f32 {
  return textureSampleCompare(t_s, placeholder_comparison_sampler, coords, 5.0f);
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp :
texture_depth_2d;

fn main() -> vec4<f32> {
  return vec4<f32>(f(tex_samp, vec2<f32>(1.0, 2.0)));
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, TextureSampleCompareInAFunction_OutOfOrder) {
    auto* src = R"(
fn main() -> vec4<f32> {
  return vec4<f32>(f(tex, samp, vec2<f32>(1.0, 2.0)));
}

fn f(t : texture_depth_2d, s : sampler_comparison, coords : vec2<f32>) -> f32 {
  return textureSampleCompare(t, s, coords, 5.0f);
}

@group(0) @binding(0) var tex : texture_depth_2d;
@group(0) @binding(1) var samp : sampler_comparison;
)";
    auto* expect = R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var tex_samp :
texture_depth_2d;

fn main() -> vec4<f32> {
  return vec4<f32>(f(tex_samp, vec2<f32>(1.0, 2.0)));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var
placeholder_comparison_sampler : sampler_comparison;

fn f(t_s : texture_depth_2d, coords : vec2<f32>) -> f32 {
  return textureSampleCompare(t_s, placeholder_comparison_sampler, coords, 5.0f);
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureFunctionParameter) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

fn f(tex: texture_2d<f32>) -> u32 {
  return 1u;
}

fn main() {
  _ = f(t);
}
)";
    auto* expect = R"(
fn f() -> u32 {
  return 1u;
}

fn main() {
  _ = f();
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedSamplerFunctionParameter) {
    auto* src = R"(
@group(0) @binding(0) var s : sampler;

fn f(sampler1: sampler) -> u32 {
  return 1u;
}

fn main() {
  _ = f(s);
}
)";
    auto* expect = R"(
fn f() -> u32 {
  return 1u;
}

fn main() {
  _ = f();
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureAndSamplerFunctionParameter) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn f(tex: texture_2d<f32>, sampler1: sampler) -> u32 {
  return 1u;
}

fn main() {
  _ = f(t, s);
}
)";
    auto* expect = R"(
fn f() -> u32 {
  return 1u;
}

fn main() {
  _ = f();
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureFunctionParameter_Multiple) {
    auto* src = R"(
@group(0) @binding(0) var t1 : texture_2d<f32>;

@group(0) @binding(1) var t2 : texture_2d_array<f32>;

@group(0) @binding(2) var s : sampler;

fn f(tex1: texture_2d<f32>, tex2: texture_2d<f32>, tex3: texture_2d_array<f32>, sampler1: sampler)
-> u32 { return 1u + textureNumLayers(tex3);
}

fn main() {
  _ = f(t1, t1, t2, s);
}
)";
    auto* expect = R"(
fn f(tex3_1 : texture_2d_array<f32>) -> u32 {
  return (1u + textureNumLayers(tex3_1));
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var t2_1 :
texture_2d_array<f32>;

fn main() {
  _ = f(t2_1);
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureFunctionParameter_Nested) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

fn f_nested(tex: texture_2d<f32>) -> u32 {
  return 1u;
}

fn f(tex: texture_2d<f32>) -> u32 {
  return f_nested(tex);
}

fn main() {
  _ = f(t);
}
)";
    auto* expect = R"(
fn f_nested() -> u32 {
  return 1u;
}

fn f() -> u32 {
  return f_nested();
}

fn main() {
  _ = f();
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureAndSamplerFunctionParameter_Nested) {
    auto* src = R"(
@group(0) @binding(0) var t : texture_2d<f32>;

@group(0) @binding(1) var s : sampler;

fn f_nested(tex: texture_2d<f32>, sampler1: sampler) -> u32 {
  return 1u;
}

fn f(tex: texture_2d<f32>, sampler1: sampler) -> u32 {
  return f_nested(tex, sampler1);
}

fn main() {
  _ = f(t, s);
}
)";
    auto* expect =
        R"(
fn f_nested() -> u32 {
  return 1u;
}

fn f() -> u32 {
  return f_nested();
}

fn main() {
  _ = f();
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}

TEST_F(CombineSamplersTest, UnusedTextureAndSamplerFunctionParameter_Nested2) {
    auto* src = R"(
@group(0) @binding(1) var texture1 : texture_2d<f32>;
@group(0) @binding(2) var texture2 : texture_2d<f32>;
@group(0) @binding(3) var sampler1 : sampler;

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>, unused : texture_2d<f32>) ->
vec4<f32> { return textureSample(t, s, coords);
}

@fragment
fn main(@location(0) coords : vec2<f32>) -> @location(0) vec4<f32> {
  var a = sample(texture1, sampler1, coords, texture1);
  var b = sample(texture1, sampler1, coords, texture2);
  return a + b;
}
)";
    auto* expect =
        R"(
@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var placeholder_sampler
: sampler;

fn sample(t_s : texture_2d<f32>, coords : vec2<f32>) -> vec4<f32> {
  return textureSample(t_s, placeholder_sampler, coords);
}

@group(0) @binding(0) @internal(disable_validation__binding_point_collision) var texture1_sampler1 :
texture_2d<f32>;

@fragment
fn main(@location(0) coords : vec2<f32>) -> @location(0) vec4<f32> {
  var a = sample(texture1_sampler1, coords);
  var b = sample(texture1_sampler1, coords);
  return (a + b);
}
)";

    ast::transform::DataMap data;
    Bindings binding;
    data.Add<Bindings>(binding);
    auto got = Run<CombineSamplers>(src, data);

    EXPECT_EQ(expect, str(got));
}


 */

}  // namespace
}  // namespace tint::glsl::writer::raise
