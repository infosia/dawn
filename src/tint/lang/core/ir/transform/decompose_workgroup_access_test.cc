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

TEST_F(IR_DecomposeWorkgroupAccessTest, NoWorkgroupAccess) {
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] { b.Return(func); });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ScalarLoad) {
    auto* var = b.Var("v", workgroup, ty.u32());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, u32, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:u32 = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:u32 = load_vector_element %3, 0u
    %5:u32 = bitcast %4
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ScalarStore) {
    auto* var = b.Var("v", workgroup, ty.u32());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, 42_u);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, u32, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    store %v, 42u
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:u32 = bitcast 42u
    store_vector_element %3, 0u, %4
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, VectorLoad) {
    auto* var = b.Var("v", workgroup, ty.vec3u());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, vec3<u32>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec3<u32> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:vec4<u32> = load %3
    %5:vec3<u32> = swizzle %4, xyz
    %6:vec3<u32> = bitcast %5
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, VectorStore) {
    auto* var = b.Var("v", workgroup, ty.vec3u());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Composite(ty.vec3u(), 1_u, 2_u, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, vec3<u32>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    store %v, vec3<u32>(1u, 2u, 3u)
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:u32 = access vec3<u32>(1u, 2u, 3u), 0u
    %4:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %5:u32 = bitcast %3
    store_vector_element %4, 0u, %5
    %6:u32 = access vec3<u32>(1u, 2u, 3u), 1u
    %7:u32 = add 4u, 0u
    %8:u32 = div %7, 16u
    %9:ptr<workgroup, vec4<u32>, read_write> = access %v, %8
    %10:u32 = and %7, 15u
    %11:u32 = shr %10, 2u
    %12:u32 = bitcast %6
    store_vector_element %9, %11, %12
    %13:u32 = access vec3<u32>(1u, 2u, 3u), 2u
    %14:u32 = add 8u, 0u
    %15:u32 = div %14, 16u
    %16:ptr<workgroup, vec4<u32>, read_write> = access %v, %15
    %17:u32 = and %14, 15u
    %18:u32 = shr %17, 2u
    %19:u32 = bitcast %13
    store_vector_element %16, %18, %19
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, MatrixLoad) {
    auto* var = b.Var("v", workgroup, ty.mat2x2<f32>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, mat2x2<f32>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x2<f32> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x2<f32> = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):mat2x2<f32> {
  $B3: {
    %6:u32 = div %start_byte_offset, 16u
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 15u
    %9:u32 = shr %8, 2u
    %10:vec4<u32> = load %7
    %11:vec2<u32> = swizzle %10, zw
    %12:vec2<u32> = swizzle %10, xy
    %13:bool = eq %9, 2u
    %14:vec2<u32> = select %12, %11, %13
    %15:vec2<f32> = bitcast %14
    %16:u32 = add 8u, %start_byte_offset
    %17:u32 = div %16, 16u
    %18:ptr<workgroup, vec4<u32>, read_write> = access %v, %17
    %19:u32 = and %16, 15u
    %20:u32 = shr %19, 2u
    %21:vec4<u32> = load %18
    %22:vec2<u32> = swizzle %21, zw
    %23:vec2<u32> = swizzle %21, xy
    %24:bool = eq %20, 2u
    %25:vec2<u32> = select %23, %22, %24
    %26:vec2<f32> = bitcast %25
    %27:mat2x2<f32> = construct %15, %26
    ret %27
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, MatrixStore) {
    auto* var = b.Var("v", workgroup, ty.mat2x2<f32>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* col1 = b.Construct(ty.vec2<f32>(), 1_f, 2_f);
        auto* col2 = b.Construct(ty.vec2<f32>(), 3_f, 4_f);
        b.Store(var, b.Construct(ty.mat2x2<f32>(), col1, col2));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, mat2x2<f32>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec2<f32> = construct 1.0f, 2.0f
    %4:vec2<f32> = construct 3.0f, 4.0f
    %5:mat2x2<f32> = construct %3, %4
    store %v, %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec2<f32> = construct 1.0f, 2.0f
    %4:vec2<f32> = construct 3.0f, 4.0f
    %5:mat2x2<f32> = construct %3, %4
    %6:void = call %7, 0u, %5
    ret
  }
}
%7 = func(%start_byte_offset:u32, %val:mat2x2<f32>):void {
  $B3: {
    %10:vec2<f32> = access %val, 0u
    %11:f32 = access %10, 0u
    %12:u32 = div %start_byte_offset, 16u
    %13:ptr<workgroup, vec4<u32>, read_write> = access %v, %12
    %14:u32 = and %start_byte_offset, 15u
    %15:u32 = shr %14, 2u
    %16:u32 = bitcast %11
    store_vector_element %13, %15, %16
    %17:f32 = access %10, 1u
    %18:u32 = add 4u, %start_byte_offset
    %19:u32 = div %18, 16u
    %20:ptr<workgroup, vec4<u32>, read_write> = access %v, %19
    %21:u32 = and %18, 15u
    %22:u32 = shr %21, 2u
    %23:u32 = bitcast %17
    store_vector_element %20, %22, %23
    %24:u32 = add 8u, %start_byte_offset
    %25:vec2<f32> = access %val, 1u
    %26:f32 = access %25, 0u
    %27:u32 = div %24, 16u
    %28:ptr<workgroup, vec4<u32>, read_write> = access %v, %27
    %29:u32 = and %24, 15u
    %30:u32 = shr %29, 2u
    %31:u32 = bitcast %26
    store_vector_element %28, %30, %31
    %32:f32 = access %25, 1u
    %33:u32 = add 4u, %24
    %34:u32 = div %33, 16u
    %35:ptr<workgroup, vec4<u32>, read_write> = access %v, %34
    %36:u32 = and %33, 15u
    %37:u32 = shr %36, 2u
    %38:u32 = bitcast %32
    store_vector_element %35, %37, %38
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ArrayLoad) {
    auto* var = b.Var("v", workgroup, ty.array<u32, 4>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Load(var);
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, array<u32, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<u32, 4> = load %v
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<u32, 4> = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):array<u32, 4> {
  $B3: {
    %a:ptr<function, array<u32, 4>, read_write> = var array<u32, 4>(0u)
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
        %9:u32 = mul %idx, 4u
        %10:u32 = add %start_byte_offset, %9
        %11:ptr<function, u32, read_write> = access %a, %idx
        %12:u32 = div %10, 16u
        %13:ptr<workgroup, vec4<u32>, read_write> = access %v, %12
        %14:u32 = and %10, 15u
        %15:u32 = shr %14, 2u
        %16:u32 = load_vector_element %13, %15
        %17:u32 = bitcast %16
        store %11, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    %19:array<u32, 4> = load %a
    ret %19
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, ArrayStore) {
    auto* var = b.Var("v", workgroup, ty.array<u32, 4>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Construct(ty.array<u32, 4>(), 1_u, 2_u, 3_u, 4_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, array<u32, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<u32, 4> = construct 1u, 2u, 3u, 4u
    store %v, %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<u32, 4> = construct 1u, 2u, 3u, 4u
    %4:void = call %5, 0u, %3
    ret
  }
}
%5 = func(%start_byte_offset:u32, %val:array<u32, 4>):void {
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
        %10:u32 = mul %idx, 4u
        %11:u32 = add %start_byte_offset, %10
        %12:u32 = access %val, %idx
        %13:u32 = div %11, 16u
        %14:ptr<workgroup, vec4<u32>, read_write> = access %v, %13
        %15:u32 = and %11, 15u
        %16:u32 = shr %15, 2u
        %17:u32 = bitcast %12
        store_vector_element %14, %16, %17
        continue  # -> $B6
      }
      $B6: {  # continuing
        %18:u32 = add %idx, 1u
        next_iteration %18  # -> $B5
      }
    }
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructLoad) {
    auto* s = ty.Struct(mod.symbols.New("S"), {
                                                  {mod.symbols.New("a"), ty.u32()},
                                                  {mod.symbols.New("b"), ty.f32()},
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
S = struct @align(4) {
  a:u32 @offset(0)
  b:f32 @offset(4)
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
S = struct @align(4) {
  a:u32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):S {
  $B3: {
    %6:u32 = div %start_byte_offset, 16u
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 15u
    %9:u32 = shr %8, 2u
    %10:u32 = load_vector_element %7, %9
    %11:u32 = bitcast %10
    %12:u32 = add 4u, %start_byte_offset
    %13:u32 = div %12, 16u
    %14:ptr<workgroup, vec4<u32>, read_write> = access %v, %13
    %15:u32 = and %12, 15u
    %16:u32 = shr %15, 2u
    %17:u32 = load_vector_element %14, %16
    %18:f32 = bitcast %17
    %19:S = construct %11, %18
    ret %19
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructStore) {
    auto* s = ty.Struct(mod.symbols.New("S"), {
                                                  {mod.symbols.New("a"), ty.u32()},
                                                  {mod.symbols.New("b"), ty.f32()},
                                              });
    auto* var = b.Var("v", workgroup, s);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Store(var, b.Construct(s, 1_u, 2.0_f));
        b.Return(func);
    });

    auto* src = R"(
S = struct @align(4) {
  a:u32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<workgroup, S, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = construct 1u, 2.0f
    store %v, %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(4) {
  a:u32 @offset(0)
  b:f32 @offset(4)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = construct 1u, 2.0f
    %4:void = call %5, 0u, %3
    ret
  }
}
%5 = func(%start_byte_offset:u32, %val:S):void {
  $B3: {
    %8:u32 = access %val, 0u
    %9:u32 = div %start_byte_offset, 16u
    %10:ptr<workgroup, vec4<u32>, read_write> = access %v, %9
    %11:u32 = and %start_byte_offset, 15u
    %12:u32 = shr %11, 2u
    %13:u32 = bitcast %8
    store_vector_element %10, %12, %13
    %14:u32 = add 4u, %start_byte_offset
    %15:f32 = access %val, 1u
    %16:u32 = div %14, 16u
    %17:ptr<workgroup, vec4<u32>, read_write> = access %v, %16
    %18:u32 = and %14, 15u
    %19:u32 = shr %18, 2u
    %20:u32 = bitcast %15
    store_vector_element %17, %19, %20
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:u32 = load_vector_element %3, 0u
    %5:vec2<f16> = bitcast %4
    %6:f16 = access %5, 0u
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:u32 = load_vector_element %3, 0u
    %5:vec2<f16> = bitcast %4
    %6:f16 = access %5, 0u
    %7:f16 = access %5, 1u
    %8:bool = eq 0u, 0u
    %9:f16 = select %6, 1.0h, %8
    %10:f16 = select 1.0h, %7, %8
    %11:vec2<f16> = construct %9, %10
    %12:u32 = bitcast %11
    store_vector_element %3, 0u, %12
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:vec4<u32> = load %3
    %5:vec2<u32> = swizzle %4, xy
    %6:vec4<f16> = bitcast %5
    %7:vec3<f16> = swizzle %6, xyz
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 0u
    %5:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 1u
    %6:f16 = access vec3<f16>(1.0h, 2.0h, 3.0h), 2u
    %7:vec4<f16> = construct %4, %5, %6, 0.0h
    %8:vec2<u32> = bitcast %7
    %9:u32 = access %8, 0u
    %10:u32 = access %8, 1u
    store_vector_element %3, 0u, %9
    store_vector_element %3, 1u, %10
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x2<f16> = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):mat2x2<f16> {
  $B3: {
    %6:u32 = div %start_byte_offset, 16u
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 15u
    %9:u32 = shr %8, 2u
    %10:vec4<u32> = load %7
    %11:u32 = access %10, %9
    %12:vec2<f16> = bitcast %11
    %13:u32 = add 4u, %start_byte_offset
    %14:u32 = div %13, 16u
    %15:ptr<workgroup, vec4<u32>, read_write> = access %v, %14
    %16:u32 = and %13, 15u
    %17:u32 = shr %16, 2u
    %18:vec4<u32> = load %15
    %19:u32 = access %18, %17
    %20:vec2<f16> = bitcast %19
    %21:mat2x2<f16> = construct %12, %20
    ret %21
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
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
    %11:u32 = div %start_byte_offset, 16u
    %12:ptr<workgroup, vec4<u32>, read_write> = access %v, %11
    %13:u32 = and %start_byte_offset, 15u
    %14:u32 = shr %13, 2u
    %15:u32 = bitcast %10
    store_vector_element %12, %14, %15
    %16:u32 = add 4u, %start_byte_offset
    %17:vec2<f16> = access %val, 1u
    %18:u32 = div %16, 16u
    %19:ptr<workgroup, vec4<u32>, read_write> = access %v, %18
    %20:u32 = and %16, 15u
    %21:u32 = shr %20, 2u
    %22:u32 = bitcast %17
    store_vector_element %19, %21, %22
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
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
        %12:u32 = div %10, 16u
        %13:ptr<workgroup, vec4<u32>, read_write> = access %v, %12
        %14:u32 = and %10, 15u
        %15:u32 = shr %14, 2u
        %16:u32 = load_vector_element %13, %15
        %17:u32 = mod %10, 4u
        %18:bool = eq %17, 0u
        %19:u32 = select 1u, 0u, %18
        %20:vec2<f16> = bitcast %16
        %21:f16 = access %20, %19
        store %11, %21
        continue  # -> $B6
      }
      $B6: {  # continuing
        %22:u32 = add %idx, 1u
        next_iteration %22  # -> $B5
      }
    }
    %23:array<f16, 4> = load %a
    ret %23
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
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
        %13:u32 = div %11, 16u
        %14:ptr<workgroup, vec4<u32>, read_write> = access %v, %13
        %15:u32 = and %11, 15u
        %16:u32 = shr %15, 2u
        %17:u32 = mod %11, 4u
        %18:bool = eq %17, 0u
        %19:u32 = select 1u, 0u, %18
        %20:u32 = load_vector_element %14, %16
        %21:vec2<f16> = bitcast %20
        %22:f16 = access %21, 0u
        %23:f16 = access %21, 1u
        %24:bool = eq %19, 0u
        %25:f16 = select %22, %12, %24
        %26:f16 = select %12, %23, %24
        %27:vec2<f16> = construct %25, %26
        %28:u32 = bitcast %27
        store_vector_element %14, %16, %28
        continue  # -> $B6
      }
      $B6: {  # continuing
        %29:u32 = add %idx, 1u
        next_iteration %29  # -> $B5
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:S = call %4, 0u
    ret
  }
}
%4 = func(%start_byte_offset:u32):S {
  $B3: {
    %6:u32 = div %start_byte_offset, 16u
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, %6
    %8:u32 = and %start_byte_offset, 15u
    %9:u32 = shr %8, 2u
    %10:u32 = load_vector_element %7, %9
    %11:u32 = mod %start_byte_offset, 4u
    %12:bool = eq %11, 0u
    %13:u32 = select 1u, 0u, %12
    %14:vec2<f16> = bitcast %10
    %15:f16 = access %14, %13
    %16:u32 = add 2u, %start_byte_offset
    %17:u32 = div %16, 16u
    %18:ptr<workgroup, vec4<u32>, read_write> = access %v, %17
    %19:u32 = and %16, 15u
    %20:u32 = shr %19, 2u
    %21:u32 = load_vector_element %18, %20
    %22:u32 = mod %16, 4u
    %23:bool = eq %22, 0u
    %24:u32 = select 1u, 0u, %23
    %25:vec2<f16> = bitcast %21
    %26:f16 = access %25, %24
    %27:S = construct %15, %26
    ret %27
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
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
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
    %9:u32 = div %start_byte_offset, 16u
    %10:ptr<workgroup, vec4<u32>, read_write> = access %v, %9
    %11:u32 = and %start_byte_offset, 15u
    %12:u32 = shr %11, 2u
    %13:u32 = mod %start_byte_offset, 4u
    %14:bool = eq %13, 0u
    %15:u32 = select 1u, 0u, %14
    %16:u32 = load_vector_element %10, %12
    %17:vec2<f16> = bitcast %16
    %18:f16 = access %17, 0u
    %19:f16 = access %17, 1u
    %20:bool = eq %15, 0u
    %21:f16 = select %18, %8, %20
    %22:f16 = select %8, %19, %20
    %23:vec2<f16> = construct %21, %22
    %24:u32 = bitcast %23
    store_vector_element %10, %12, %24
    %25:u32 = add 2u, %start_byte_offset
    %26:f16 = access %val, 1u
    %27:u32 = div %25, 16u
    %28:ptr<workgroup, vec4<u32>, read_write> = access %v, %27
    %29:u32 = and %25, 15u
    %30:u32 = shr %29, 2u
    %31:u32 = mod %25, 4u
    %32:bool = eq %31, 0u
    %33:u32 = select 1u, 0u, %32
    %34:u32 = load_vector_element %28, %30
    %35:vec2<f16> = bitcast %34
    %36:f16 = access %35, 0u
    %37:f16 = access %35, 1u
    %38:bool = eq %33, 0u
    %39:f16 = select %36, %26, %38
    %40:f16 = select %26, %37, %38
    %41:vec2<f16> = construct %39, %40
    %42:u32 = bitcast %41
    store_vector_element %28, %30, %42
    ret
  }
}
)";
    Run(DecomposeWorkgroupAccess);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
