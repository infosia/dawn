// Copyright 2026 The Dawn & Tint Authors
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

#include "src/tint/lang/core/ir/transform/resource_table.h"

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/type/resource_table.h"
#include "src/tint/lang/core/type/resource_type.h"
#include "src/tint/lang/core/type/sampled_texture.h"  // IWYU pragma: export
#include "src/tint/lang/core/type/storage_texture.h"  // IWYU pragma: export

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {
namespace {

class Helper : public ResourceTableHelper {
  public:
    ~Helper() override = default;

    // Returns a map of types to the var which is used to access the memory of that type
    Hashmap<const core::type::Type*, core::ir::Var*, 4> GenerateVars(
        core::ir::Builder& b,
        const BindingPoint& bp,
        const std::vector<ResourceType>& types) const override {
        Hashmap<const core::type::Type*, core::ir::Var*, 4> res;

        for (auto& type : types) {
            auto* t = core::type::ResourceTypeToType(b.ir.Types(), type);
            auto* ty = b.ir.Types().Get<core::type::ResourceTable>(t);

            auto* v = b.Var(b.ir.Types().ptr(handle, ty));
            v->SetBindingPoint(bp.group, bp.binding);
            res.Add(t, v);
        }

        return res;
    }
};

using IR_ResourceTableTest = TransformTest;

TEST_F(IR_ResourceTableTest, NoResources) {
    auto format = core::TexelFormat::kRgba8Unorm;
    auto* texture_ty =
        ty.storage_texture(core::type::TextureDimension::k2d, format, core::Access::kWrite);

    auto* var = b.Var("texture", ty.ptr(handle, texture_ty));
    var->SetBindingPoint(3, 2);
    mod.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] { b.Return(func); });

    auto* src = R"(
$B1: {  # root
  %texture:ptr<handle, texture_storage_2d<rgba8unorm, write>, read> = var undef @binding_point(3, 2)
}

%foo = func():void {
  $B2: {
    ret
  }
}
)";

    auto* expect = src;

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order = {},
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, MissingConfig) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2dArray, ty.u32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{texture_ty}, 1_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<texture_2d_array<u32>> 1u
    %t:bool = let %2
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    auto result = RunWithFailure(ResourceTable, std::nullopt, &helper);
    EXPECT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason, "hasResource and getResource require a resource table");
}

