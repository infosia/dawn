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

            if (res.Contains(t)) {
                continue;
            }

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
            .binding_to_resource_type = {},
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
            .binding_to_resource_type = {},
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
            .binding_to_resource_type = {},
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
            .binding_to_resource_type = {},
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
            .binding_to_resource_type = {},
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
  %2:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %4:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
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
        %14:vec3<u32> = construct 6u, 7u, 34u
        %15:vec3<bool> = eq %13, %14
        %16:bool = any %15
        exit_if %16  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %texture_kind:u32 = if %10 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        %18:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %19:u32 = load %18
        exit_if %19  # if_2
      }
      $B6: {  # false
        exit_if 6u  # if_2
      }
    }
    %20:u32 = if %10 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        exit_if 1u  # if_3
      }
      $B8: {  # false
        %21:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %22:u32 = load %21
        %23:u32 = add 1u, %22
        exit_if %23  # if_3
      }
    }
    %24:ptr<handle, texture_2d<f32>, read> = access %1, %20
    %25:texture_2d<f32> = load %24
    %26:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %27:u32 = load %26
    %28:bool = lt 2u, %27
    %29:bool = if %28 [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        %30:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %31:u32 = load %30
        %32:vec2<u32> = construct %31
        %33:vec2<u32> = construct 40u, 41u
        %34:vec2<bool> = eq %32, %33
        %35:bool = any %34
        exit_if %35  # if_4
      }
      $B10: {  # false
        exit_if false  # if_4
      }
    }
    %sampler_kind:u32 = if %29 [t: $B11, f: $B12] {  # if_5
      $B11: {  # true
        %37:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 2u
        %38:u32 = load %37
        exit_if %38  # if_5
      }
      $B12: {  # false
        exit_if 41u  # if_5
      }
    }
    %39:u32 = if %29 [t: $B13, f: $B14] {  # if_6
      $B13: {  # true
        exit_if 2u  # if_6
      }
      $B14: {  # false
        %40:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %41:u32 = load %40
        %42:u32 = add 5u, %41
        exit_if %42  # if_6
      }
    }
    %43:ptr<handle, sampler, read> = access %4, %39
    %44:sampler = load %43
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
    %48:vec4<f32> = if %use_sampler [t: $B19, f: $B20] {  # if_8
      $B19: {  # true
        %49:vec4<f32> = textureSample %25, %44, vec2<f32>(0.0f)
        exit_if %49  # if_8
      }
      $B20: {  # false
        %50:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %51:u32 = load %50
        %52:u32 = add 5u, %51
        %53:ptr<handle, sampler, read> = access %4, %52
        %54:sampler = load %53
        %55:vec4<f32> = textureSample %25, %54, vec2<f32>(0.0f)
        exit_if %55  # if_8
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
            .binding_to_resource_type = {},
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_ResourceTexture_VarSampler) {
    auto* sam_var = b.Var("sampler", ty.ptr(handle, ty.sampler()));
    sam_var->SetBindingPoint(3, 2);
    mod.root_block->Append(sam_var);

    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* tex =
            b.CallExplicit(texture_ty, core::BuiltinFn::kGetResource, Vector{texture_ty}, 1_u);

        core::ir::Load* sam = b.Load(sam_var);
        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %sampler:ptr<handle, sampler, read> = var undef @binding_point(3, 2)
}

%foo = func():void {
  $B2: {
    %3:texture_2d<f32> = getResource<texture_2d<f32>> 1u
    %4:sampler = load %sampler
    %5:vec4<f32> = textureSample %3, %4, vec2<f32>(0.0f)
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
  %sampler:ptr<handle, sampler, read> = var undef @binding_point(3, 2)
  %2:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %4:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %5:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %8:sampler = load %sampler
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
    %26:ptr<handle, texture_2d<f32>, read> = access %2, %22
    %27:texture_2d<f32> = load %26
    %sampler_kind:u32 = let 40u
    %sampler_is_filtering:bool = eq %sampler_kind, 40u
    %use_sampler:bool = if %sampler_is_filtering [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        %texture_is_unfilterable:bool = switch %texture_kind [c: (default, $B11), c: (11u 16u 1u 21u 26u 6u, $B12)] {  # switch_1
          $B11: {  # case
            exit_switch false  # switch_1
          }
          $B12: {  # case
            exit_switch true  # switch_1
          }
        }
        exit_if %texture_is_unfilterable  # if_4
      }
      $B10: {  # false
        exit_if true  # if_4
      }
    }
    %32:vec4<f32> = if %use_sampler [t: $B13, f: $B14] {  # if_5
      $B13: {  # true
        %33:vec4<f32> = textureSample %27, %8, vec2<f32>(0.0f)
        exit_if %33  # if_5
      }
      $B14: {  # false
        %34:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %35:u32 = load %34
        %36:u32 = add 5u, %35
        %37:ptr<handle, sampler, read> = access %5, %36
        %38:sampler = load %37
        %39:vec4<f32> = textureSample %27, %38, vec2<f32>(0.0f)
        exit_if %39  # if_5
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
            .binding_to_resource_type =
                {
                    {BindingPoint{3, 2}, ResourceType::kSampler_filtering},
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_VarTexture_ResourceSampler) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* tex_var = b.Var("texture", ty.ptr(handle, texture_ty));
    tex_var->SetBindingPoint(3, 2);
    mod.root_block->Append(tex_var);

    auto* sampler_ty = ty.sampler();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* sam =
            b.CallExplicit(sampler_ty, core::BuiltinFn::kGetResource, Vector{sampler_ty}, 1_u);

        core::ir::Load* tex = b.Load(tex_var);
        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %texture:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(3, 2)
}

%foo = func():void {
  $B2: {
    %3:sampler = getResource<sampler> 1u
    %4:texture_2d<f32> = load %texture
    %5:vec4<f32> = textureSample %4, %3, vec2<f32>(0.0f)
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
  %texture:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(3, 2)
  %2:ptr<handle, resource_table<texture_2d<f32>>, read> = var undef @binding_point(0, 1)
  %3:ptr<handle, resource_table<texture_3d<i32>>, read> = var undef @binding_point(0, 1)
  %4:ptr<handle, resource_table<texture_2d_array<u32>>, read> = var undef @binding_point(0, 1)
  %5:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %8:texture_2d<f32> = load %texture
    %texture_kind:u32 = let 6u
    %10:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %11:u32 = load %10
    %12:bool = lt 1u, %11
    %13:bool = if %12 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %14:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %15:u32 = load %14
        %16:vec2<u32> = construct %15
        %17:vec2<u32> = construct 40u, 41u
        %18:vec2<bool> = eq %16, %17
        %19:bool = any %18
        exit_if %19  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %sampler_kind:u32 = if %13 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        %21:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %22:u32 = load %21
        exit_if %22  # if_2
      }
      $B6: {  # false
        exit_if 41u  # if_2
      }
    }
    %23:u32 = if %13 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        exit_if 1u  # if_3
      }
      $B8: {  # false
        %24:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %25:u32 = load %24
        %26:u32 = add 5u, %25
        exit_if %26  # if_3
      }
    }
    %27:ptr<handle, sampler, read> = access %5, %23
    %28:sampler = load %27
    %sampler_is_filtering:bool = eq %sampler_kind, 40u
    %use_sampler:bool = if %sampler_is_filtering [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        %texture_is_unfilterable:bool = switch %texture_kind [c: (default, $B11), c: (11u 16u 1u 21u 26u 6u, $B12)] {  # switch_1
          $B11: {  # case
            exit_switch false  # switch_1
          }
          $B12: {  # case
            exit_switch true  # switch_1
          }
        }
        exit_if %texture_is_unfilterable  # if_4
      }
      $B10: {  # false
        exit_if true  # if_4
      }
    }
    %32:vec4<f32> = if %use_sampler [t: $B13, f: $B14] {  # if_5
      $B13: {  # true
        %33:vec4<f32> = textureSample %8, %28, vec2<f32>(0.0f)
        exit_if %33  # if_5
      }
      $B14: {  # false
        %34:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %35:u32 = load %34
        %36:u32 = add 5u, %35
        %37:ptr<handle, sampler, read> = access %5, %36
        %38:sampler = load %37
        %39:vec4<f32> = textureSample %8, %38, vec2<f32>(0.0f)
        exit_if %39  # if_5
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
            .binding_to_resource_type =
                {
                    {BindingPoint{3, 2}, ResourceType::kTexture2d_f32_filterable},
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

// TODO(479179409): This case could be smarter. When reading from the resource_table we could track
// the scope we're in and decide we can reuse a previous load.
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
        %12:vec3<u32> = construct %11
        %13:vec3<u32> = construct 11u, 12u, 35u
        %14:vec3<bool> = eq %12, %13
        %15:bool = any %14
        exit_if %15  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %16:u32 = if %9 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        exit_if 1u  # if_2
      }
      $B6: {  # false
        %17:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %18:u32 = load %17
        %19:u32 = add 3u, %18
        exit_if %19  # if_2
      }
    }
    %20:ptr<handle, texture_2d_array<f32>, read> = access %3, %16
    %21:texture_2d_array<f32> = load %20
    %22:vec2<u32> = textureDimensions %21
    %23:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %24:u32 = load %23
    %25:bool = lt 1u, %24
    %26:bool = if %25 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        %27:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %28:u32 = load %27
        %29:vec3<u32> = construct %28
        %30:vec3<u32> = construct 11u, 12u, 35u
        %31:vec3<bool> = eq %29, %30
        %32:bool = any %31
        exit_if %32  # if_3
      }
      $B8: {  # false
        exit_if false  # if_3
      }
    }
    %33:u32 = if %26 [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        exit_if 1u  # if_4
      }
      $B10: {  # false
        %34:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %35:u32 = load %34
        %36:u32 = add 3u, %35
        exit_if %36  # if_4
      }
    }
    %37:ptr<handle, texture_2d_array<f32>, read> = access %3, %33
    %38:texture_2d_array<f32> = load %37
    %39:vec2<u32> = textureDimensions %38
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
            .binding_to_resource_type = {},
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
            .binding_to_resource_type = {},
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_Unused) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());

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
            .binding_to_resource_type = {},
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, HasResource_GetSamplerIndexFromMetadata) {
    auto* sampler_ty = ty.sampler();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        b.Let("t",
              b.CallExplicit(ty.bool_(), core::BuiltinFn::kHasResource, Vector{sampler_ty}, 1_u));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %2:bool = hasResource<sampler> 1u
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
  %1:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
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
        %12:vec2<u32> = construct 40u, 41u
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
            .binding_to_resource_type = {},
        },
        &helper);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_ResourceTableTest, GetResource_GetSamplerIndexFromMetadata) {
    auto* texture_ty = ty.sampled_texture(core::type::TextureDimension::k2d, ty.f32());
    auto* tex_var = b.Var("texture", ty.ptr(handle, texture_ty));
    tex_var->SetBindingPoint(3, 2);

    mod.root_block->Append(tex_var);

    auto* sampler_ty = ty.sampler();

    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        core::ir::Load* tex = b.Load(tex_var);
        core::ir::Instruction* sam =
            b.CallExplicit(sampler_ty, core::BuiltinFn::kGetResource, Vector{sampler_ty}, 1_u);

        b.Call(ty.vec4<f32>(), core::BuiltinFn::kTextureSample, tex, sam,
               b.Splat(ty.vec2<f32>(), 0_f));

        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %texture:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(3, 2)
}

