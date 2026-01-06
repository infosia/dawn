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

#include "src/tint/lang/core/ir/transform/decompose_memory_access.h"

#include <gtest/gtest.h>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {
namespace {

using IR_DecomposeWorkgroupAccessTest = core::ir::transform::TransformTest;

TEST_F(IR_DecomposeWorkgroupAccessTest, ScalarLoadF16) {
    auto* var = b.Var("v", workgroup, ty.f16());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, f16, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:f16 = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<f16>, read_write> = access %v, 0u
    %4:f16 = load_vector_element %3, 0u
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ScalarStoreF16) {
    auto* var = b.Var("v", workgroup, ty.f16());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, 1.0_h);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, f16, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    store %v, 1.0h
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<f16>, read_write> = access %v, 0u
    store_vector_element %3, 0u, 1.0h
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, VectorLoadF16) {
    auto* var = b.Var("v", workgroup, ty.vec3<f16>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, vec3<f16>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec3<f16> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<f16>, read_write> = access %v, 0u
    %4:vec4<f16> = load %3
    %5:vec3<f16> = swizzle %4, xyz
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, VectorStoreF16) {
    auto* var = b.Var("v", workgroup, ty.vec3<f16>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Composite(ty.vec3<f16>(), 1.0_h, 2.0_h, 3.0_h));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, vec3<f16>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    store %v, vec3<f16>(1.0h, 2.0h, 3.0h)
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<f16>, read_write> = access %v, 0u
    %4:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 0u
    %5:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 1u
    %6:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 2u
    %7:vec4<f16> = construct %4, %5, %6, 0.0h
    store %3, %7
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, MatrixLoadF16) {
    auto* var = b.Var("v", workgroup, ty.mat2x2<f16>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, mat2x2<f16>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x2<f16> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x2<f16> = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):mat2x2<f16> {
  $B3: {
    %6:u32 = div %start_byte_offset, 8u
    %7:ptr<workgroup, vec4<f16>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 7u
    %9:u32 = shr %8, 1u
    %10:vec4<f16> = load %7
    %11:vec2<f16> = swizzle %10, zw
    %12:vec2<f16> = swizzle %10, xy
    %13:bool = gte %9, 2u
    %14:vec2<f16> = select %12, %11, %13
    %15:u32 = add 4u, %start_byte_offset
    %16:u32 = div %15, 8u
    %17:ptr<workgroup, vec4<f16>, read_write> = access %v, %16
    %18:u32 = and %15, 7u
    %19:u32 = shr %18, 1u
    %20:vec4<f16> = load %17
    %21:vec2<f16> = swizzle %20, zw
    %22:vec2<f16> = swizzle %20, xy
    %23:bool = gte %19, 2u
    %24:vec2<f16> = select %22, %21, %23
    %25:mat2x2<f16> = construct %14, %24
    ret %25
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, MatrixStoreF16) {
    auto* var = b.Var("v", workgroup, ty.mat2x2<f16>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* col1 = b.Construct(ty.vec2<f16>(), 1_h, 2_h);
        auto* col2 = b.Construct(ty.vec2<f16>(), 3_h, 4_h);
        b.Store(var, b.Construct(ty.mat2x2<f16>(), col1, col2));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, mat2x2<f16>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec2<f16> = construct 1.0h, 2.0h
    %4:vec2<f16> = construct 3.0h, 4.0h
    %5:mat2x2<f16> = construct %3, %4
    store %v, %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec2<f16> = construct 1.0h, 2.0h
    %4:vec2<f16> = construct 3.0h, 4.0h
    %5:mat2x2<f16> = construct %3, %4
    %6:void = call %7, 0u, %5
    ret
  }
}
%7 = func(%start_byte_offset:u32, %val:mat2x2<f16>):void {
  $B3: {
    %10:vec2<f16> = access %val, 0u
    %11:u32 = div %start_byte_offset, 8u
    %12:ptr<workgroup, vec4<f16>, read_write> = access %v, %11
    %13:u32 = and %start_byte_offset, 7u
    %14:u32 = shr %13, 1u
    %15:f16 = access %10, 0u
    %16:f16 = access %10, 1u
    store_vector_element %12, %14, %15
    %17:u32 = add %14, 1u
    store_vector_element %12, %17, %16
    %18:u32 = add 4u, %start_byte_offset
    %19:vec2<f16> = access %val, 1u
    %20:u32 = div %18, 8u
    %21:ptr<workgroup, vec4<f16>, read_write> = access %v, %20
    %22:u32 = and %18, 7u
    %23:u32 = shr %22, 1u
    %24:f16 = access %19, 0u
    %25:f16 = access %19, 1u
    store_vector_element %21, %23, %24
    %26:u32 = add %23, 1u
    store_vector_element %21, %26, %25
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ArrayLoadF16) {
    auto* var = b.Var("v", workgroup, ty.array<f16, 4>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, array<f16, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<f16, 4> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<f16, 4> = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):array<f16, 4> {
  $B3: {
    %a:ptr<function, array<f16, 4>, read_write> = var array<f16, 4>(0.0h)
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %8:bool = gte %idx, 4u
        if %8 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %9:u32 = mul %idx, 2u
        %10:u32 = add %start_byte_offset, %9
        %11:ptr<function, f16, read_write> = access %a, %idx
        %12:u32 = div %10, 8u
        %13:ptr<workgroup, vec4<f16>, read_write> = access %v, %12
        %14:u32 = and %10, 7u
        %15:u32 = shr %14, 1u
        %16:f16 = load_vector_element %13, %15
        store %11, %16
        continue  # -> $B6
      }
      $B6: {  # continuing
        %17:u32 = add %idx, 1u
        next_iteration %17  # -> $B5
      }
    }
    %18:array<f16, 4> = load %a
    ret %18
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ArrayStoreF16) {
    auto* var = b.Var("v", workgroup, ty.array<f16, 4>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Construct(ty.array<f16, 4>(), 1_h, 2_h, 3_h, 4_h));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, array<f16, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<f16, 4> = construct 1.0h, 2.0h, 3.0h, 4.0h
    store %v, %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<f16, 4> = construct 1.0h, 2.0h, 3.0h, 4.0h
    %4:void = call %5, 0u, %3
    ret
  }
}
%5 = func(%start_byte_offset:u32, %val:array<f16, 4>):void {
  $B3: {
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %9:bool = gte %idx, 4u
        if %9 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %10:u32 = mul %idx, 2u
        %11:u32 = add %start_byte_offset, %10
        %12:f16 = access %val, %idx
        %13:u32 = div %11, 8u
        %14:ptr<workgroup, vec4<f16>, read_write> = access %v, %13
        %15:u32 = and %11, 7u
        %16:u32 = shr %15, 1u
        store_vector_element %14, %16, %12
        continue  # -> $B6
      }
      $B6: {  # continuing
        %17:u32 = add %idx, 1u
        next_iteration %17  # -> $B5
      }
    }
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructLoadF16) {
    auto* s = ty.Struct(mod.symbols.New("S"), {
                                                  {mod.symbols.New("a"), ty.f16()},
                                                  {mod.symbols.New("b"), ty.f16()},
                                              });
    auto* var = b.Var("v", workgroup, s);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
S = struct @align(2) {
  a:f16 @offset(0)
  b:f16 @offset(2)
}

$B1: {  # root
  %v:ptr<workgroup, S, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(2) {
  a:f16 @offset(0)
  b:f16 @offset(2)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):S {
  $B3: {
    %6:u32 = div %start_byte_offset, 8u
    %7:ptr<workgroup, vec4<f16>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 7u
    %9:u32 = shr %8, 1u
    %10:f16 = load_vector_element %7, %9
    %11:u32 = add 2u, %start_byte_offset
    %12:u32 = div %11, 8u
    %13:ptr<workgroup, vec4<f16>, read_write> = access %v, %12
    %14:u32 = and %11, 7u
    %15:u32 = shr %14, 1u
    %16:f16 = load_vector_element %13, %15
    %17:S = construct %10, %16
    ret %17
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructStoreF16) {
    auto* s = ty.Struct(mod.symbols.New("S"), {
                                                  {mod.symbols.New("a"), ty.f16()},
                                                  {mod.symbols.New("b"), ty.f16()},
                                              });
    auto* var = b.Var("v", workgroup, s);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Construct(s, 1_h, 2_h));
        b.Return(func);
    });

    auto* src = R"(
S = struct @align(2) {
  a:f16 @offset(0)
  b:f16 @offset(2)
}

$B1: {  # root
  %v:ptr<workgroup, S, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = construct 1.0h, 2.0h
    store %v, %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(2) {
  a:f16 @offset(0)
  b:f16 @offset(2)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = construct 1.0h, 2.0h
    %4:void = call %5, 0u, %3
    ret
  }
}
%5 = func(%start_byte_offset:u32, %val:S):void {
  $B3: {
    %8:f16 = access %val, 0u
    %9:u32 = div %start_byte_offset, 8u
    %10:ptr<workgroup, vec4<f16>, read_write> = access %v, %9
    %11:u32 = and %start_byte_offset, 7u
    %12:u32 = shr %11, 1u
    store_vector_element %10, %12, %8
    %13:u32 = add 2u, %start_byte_offset
    %14:f16 = access %val, 1u
    %15:u32 = div %13, 8u
    %16:ptr<workgroup, vec4<f16>, read_write> = access %v, %15
    %17:u32 = and %13, 7u
    %18:u32 = shr %17, 1u
    store_vector_element %16, %18, %14
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructNestedLoadF16) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f16>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f16>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f16()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", workgroup, SB);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        auto* access = b.Access(ty.ptr(workgroup, ty.vec3<f16>()), var, 1_u, 1_u, 1_u, 3_u);
        b.Let("b", b.LoadVectorElement(access, 2_u));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(8) {
  s:mat3x3<f16> @offset(0)
  t:array<vec3<f16>, 5> @offset(24)
}

Outer = struct @align(8) {
  x:f16 @offset(0)
  y:Inner @offset(8)
}

SB = struct @align(8) {
  a:i32 @offset(0)
  b:Outer @offset(8)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<workgroup, vec3<f16>, read_write> = access %v, 1u, 1u, 1u, 3u
    %6:f16 = load_vector_element %5, 2u
    %b:f16 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(8) {
  s:mat3x3<f16> @offset(0)
  t:array<vec3<f16>, 5> @offset(24)
}

Outer = struct @align(8) {
  x:f16 @offset(0)
  y:Inner @offset(8)
}

SB = struct @align(8) {
  a:i32 @offset(0)
  b:Outer @offset(8)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 10>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:ptr<workgroup, vec4<f16>, read_write> = access %v, 8u
    %7:f16 = load_vector_element %6, 2u
    %b:f16 = let %7
    ret
  }
}
%4 = func(%start_byte_offset:u32):SB {
  $B3: {
    %10:u32 = div %start_byte_offset, 8u
    %11:ptr<workgroup, vec4<f16>, read_write> = access %v, %10
    %12:u32 = and %start_byte_offset, 7u
    %13:u32 = shr %12, 1u
    %14:f16 = load_vector_element %11, %13
    %15:u32 = add %13, 1u
    %16:f16 = load_vector_element %11, %15
    %17:vec2<f16> = construct %14, %16
    %18:i32 = bitcast %17
    %19:u32 = add 8u, %start_byte_offset
    %20:Outer = call %21, %19
    %22:SB = construct %18, %20
    ret %22
  }
}
%21 = func(%start_byte_offset_1:u32):Outer {  # %start_byte_offset_1: 'start_byte_offset'
  $B4: {
    %24:u32 = div %start_byte_offset_1, 8u
    %25:ptr<workgroup, vec4<f16>, read_write> = access %v, %24
    %26:u32 = and %start_byte_offset_1, 7u
    %27:u32 = shr %26, 1u
    %28:f16 = load_vector_element %25, %27
    %29:u32 = add 8u, %start_byte_offset_1
    %30:Inner = call %31, %29
    %32:Outer = construct %28, %30
    ret %32
  }
}
%31 = func(%start_byte_offset_2:u32):Inner {  # %start_byte_offset_2: 'start_byte_offset'
  $B5: {
    %34:mat3x3<f16> = call %35, %start_byte_offset_2
    %36:u32 = add 24u, %start_byte_offset_2
    %37:array<vec3<f16>, 5> = call %38, %36
    %39:Inner = construct %34, %37
    ret %39
  }
}
%35 = func(%start_byte_offset_3:u32):mat3x3<f16> {  # %start_byte_offset_3: 'start_byte_offset'
  $B6: {
    %41:u32 = div %start_byte_offset_3, 8u
    %42:ptr<workgroup, vec4<f16>, read_write> = access %v, %41
    %43:u32 = and %start_byte_offset_3, 7u
    %44:u32 = shr %43, 1u
    %45:vec4<f16> = load %42
    %46:vec3<f16> = swizzle %45, xyz
    %47:u32 = add 8u, %start_byte_offset_3
    %48:u32 = div %47, 8u
    %49:ptr<workgroup, vec4<f16>, read_write> = access %v, %48
    %50:u32 = and %47, 7u
    %51:u32 = shr %50, 1u
    %52:vec4<f16> = load %49
    %53:vec3<f16> = swizzle %52, xyz
    %54:u32 = add 16u, %start_byte_offset_3
    %55:u32 = div %54, 8u
    %56:ptr<workgroup, vec4<f16>, read_write> = access %v, %55
    %57:u32 = and %54, 7u
    %58:u32 = shr %57, 1u
    %59:vec4<f16> = load %56
    %60:vec3<f16> = swizzle %59, xyz
    %61:mat3x3<f16> = construct %46, %53, %60
    ret %61
  }
}
%38 = func(%start_byte_offset_4:u32):array<vec3<f16>, 5> {  # %start_byte_offset_4: 'start_byte_offset'
  $B7: {
    %a_1:ptr<function, array<vec3<f16>, 5>, read_write> = var array<vec3<f16>, 5>(vec3<f16>(0.0h))  # %a_1: 'a'
    loop [i: $B8, b: $B9, c: $B10] {  # loop_1
      $B8: {  # initializer
        next_iteration 0u  # -> $B9
      }
      $B9 (%idx:u32): {  # body
        %65:bool = gte %idx, 5u
        if %65 [t: $B11] {  # if_1
          $B11: {  # true
            exit_loop  # loop_1
          }
        }
        %66:u32 = mul %idx, 8u
        %67:u32 = add %start_byte_offset_4, %66
        %68:ptr<function, vec3<f16>, read_write> = access %a_1, %idx
        %69:u32 = div %67, 8u
        %70:ptr<workgroup, vec4<f16>, read_write> = access %v, %69
        %71:u32 = and %67, 7u
        %72:u32 = shr %71, 1u
        %73:vec4<f16> = load %70
        %74:vec3<f16> = swizzle %73, xyz
        store %68, %74
        continue  # -> $B10
      }
      $B10: {  # continuing
        %75:u32 = add %idx, 1u
        next_iteration %75  # -> $B9
      }
    }
    %76:array<vec3<f16>, 5> = load %a_1
    ret %76
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructNestedStoreF16) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f16>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f16>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f16()},
                                                          {mod.symbols.New("y"), Inner},
                                                      });

    auto* SB = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("a"), ty.i32()},
                                                    {mod.symbols.New("b"), Outer},
                                                });

    auto* var = b.Var("v", workgroup, SB);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* access = b.Access(ty.ptr(workgroup, ty.vec3<f16>()), var, 1_u, 1_u, 1_u, 3_u);
        b.StoreVectorElement(access, 2_u, b.Constant(42_h));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(8) {
  s:mat3x3<f16> @offset(0)
  t:array<vec3<f16>, 5> @offset(24)
}

Outer = struct @align(8) {
  x:f16 @offset(0)
  y:Inner @offset(8)
}

SB = struct @align(8) {
  a:i32 @offset(0)
  b:Outer @offset(8)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec3<f16>, read_write> = access %v, 1u, 1u, 1u, 3u
    store_vector_element %3, 2u, 42.0h
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(8) {
  s:mat3x3<f16> @offset(0)
  t:array<vec3<f16>, 5> @offset(24)
}

Outer = struct @align(8) {
  x:f16 @offset(0)
  y:Inner @offset(8)
}

SB = struct @align(8) {
  a:i32 @offset(0)
  b:Outer @offset(8)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 10>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<f16>, read_write> = access %v, 8u
    store_vector_element %3, 2u, 42.0h
    ret
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