TEST_F(IR_ResourceTableTest, HasResource) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2dArray, ty.u32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{texture_ty}, 1_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<texture_2d_array<u32>> 1u
    %t:bool = let %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %6:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %7:u32 = load %6
    %8:bool = lt 1u, %7
    %9:bool = if %8 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %11:u32 = load %10
        %12:bool = eq %11, 15u
        exit_if %12  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %t:bool = let %9
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, HasResource_Filterable) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{texture_ty}, 2_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<texture_2d<f32>> 2u
    %t:bool = let %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %6:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %7:u32 = load %6
    %8:bool = lt 2u, %7
    %9:bool = if %8 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %11:u32 = load %10
        %12:vec3<u32> = construct %11
        %13:vec3<u32> = construct 6u, 7u, 34u
        %14:vec3<bool> = eq %12, %13
        %15:bool = any %14
        exit_if %15  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %t:bool = let %9
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2d_f32_filterable,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, HasResource_Unfilterable) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{texture_ty}, 2_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<texture_2d<f32>> 2u
    %t:bool = let %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %6:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %7:u32 = load %6
    %8:bool = lt 2u, %7
    %9:bool = if %8 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %11:u32 = load %10
        %12:vec3<u32> = construct %11
        %13:vec3<u32> = construct 6u, 7u, 34u
        %14:vec3<bool> = eq %12, %13
        %15:bool = any %14
        exit_if %15  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %t:bool = let %9
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_unfilterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2d_f32_unfilterable,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_Texture) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2dArray, ty.u32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);
        b.Call(ty.vec2<u32>(), core::BuiltinFn::kTextureDimensions, tex);
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d_array<u32> = getResource<texture_2d_array<u32>> 1u
    %3:vec2<u32> = textureDimensions %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %6:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %7:u32 = load %6
    %8:bool = lt 1u, %7
    %9:bool = if %8 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %11:u32 = load %10
        %12:bool = eq %11, 15u
        exit_if %12  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %13:u32 = if %9 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        exit_if 1u  # if_2
      }
      $B6: {  # false
        %14:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %15:u32 = load %14
        %16:u32 = add 2u, %15
        exit_if %16  # if_2
      }
    }
    %17:ptr<handle, texture_2d_array<u32>, read> = access %3, %13
    %18:texture_2d_array<u32> = load %17
    %19:vec2<u32> = textureDimensions %18
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_ResourceTexture_ResourceSampler) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* sampler_ty = ty.sampler();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);
        auto* sam =
            b.CallExplicit(sampler_ty, core::BuiltinFn::kGetResource, Vector{sampler_ty}, 2_u);

        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d<f32> = getResource<texture_2d<f32>> 1u
    %3:sampler = getResource<sampler> 2u
    %4:vec4<f32> = textureSample %2, %3, vec2<f32>(0.0f)
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %4:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %5:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
  %6:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %9:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %10:u32 = load %9
    %11:bool = lt 1u, %10
    %12:bool = if %11 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %13:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %14:u32 = load %13
        %15:vec3<u32> = construct %14
        %16:vec3<u32> = construct 6u, 7u, 34u
        %17:vec3<bool> = eq %15, %16
        %18:bool = any %17
        exit_if %18  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %texture_kind:u32 = if %12 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        %20:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %21:u32 = load %20
        exit_if %21  # if_2
      }
      $B6: {  # false
        exit_if 6u  # if_2
      }
    }
    %22:u32 = if %12 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        exit_if 1u  # if_3
      }
      $B8: {  # false
        %23:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %24:u32 = load %23
        %25:u32 = add 1u, %24
        exit_if %25  # if_3
      }
    }
    %26:ptr<handle, texture_2d<f32>, read> = access %1, %22
    %27:texture_2d<f32> = load %26
    %28:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %29:u32 = load %28
    %30:bool = lt 2u, %29
    %31:bool = if %30 [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        %32:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %33:u32 = load %32
        %34:vec2<u32> = construct %33
        %35:vec2<u32> = construct 40u, 41u
        %36:vec2<bool> = eq %34, %35
        %37:bool = any %36
        exit_if %37  # if_4
      }
      $B10: {  # false
        exit_if false  # if_4
      }
    }
    %sampler_kind:u32 = if %31 [t: $B11, f: $B12] {  # if_5
      $B11: {  # true
        %39:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %40:u32 = load %39
        exit_if %40  # if_5
      }
      $B12: {  # false
        exit_if 41u  # if_5
      }
    }
    %41:u32 = if %31 [t: $B13, f: $B14] {  # if_6
      $B13: {  # true
        exit_if 2u  # if_6
      }
      $B14: {  # false
        %42:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %43:u32 = load %42
        %44:u32 = add 5u, %43
        exit_if %44  # if_6
      }
    }
    %45:ptr<handle, sampler, read> = access %5, %41
    %46:sampler = load %45
    %sampler_is_filtering:bool = eq %sampler_kind, 40u
    %use_sampler:bool = if %sampler_is_filtering [t: $B15, f: $B16] {  # if_7
      $B15: {  # true
        %texture_is_unfilterable:bool = switch %texture_kind [c: (default, $B17), c: (11u 16u 1u 21u 26u 6u, $B18)] {  # switch_1
          $B17: {  # case
            exit_switch false  # switch_1
          }
          $B18: {  # case
            exit_switch true  # switch_1
          }
        }
        exit_if %texture_is_unfilterable  # if_7
      }
      $B16: {  # false
        exit_if true  # if_7
      }
    }
    %50:vec4<f32> = if %use_sampler [t: $B19, f: $B20] {  # if_8
      $B19: {  # true
        %51:vec4<f32> = textureSample %27, %46, vec2<f32>(0.0f)
        exit_if %51  # if_8
      }
      $B20: {  # false
        %52:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %53:u32 = load %52
        %54:u32 = add 5u, %53
        %55:ptr<handle, sampler, read> = access %5, %54
        %56:sampler = load %55
        %57:vec4<f32> = textureSample %27, %56, vec2<f32>(0.0f)
        exit_if %57  # if_8
      }
    }
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture2d_f32_unfilterable,
                    ResourceType::kTexture2d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                    ResourceType::kSampler_filtering,
                    ResourceType::kSampler_non_filtering,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, DISABLED_GetResource_ResourceTexture_VarSampler) {
    auto* sam = b.Var("sampler", ty.ptr(handle, ty.sampler()));
    sam->SetBindingPoint(3, 2);
    mod.root_block->Append(sam);

    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);

        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d<f32> = getResource<texture_2d<f32>> 1u
    %3:sampler = getResource<sampler> 2u
    %4:vec4<f32> = textureSample %2, %3, vec2<f32>(0.0f)
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture2d_f32_unfilterable,
                    ResourceType::kTexture2d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                    ResourceType::kSampler_filtering,
                    ResourceType::kSampler_non_filtering,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, DISABLED_GetResource_VarTexture_ResourceSampler) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* tex = b.Var("texture", ty.ptr(handle, texture_ty));
    tex->SetBindingPoint(3, 2);
    mod.root_block->Append(tex);

    auto* sampler_ty = ty.sampler();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* sam =
            b.CallExplicit(sampler_ty, core::BuiltinFn::kGetResource, Vector{sampler_ty}, 1_u);

        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d<f32> = getResource<texture_2d<f32>> 1u
    %3:sampler = getResource<sampler> 2u
    %4:vec4<f32> = textureSample %2, %3, vec2<f32>(0.0f)
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture2d_f32_unfilterable,
                    ResourceType::kTexture2d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                    ResourceType::kSampler_filtering,
                    ResourceType::kSampler_non_filtering,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

