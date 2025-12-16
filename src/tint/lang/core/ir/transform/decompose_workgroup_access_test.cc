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
    auto* var = b.Var<workgroup, vec4<f32>>("v");
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        b.Let("b", b.LoadVectorElement(var, 0_u));
        b.Let("c", b.LoadVectorElement(var, 1_u));
        b.Let("d", b.LoadVectorElement(var, 2_u));
        b.Let("e", b.LoadVectorElement(var, 3_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, vec4<f32>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec4<f32> = load %v
    %a:vec4<f32> = let %3
    %5:f32 = load_vector_element %v, 0u
    %b:f32 = let %5
    %7:f32 = load_vector_element %v, 1u
    %c:f32 = let %7
    %9:f32 = load_vector_element %v, 2u
    %d:f32 = let %9
    %11:f32 = load_vector_element %v, 3u
    %e:f32 = let %11
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 1>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %4:vec4<u32> = load %3
    %5:vec4<f32> = bitcast %4
    %a:vec4<f32> = let %5
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %8:u32 = load_vector_element %7, 0u
    %9:f32 = bitcast %8
    %b:f32 = let %9
    %11:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %12:u32 = load_vector_element %11, 1u
    %13:f32 = bitcast %12
    %c:f32 = let %13
    %15:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %16:u32 = load_vector_element %15, 2u
    %17:f32 = bitcast %16
    %d:f32 = let %17
    %19:ptr<workgroup, vec4<u32>, read_write> = access %v, 0u
    %20:u32 = load_vector_element %19, 3u
    %21:f32 = bitcast %20
    %e:f32 = let %21
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

TEST_F(IR_DecomposeWorkgroupAccessTest, AccessChainFromUnnamedAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                          {mod.symbols.New("d"), ty.u32()},
                                                      });

    tint::Vector<const core::type::StructMember*, 2> members;
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("a"), ty.i32(), 0u, 0u, 4u,
                                                  ty.i32()->Size(), core::IOAttributes{}));
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("b"), Inner, 1u, 16u, 16u,
                                                  Inner->Size(), core::IOAttributes{}));
    auto* sb = ty.Struct(mod.symbols.New("SB"), members);

    auto* var = b.Var("v", workgroup, ty.array(sb, 4));
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* access1 = b.Access(ty.ptr(workgroup, sb), var, 2_u);
        auto* access2 = b.Access(ty.ptr(workgroup, Inner), access1, 1_u);
        auto* access3 = b.Access(ty.ptr(workgroup, ty.u32()), access2, 1_u);
        b.Let("x", b.Load(access3));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<SB, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, SB, read_write> = access %v, 2u
    %4:ptr<workgroup, Inner, read_write> = access %3, 1u
    %5:ptr<workgroup, u32, read_write> = access %4, 1u
    %6:u32 = load %5
    %x:u32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
  d:u32 @offset(4)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 8>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 5u
    %4:u32 = load_vector_element %3, 1u
    %5:u32 = bitcast %4
    %x:u32 = let %5
    ret
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, AccessChainFromLetAccessChain) {
    auto* Inner = ty.Struct(mod.symbols.New("Inner"), {
                                                          {mod.symbols.New("c"), ty.f32()},
                                                      });

    tint::Vector<const core::type::StructMember*, 2> members;
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("a"), ty.i32(), 0u, 0u, 4u,
                                                  ty.i32()->Size(), core::IOAttributes{}));
    members.Push(ty.Get<core::type::StructMember>(mod.symbols.New("b"), Inner, 1u, 16u, 16u,
                                                  Inner->Size(), core::IOAttributes{}));
    auto* sb = ty.Struct(mod.symbols.New("SB"), members);

    auto* var = b.Var("v", workgroup, sb);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* x = b.Let("x", var);
        auto* access1 = b.Access(ty.ptr(workgroup, Inner), x, 1_u);
        auto* y = b.Let("y", access1);
        auto* access2 = b.Access(ty.ptr(workgroup, ty.f32()), y, 0_u);
        auto* z = b.Let("z", access2);
        b.Let("a", b.Load(z));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %x:ptr<workgroup, SB, read_write> = let %v
    %4:ptr<workgroup, Inner, read_write> = access %x, 1u
    %y:ptr<workgroup, Inner, read_write> = let %4
    %6:ptr<workgroup, f32, read_write> = access %y, 0u
    %z:ptr<workgroup, f32, read_write> = let %6
    %8:f32 = load %z
    %a:f32 = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(4) {
  c:f32 @offset(0)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Inner @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 2>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 1u
    %4:u32 = load_vector_element %3, 0u
    %5:f32 = bitcast %4
    %a:f32 = let %5
    ret
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, Mat2x3F16Load) {
    auto* var = b.Var("v", workgroup, ty.mat2x3<f16>());
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        b.Let("a", b.Load(var));
        auto* col_ptr = b.Access(ty.ptr(workgroup, ty.vec3<f16>()), var, 1_u);
        b.Let("b", b.Load(col_ptr));
        b.Let("c", b.LoadVectorElement(col_ptr, 2_u));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %v:ptr<workgroup, mat2x3<f16>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x3<f16> = load %v
    %a:mat2x3<f16> = let %3
    %5:ptr<workgroup, vec3<f16>, read_write> = access %v, 1u
    %6:vec3<f16> = load %5
    %b:vec3<f16> = let %6
    %8:f16 = load_vector_element %5, 2u
    %c:f16 = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %v:ptr<workgroup, array<vec4<f16>, 2>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:mat2x3<f16> = call %4, 0u
    %a:mat2x3<f16> = let %3
    %6:ptr<workgroup, vec4<f16>, read_write> = access %v, 1u
    %7:vec4<f16> = load %6
    %8:vec3<f16> = swizzle %7, xyz
    %b:vec3<f16> = let %8
    %10:ptr<workgroup, vec4<f16>, read_write> = access %v, 1u
    %11:f16 = load_vector_element %10, 2u
    %c:f16 = let %11
    ret
  }
}
%4 = func(%start_byte_offset:u32):mat2x3<f16> {
  $B3: {
    %14:u32 = div %start_byte_offset, 8u
    %15:ptr<workgroup, vec4<f16>, read_write> = access %v, %14
    %16:u32 = and %start_byte_offset, 7u
    %17:u32 = shr %16, 1u
    %18:vec4<f16> = load %15
    %19:vec3<f16> = swizzle %18, xyz
    %20:u32 = add 8u, %start_byte_offset
    %21:u32 = div %20, 8u
    %22:ptr<workgroup, vec4<f16>, read_write> = access %v, %21
    %23:u32 = and %20, 7u
    %24:u32 = shr %23, 1u
    %25:vec4<f16> = load %22
    %26:vec3<f16> = swizzle %25, xyz
    %27:mat2x3<f16> = construct %19, %26
    ret %27
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, AccessChainReused) {
    auto* sb = ty.Struct(mod.symbols.New("SB"), {
                                                    {mod.symbols.New("c"), ty.f32()},
                                                    {mod.symbols.New("d"), ty.vec3f()},
                                                });

    auto* var = b.Var("v", workgroup, sb);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* access = b.Access(ty.ptr(workgroup, ty.vec3f()), var, 1_u);
        b.Let("b", b.LoadVectorElement(access, 1_u));
        b.Let("c", b.LoadVectorElement(access, 2_u));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  c:f32 @offset(0)
  d:vec3<f32> @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec3<f32>, read_write> = access %v, 1u
    %4:f32 = load_vector_element %3, 1u
    %b:f32 = let %4
    %6:f32 = load_vector_element %3, 2u
    %c:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(16) {
  c:f32 @offset(0)
  d:vec3<f32> @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 2>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 1u
    %4:u32 = load_vector_element %3, 1u
    %5:f32 = bitcast %4
    %b:f32 = let %5
    %7:ptr<workgroup, vec4<u32>, read_write> = access %v, 1u
    %8:u32 = load_vector_element %7, 2u
    %9:f32 = bitcast %8
    %c:f32 = let %9
    ret
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructNestedLoad) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
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
        auto* access = b.Access(ty.ptr(workgroup, ty.vec3f()), var, 1_u, 1_u, 1_u, 3_u);
        b.Let("b", b.LoadVectorElement(access, 2_u));
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:SB = load %v
    %a:SB = let %3
    %5:ptr<workgroup, vec3<f32>, read_write> = access %v, 1u, 1u, 1u, 3u
    %6:f32 = load_vector_element %5, 2u
    %b:f32 = let %6
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 10>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:SB = call %4, 0u
    %a:SB = let %3
    %6:ptr<workgroup, vec4<u32>, read_write> = access %v, 8u
    %7:u32 = load_vector_element %6, 2u
    %8:f32 = bitcast %7
    %b:f32 = let %8
    ret
  }
}
%4 = func(%start_byte_offset:u32):SB {
  $B3: {
    %11:u32 = div %start_byte_offset, 16u
    %12:ptr<workgroup, vec4<u32>, read_write> = access %v, %11
    %13:u32 = and %start_byte_offset, 15u
    %14:u32 = shr %13, 2u
    %15:u32 = load_vector_element %12, %14
    %16:i32 = bitcast %15
    %17:u32 = add 16u, %start_byte_offset
    %18:Outer = call %19, %17
    %20:SB = construct %16, %18
    ret %20
  }
}
%19 = func(%start_byte_offset_1:u32):Outer {  # %start_byte_offset_1: 'start_byte_offset'
  $B4: {
    %22:u32 = div %start_byte_offset_1, 16u
    %23:ptr<workgroup, vec4<u32>, read_write> = access %v, %22
    %24:u32 = and %start_byte_offset_1, 15u
    %25:u32 = shr %24, 2u
    %26:u32 = load_vector_element %23, %25
    %27:f32 = bitcast %26
    %28:u32 = add 16u, %start_byte_offset_1
    %29:Inner = call %30, %28
    %31:Outer = construct %27, %29
    ret %31
  }
}
%30 = func(%start_byte_offset_2:u32):Inner {  # %start_byte_offset_2: 'start_byte_offset'
  $B5: {
    %33:mat3x3<f32> = call %34, %start_byte_offset_2
    %35:u32 = add 48u, %start_byte_offset_2
    %36:array<vec3<f32>, 5> = call %37, %35
    %38:Inner = construct %33, %36
    ret %38
  }
}
%34 = func(%start_byte_offset_3:u32):mat3x3<f32> {  # %start_byte_offset_3: 'start_byte_offset'
  $B6: {
    %40:u32 = div %start_byte_offset_3, 16u
    %41:ptr<workgroup, vec4<u32>, read_write> = access %v, %40
    %42:vec4<u32> = load %41
    %43:vec3<u32> = swizzle %42, xyz
    %44:vec3<f32> = bitcast %43
    %45:u32 = add 16u, %start_byte_offset_3
    %46:u32 = div %45, 16u
    %47:ptr<workgroup, vec4<u32>, read_write> = access %v, %46
    %48:vec4<u32> = load %47
    %49:vec3<u32> = swizzle %48, xyz
    %50:vec3<f32> = bitcast %49
    %51:u32 = add 32u, %start_byte_offset_3
    %52:u32 = div %51, 16u
    %53:ptr<workgroup, vec4<u32>, read_write> = access %v, %52
    %54:vec4<u32> = load %53
    %55:vec3<u32> = swizzle %54, xyz
    %56:vec3<f32> = bitcast %55
    %57:mat3x3<f32> = construct %44, %50, %56
    ret %57
  }
}
%37 = func(%start_byte_offset_4:u32):array<vec3<f32>, 5> {  # %start_byte_offset_4: 'start_byte_offset'
  $B7: {
    %a_1:ptr<function, array<vec3<f32>, 5>, read_write> = var array<vec3<f32>, 5>(vec3<f32>(0.0f))  # %a_1: 'a'
    loop [i: $B8, b: $B9, c: $B10] {  # loop_1
      $B8: {  # initializer
        next_iteration 0u  # -> $B9
      }
      $B9 (%idx:u32): {  # body
        %61:bool = gte %idx, 5u
        if %61 [t: $B11] {  # if_1
          $B11: {  # true
            exit_loop  # loop_1
          }
        }
        %62:u32 = mul %idx, 16u
        %63:u32 = add %start_byte_offset_4, %62
        %64:ptr<function, vec3<f32>, read_write> = access %a_1, %idx
        %65:u32 = div %63, 16u
        %66:ptr<workgroup, vec4<u32>, read_write> = access %v, %65
        %67:vec4<u32> = load %66
        %68:vec3<u32> = swizzle %67, xyz
        %69:vec3<f32> = bitcast %68
        store %64, %69
        continue  # -> $B10
      }
      $B10: {  # continuing
        %70:u32 = add %idx, 1u
        next_iteration %70  # -> $B9
      }
    }
    %71:array<vec3<f32>, 5> = load %a_1
    ret %71
  }
}
)";
    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, StructNestedStore) {
    auto* Inner =
        ty.Struct(mod.symbols.New("Inner"), {
                                                {mod.symbols.New("s"), ty.mat3x3<f32>()},
                                                {mod.symbols.New("t"), ty.array<vec3<f32>, 5>()},
                                            });
    auto* Outer = ty.Struct(mod.symbols.New("Outer"), {
                                                          {mod.symbols.New("x"), ty.f32()},
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
        auto* access = b.Access(ty.ptr(workgroup, ty.vec3f()), var, 1_u, 1_u, 1_u, 3_u);
        b.StoreVectorElement(access, 2_u, 42.0_f);
        b.Return(func);
    });

    auto* src = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec3<f32>, read_write> = access %v, 1u, 1u, 1u, 3u
    store_vector_element %3, 2u, 42.0f
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
Inner = struct @align(16) {
  s:mat3x3<f32> @offset(0)
  t:array<vec3<f32>, 5> @offset(48)
}

Outer = struct @align(16) {
  x:f32 @offset(0)
  y:Inner @offset(16)
}

SB = struct @align(16) {
  a:i32 @offset(0)
  b:Outer @offset(16)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 10>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, vec4<u32>, read_write> = access %v, 8u
    %4:u32 = bitcast 42.0f
    store_vector_element %3, 2u, %4
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

TEST_F(IR_DecomposeWorkgroupAccessTest, Determinism_MultipleUsesOfLetFromVar) {
    auto* sb =
        ty.Struct(mod.symbols.New("SB"), {
                                             {mod.symbols.New("a"), ty.array<vec4<f32>, 2>()},
                                             {mod.symbols.New("b"), ty.array<vec4<i32>, 2>()},
                                         });

    auto* var = b.Var("v", workgroup, sb);
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* let = b.Let("l", var);
        auto* pa = b.Access(ty.ptr(workgroup, ty.array<vec4<f32>, 2>(), core::Access::kReadWrite),
                            let, 0_u);
        b.Let("a", b.Load(pa));
        auto* pb = b.Access(ty.ptr(workgroup, ty.array<vec4<i32>, 2>(), core::Access::kReadWrite),
                            let, 1_u);
        b.Let("b", b.Load(pb));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, SB, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %l:ptr<workgroup, SB, read_write> = let %v
    %4:ptr<workgroup, array<vec4<f32>, 2>, read_write> = access %l, 0u
    %5:array<vec4<f32>, 2> = load %4
    %a:array<vec4<f32>, 2> = let %5
    %7:ptr<workgroup, array<vec4<i32>, 2>, read_write> = access %l, 1u
    %8:array<vec4<i32>, 2> = load %7
    %b:array<vec4<i32>, 2> = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 4>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<vec4<f32>, 2> = call %4, 0u
    %a:array<vec4<f32>, 2> = let %3
    %6:array<vec4<i32>, 2> = call %7, 32u
    %b:array<vec4<i32>, 2> = let %6
    ret
  }
}
%7 = func(%start_byte_offset:u32):array<vec4<i32>, 2> {
  $B3: {
    %a_1:ptr<function, array<vec4<i32>, 2>, read_write> = var array<vec4<i32>, 2>(vec4<i32>(0i))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 2u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:u32 = mul %idx, 16u
        %14:u32 = add %start_byte_offset, %13
        %15:ptr<function, vec4<i32>, read_write> = access %a_1, %idx
        %16:u32 = div %14, 16u
        %17:ptr<workgroup, vec4<u32>, read_write> = access %v, %16
        %18:vec4<u32> = load %17
        %19:vec4<i32> = bitcast %18
        store %15, %19
        continue  # -> $B6
      }
      $B6: {  # continuing
        %20:u32 = add %idx, 1u
        next_iteration %20  # -> $B5
      }
    }
    %21:array<vec4<i32>, 2> = load %a_1
    ret %21
  }
}
%4 = func(%start_byte_offset_1:u32):array<vec4<f32>, 2> {  # %start_byte_offset_1: 'start_byte_offset'
  $B8: {
    %a_2:ptr<function, array<vec4<f32>, 2>, read_write> = var array<vec4<f32>, 2>(vec4<f32>(0.0f))  # %a_2: 'a'
    loop [i: $B9, b: $B10, c: $B11] {  # loop_2
      $B9: {  # initializer
        next_iteration 0u  # -> $B10
      }
      $B10 (%idx_1:u32): {  # body
        %25:bool = gte %idx_1, 2u
        if %25 [t: $B12] {  # if_2
          $B12: {  # true
            exit_loop  # loop_2
          }
        }
        %26:u32 = mul %idx_1, 16u
        %27:u32 = add %start_byte_offset_1, %26
        %28:ptr<function, vec4<f32>, read_write> = access %a_2, %idx_1
        %29:u32 = div %27, 16u
        %30:ptr<workgroup, vec4<u32>, read_write> = access %v, %29
        %31:vec4<u32> = load %30
        %32:vec4<f32> = bitcast %31
        store %28, %32
        continue  # -> $B11
      }
      $B11: {  # continuing
        %33:u32 = add %idx_1, 1u
        next_iteration %33  # -> $B10
      }
    }
    %34:array<vec4<f32>, 2> = load %a_2
    ret %34
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, Determinism_MultipleUsesOfLetFromAccess) {
    auto* sb =
        ty.Struct(mod.symbols.New("SB"), {
                                             {mod.symbols.New("a"), ty.array<vec4<f32>, 2>()},
                                             {mod.symbols.New("b"), ty.array<vec4<i32>, 2>()},
                                         });

    auto* var = b.Var("v", workgroup, ty.array(sb, 2));
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* let = b.Let("l", b.Access(ty.ptr(workgroup, sb, core::Access::kReadWrite), var, 0_u));
        auto* pa = b.Access(ty.ptr(workgroup, ty.array<vec4<f32>, 2>(), core::Access::kReadWrite),
                            let, 0_u);
        b.Let("a", b.Load(pa));
        auto* pb = b.Access(ty.ptr(workgroup, ty.array<vec4<i32>, 2>(), core::Access::kReadWrite),
                            let, 1_u);
        b.Let("b", b.Load(pb));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, array<SB, 2>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, SB, read_write> = access %v, 0u
    %l:ptr<workgroup, SB, read_write> = let %3
    %5:ptr<workgroup, array<vec4<f32>, 2>, read_write> = access %l, 0u
    %6:array<vec4<f32>, 2> = load %5
    %a:array<vec4<f32>, 2> = let %6
    %8:ptr<workgroup, array<vec4<i32>, 2>, read_write> = access %l, 1u
    %9:array<vec4<i32>, 2> = load %8
    %b:array<vec4<i32>, 2> = let %9
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 8>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<vec4<f32>, 2> = call %4, 0u
    %a:array<vec4<f32>, 2> = let %3
    %6:array<vec4<i32>, 2> = call %7, 32u
    %b:array<vec4<i32>, 2> = let %6
    ret
  }
}
%7 = func(%start_byte_offset:u32):array<vec4<i32>, 2> {
  $B3: {
    %a_1:ptr<function, array<vec4<i32>, 2>, read_write> = var array<vec4<i32>, 2>(vec4<i32>(0i))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 2u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:u32 = mul %idx, 16u
        %14:u32 = add %start_byte_offset, %13
        %15:ptr<function, vec4<i32>, read_write> = access %a_1, %idx
        %16:u32 = div %14, 16u
        %17:ptr<workgroup, vec4<u32>, read_write> = access %v, %16
        %18:vec4<u32> = load %17
        %19:vec4<i32> = bitcast %18
        store %15, %19
        continue  # -> $B6
      }
      $B6: {  # continuing
        %20:u32 = add %idx, 1u
        next_iteration %20  # -> $B5
      }
    }
    %21:array<vec4<i32>, 2> = load %a_1
    ret %21
  }
}
%4 = func(%start_byte_offset_1:u32):array<vec4<f32>, 2> {  # %start_byte_offset_1: 'start_byte_offset'
  $B8: {
    %a_2:ptr<function, array<vec4<f32>, 2>, read_write> = var array<vec4<f32>, 2>(vec4<f32>(0.0f))  # %a_2: 'a'
    loop [i: $B9, b: $B10, c: $B11] {  # loop_2
      $B9: {  # initializer
        next_iteration 0u  # -> $B10
      }
      $B10 (%idx_1:u32): {  # body
        %25:bool = gte %idx_1, 2u
        if %25 [t: $B12] {  # if_2
          $B12: {  # true
            exit_loop  # loop_2
          }
        }
        %26:u32 = mul %idx_1, 16u
        %27:u32 = add %start_byte_offset_1, %26
        %28:ptr<function, vec4<f32>, read_write> = access %a_2, %idx_1
        %29:u32 = div %27, 16u
        %30:ptr<workgroup, vec4<u32>, read_write> = access %v, %29
        %31:vec4<u32> = load %30
        %32:vec4<f32> = bitcast %31
        store %28, %32
        continue  # -> $B11
      }
      $B11: {  # continuing
        %33:u32 = add %idx_1, 1u
        next_iteration %33  # -> $B10
      }
    }
    %34:array<vec4<f32>, 2> = load %a_2
    ret %34
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_DecomposeWorkgroupAccessTest, Determinism_MultipleUsesOfAccess) {
    auto* sb =
        ty.Struct(mod.symbols.New("SB"), {
                                             {mod.symbols.New("a"), ty.array<vec4<f32>, 2>()},
                                             {mod.symbols.New("b"), ty.array<vec4<i32>, 2>()},
                                         });

    auto* var = b.Var("v", workgroup, ty.array(sb, 2));
    b.ir.root_block->Append(var);

    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(b.Constant(1_u), b.Constant(1_u), b.Constant(1_u));
    b.Append(func->Block(), [&] {
        auto* access = b.Access(ty.ptr(workgroup, sb, core::Access::kReadWrite), var, 0_u);
        auto* pa = b.Access(ty.ptr(workgroup, ty.array<vec4<f32>, 2>(), core::Access::kReadWrite),
                            access, 0_u);
        b.Let("a", b.Load(pa));
        auto* pb = b.Access(ty.ptr(workgroup, ty.array<vec4<i32>, 2>(), core::Access::kReadWrite),
                            access, 1_u);
        b.Let("b", b.Load(pb));
        b.Return(func);
    });

    auto* src = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, array<SB, 2>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, SB, read_write> = access %v, 0u
    %4:ptr<workgroup, array<vec4<f32>, 2>, read_write> = access %3, 0u
    %5:array<vec4<f32>, 2> = load %4
    %a:array<vec4<f32>, 2> = let %5
    %7:ptr<workgroup, array<vec4<i32>, 2>, read_write> = access %3, 1u
    %8:array<vec4<i32>, 2> = load %7
    %b:array<vec4<i32>, 2> = let %8
    ret
  }
}
)";
    ASSERT_EQ(src, str());

    auto* expect = R"(