%foo = func():void {
  $B2: {
    %3:texture_2d<f32> = load %texture
    %4:sampler = getResource<sampler> 1u
    %5:vec4<f32> = textureSample %3, %4, vec2<f32>(0.0f)
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
  %texture:ptr<handle, texture_2d<f32>, read> = var undef @binding_point(3, 2)
  %2:ptr<handle, resource_table<sampler>, read> = var undef @binding_point(0, 1)
  %tint_resource_table_metadata:ptr<storage, tint_resource_table_metadata_struct, read> = var undef @binding_point(1, 2)
}

%foo = func():void {
  $B2: {
    %5:texture_2d<f32> = load %texture
    %texture_kind:u32 = let 6u
    %7:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
    %8:u32 = load %7
    %9:bool = lt 1u, %8
    %10:bool = if %9 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %11:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %12:u32 = load %11
        %13:u32 = and %12, 65535u
        %14:vec2<u32> = construct %13
        %15:vec2<u32> = construct 40u, 41u
        %16:vec2<bool> = eq %14, %15
        %17:bool = any %16
        exit_if %17  # if_1
      }
      $B4: {  # false
        exit_if false  # if_1
      }
    }
    %sampler_kind:u32 = if %10 [t: $B5, f: $B6] {  # if_2
      $B5: {  # true
        %19:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, 1u
        %20:u32 = load %19
        exit_if %20  # if_2
      }
      $B6: {  # false
        exit_if 41u  # if_2
      }
    }
    %21:u32 = if %10 [t: $B7, f: $B8] {  # if_3
      $B7: {  # true
        exit_if 1u  # if_3
      }
      $B8: {  # false
        %22:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %23:u32 = load %22
        %24:u32 = add 0u, %23
        exit_if %24  # if_3
      }
    }
    %25:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, %21
    %26:u32 = load %25
    %27:u32 = shr %26, 16u
    %28:ptr<handle, sampler, read> = access %2, %27
    %29:sampler = load %28
    %sampler_is_filtering:bool = eq %sampler_kind, 40u
    %use_sampler:bool = if %sampler_is_filtering [t: $B9, f: $B10] {  # if_4
      $B9: {  # true
        %texture_is_unfilterable:bool = switch %texture_kind [c: (default, $B11), c: (11u 16u 1u 21u 26u 6u, $B12)] {  # switch_1
          $B11: {  # case
            exit_switch false  # switch_1
          }
          $B12: {  # case
            exit_switch true  # switch_1
          }
        }
        exit_if %texture_is_unfilterable  # if_4
      }
      $B10: {  # false
        exit_if true  # if_4
      }
    }
    %33:vec4<f32> = if %use_sampler [t: $B13, f: $B14] {  # if_5
      $B13: {  # true
        %34:vec4<f32> = textureSample %5, %29, vec2<f32>(0.0f)
        exit_if %34  # if_5
      }
      $B14: {  # false
        %35:ptr<storage, u32, read> = access %tint_resource_table_metadata, 0u
        %36:u32 = load %35
        %37:u32 = add 0u, %36
        %38:ptr<storage, u32, read> = access %tint_resource_table_metadata, 1u, %37
        %39:u32 = load %38
        %40:u32 = shr %39, 16u
        %41:ptr<handle, sampler, read> = access %2, %40
        %42:sampler = load %41
        %43:vec4<f32> = textureSample %5, %42, vec2<f32>(0.0f)
        exit_if %43  # if_5
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
                    ResourceType::kSampler_non_filtering,
                },
            .get_sampler_index_from_metadata = true,
            .binding_to_resource_type =
                {
                    {BindingPoint{3, 2}, ResourceType::kTexture2d_f32_filterable},
                },
        },
        &helper);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