// TODO(479179409): This case could be smarter. When reading from the
// resource_table we could track the scope we're in and decide we can re-use a
// previous load.
TEST_F(IR_ResourceTableTest, GetResource_MultiUse) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2dArray, ty.f32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);
        b.Call(ty.vec2<u32>(), core::BuiltinFn::kTextureDimensions, tex);
        b.Call(ty.vec2<u32>(), core::BuiltinFn::kTextureDimensions, tex);
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d_array<f32> = getResource<texture_2d_array<f32>> 1u
    %3:vec2<u32> = textureDimensions %2
    %4:vec2<u32> = textureDimensions %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d_array<f32>>, read> = var undef @binding_point(0, 1)
  %4:ptr<handle, resource_table<texture_2d_array<f32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %7:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %8:u32 = load %7
    %9:bool = lt 1u, %8
    %10:bool = if %9 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %11:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %12:u32 = load %11
        %13:vec3<u32> = construct %12
        %14:vec3<u32> = construct 11u, 12u, 35u
        %15:vec3<bool> = eq %13, %14
        %16:bool = any %15
        exit_if %16  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %17:u32 = if %10 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        exit_if 1u  # if_2
      }
      $B6: {  # false
        %18:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %19:u32 = load %18
        %20:u32 = add 3u, %19
        exit_if %20  # if_2
      }
    }
    %21:ptr<handle, texture_2d_array<f32>, read> = access %3, %17
    %22:texture_2d_array<f32> = load %21
    %23:vec2<u32> = textureDimensions %22
    %24:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %25:u32 = load %24
    %26:bool = lt 1u, %25
    %27:bool = if %26 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        %28:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %29:u32 = load %28
        %30:vec3<u32> = construct %29
        %31:vec3<u32> = construct 11u, 12u, 35u
        %32:vec3<bool> = eq %30, %31
        %33:bool = any %32
        exit_if %33  # if_3
      }
      $B8: {  # false
        exit_if false  # if_3
      }
    }
    %34:u32 = if %27 [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        exit_if 1u  # if_4
      }
      $B10: {  # false
        %35:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %36:u32 = load %35
        %37:u32 = add 3u, %36
        exit_if %37  # if_4
      }
    }
    %38:ptr<handle, texture_2d_array<f32>, read> = access %3, %34
    %39:texture_2d_array<f32> = load %38
    %40:vec2<u32> = textureDimensions %39
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_unfilterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_f32_unfilterable,
                    ResourceType::kTexture2dArray_f32_filterable,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_MultiUse_DifferentScope) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2dArray, ty.u32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);
        auto* if_ = b.If(true);
        b.Append(if_->True(), [&] {
            b.Call(ty.vec2<u32>(), core::BuiltinFn::kTextureDimensions, tex);
            b.ExitIf(if_);
        });
        b.Append(if_->False(), [&] {
            b.Call(ty.vec2<u32>(), core::BuiltinFn::kTextureDimensions, tex);
            b.ExitIf(if_);
        });

        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d_array<u32> = getResource<texture_2d_array<u32>> 1u
    if true [t: $B2, f: $B3] {  # if_1
      $B2: {  # true
        %3:vec2<u32> = textureDimensions %2
        exit_if  # if_1
      }
      $B3: {  # false
        %4:vec2<u32> = textureDimensions %2
        exit_if  # if_1
      }
    }
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    if true [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %6:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %7:u32 = load %6
        %8:bool = lt 1u, %7
        %9:bool = if %8 [t: $B5, f: $B6] {  # if_2
          $B5: {  # true
            %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
            %11:u32 = load %10
            %12:bool = eq %11, 15u
            exit_if %12  # if_2
          }
          $B6: {  # false
            exit_if false  # if_2
          }
        }
        %13:u32 = if %9 [t: $B7, f: $B8] {  # if_3
          $B7: {  # true
            exit_if 1u  # if_3
          }
          $B8: {  # false
            %14:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
            %15:u32 = load %14
            %16:u32 = add 2u, %15
            exit_if %16  # if_3
          }
        }
        %17:ptr<handle, texture_2d_array<u32>, read> = access %3, %13
        %18:texture_2d_array<u32> = load %17
        %19:vec2<u32> = textureDimensions %18
        exit_if  # if_1
      }
      $B4: {  # false
        %20:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %21:u32 = load %20
        %22:bool = lt 1u, %21
        %23:bool = if %22 [t: $B9, f: $B10] {  # if_4
          $B9: {  # true
            %24:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
            %25:u32 = load %24
            %26:bool = eq %25, 15u
            exit_if %26  # if_4
          }
          $B10: {  # false
            exit_if false  # if_4
          }
        }
        %27:u32 = if %23 [t: $B11, f: $B12] {  # if_5
          $B11: {  # true
            exit_if 1u  # if_5
          }
          $B12: {  # false
            %28:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
            %29:u32 = load %28
            %30:u32 = add 2u, %29
            exit_if %30  # if_5
          }
        }
        %31:ptr<handle, texture_2d_array<u32>, read> = access %3, %27
        %32:texture_2d_array<u32> = load %31
        %33:vec2<u32> = textureDimensions %32
        exit_if  # if_1
      }
    }
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2dArray_u32,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_Unused) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32()
                                          // core::TextureFilterable::kUnfilterable
    );

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 2_u);
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:texture_2d<f32> = getResource<texture_2d<f32>> 2u
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<texture_1d<f32>>, read> = var undef @binding_point(0, 1)
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kTexture1d_f32_filterable,
                    ResourceType::kTexture3d_i32,
                    ResourceType::kTexture2d_f32_unfilterable,
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, DISABLED_HasResource_GetSamplerIndexFromMetadata) {
    auto* sampler_ty = ty.sampler();  // SamplerFiltering::kNonFiltering);

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{sampler_ty}, 1_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<sampler<non_filtering>> 1u
    %t:bool = let %2
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<sampler<non_filtering>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %4:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %5:u32 = load %4
    %6:bool = lt 1u, %5
    %7:bool = if %6 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %8:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %9:u32 = load %8
        %10:u32 = and %9, 65535u
        %11:vec2<u32> = construct %10
        %12:vec2<u32> = construct 34u, 33u
        %13:vec2<bool> = eq %11, %12
        %14:bool = any %13
        exit_if %14  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %t:bool = let %7
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kSampler_non_filtering,
                },
            .get_sampler_index_from_metadata = true,
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, DISABLED_GetResource_GetSamplerIndexFromMetadata) {
    auto* sampler_ty = ty.sampler();  // SamplerFiltering::kNonFiltering);

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.CallExplicit(sampler_ty, core::BuiltinFn::kGetResource, Vector{sampler_ty}, 1_u);
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:sampler<non_filtering> = getResource<sampler<non_filtering>> 1u
    ret
  }
}
)";

    auto* expect = R"(