SB = struct @align(16) {
  a:array<vec4<f32>, 2> @offset(0)
  b:array<vec4<i32>, 2> @offset(32)
}

$B1: {  # root
  %v:ptr<workgroup, array<vec4<u32>, 8>, read_write> = var undef
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:array<vec4<f32>, 2> = call %4, 0u
    %a:array<vec4<f32>, 2> = let %3
    %6:array<vec4<i32>, 2> = call %7, 32u
    %b:array<vec4<i32>, 2> = let %6
    ret
  }
}
%7 = func(%start_byte_offset:u32):array<vec4<i32>, 2> {
  $B3: {
    %a_1:ptr<function, array<vec4<i32>, 2>, read_write> = var array<vec4<i32>, 2>(vec4<i32>(0i))  # %a_1: 'a'
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        next_iteration 0u  # -> $B5
      }
      $B5 (%idx:u32): {  # body
        %12:bool = gte %idx, 2u
        if %12 [t: $B7] {  # if_1
          $B7: {  # true
            exit_loop  # loop_1
          }
        }
        %13:u32 = mul %idx, 16u
        %14:u32 = add %start_byte_offset, %13
        %15:ptr<function, vec4<i32>, read_write> = access %a_1, %idx
        %16:u32 = div %14, 16u
        %17:ptr<workgroup, vec4<u32>, read_write> = access %v, %16
        %18:vec4<u32> = load %17
        %19:vec4<i32> = bitcast %18
        store %15, %19
        continue  # -> $B6
      }
      $B6: {  # continuing
        %20:u32 = add %idx, 1u
        next_iteration %20  # -> $B5
      }
    }
    %21:array<vec4<i32>, 2> = load %a_1
    ret %21
  }
}
%4 = func(%start_byte_offset_1:u32):array<vec4<f32>, 2> {  # %start_byte_offset_1: 'start_byte_offset'
  $B8: {
    %a_2:ptr<function, array<vec4<f32>, 2>, read_write> = var array<vec4<f32>, 2>(vec4<f32>(0.0f))  # %a_2: 'a'
    loop [i: $B9, b: $B10, c: $B11] {  # loop_2
      $B9: {  # initializer
        next_iteration 0u  # -> $B10
      }
      $B10 (%idx_1:u32): {  # body
        %25:bool = gte %idx_1, 2u
        if %25 [t: $B12] {  # if_2
          $B12: {  # true
            exit_loop  # loop_2
          }
        }
        %26:u32 = mul %idx_1, 16u
        %27:u32 = add %start_byte_offset_1, %26
        %28:ptr<function, vec4<f32>, read_write> = access %a_2, %idx_1
        %29:u32 = div %27, 16u
        %30:ptr<workgroup, vec4<u32>, read_write> = access %v, %29
        %31:vec4<u32> = load %30
        %32:vec4<f32> = bitcast %31
        store %28, %32
        continue  # -> $B11
      }
      $B11: {  # continuing
        %33:u32 = add %idx_1, 1u
        next_iteration %33  # -> $B10
      }
    }
    %34:array<vec4<f32>, 2> = load %a_2
    ret %34
  }
}
)";

    Run(DecomposeWorkgroupAccess);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