tint_resource_table_metadata_struct = struct @align(4) {
  array_length:u32 @offset(0)
  bindings:array<u32> @offset(4)
}

$B1: {  # root
  %1:ptr<handle, resource_table<sampler<non_filtering>>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %4:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %5:u32 = load %4
    %6:bool = lt 1u, %5
    %7:bool = if %6 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %8:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %9:u32 = load %8
        %10:u32 = and %9, 65535u
        %11:vec2<u32> = construct %10
        %12:vec2<u32> = construct 34u, 33u
        %13:vec2<bool> = eq %11, %12
        %14:bool = any %13
        exit_if %14  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %15:u32 = if %7 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        exit_if 1u  # if_2
      }
      $B6: {  # false
        %16:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %17:u32 = load %16
        %18:u32 = add 0u, %17
        exit_if %18  # if_2
      }
    }
    %19:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, %15
    %20:u32 = load %19
    %21:u32 = shr %20, 16u
    %22:ptr<handle, sampler<non_filtering>, read> = access %1, %21
    %23:sampler<non_filtering> = load %22
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    Helper helper;
    Run(ResourceTable,
        ResourceTableConfig{
            .resource_table_binding = {0, 1},
            .storage_buffer_binding = {1, 2},
            .default_binding_type_order =
                {
                    ResourceType::kSampler_non_filtering,
                },
            .get_sampler_index_from_metadata = true,
        },
        &helper);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
