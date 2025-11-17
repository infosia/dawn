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

#include "src/tint/lang/msl/writer/raise/validate_subgroup_matrix.h"

#include "gtest/gtest.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::msl::writer::raise {
namespace {

using MslWriter_ValidateSubgroupMatrixTest = core::ir::transform::TransformTest;

TEST_F(MslWriter_ValidateSubgroupMatrixTest, F16_8x8) {
    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.f16(), 8u, 8u)));
        b.Return(func);
    });

    auto* src = R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %v:ptr<function, subgroup_matrix_left<f16, 8, 8>, read_write> = var undef
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(ValidateSubgroupMatrix);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, F32_8x8) {
    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.f32(), 8u, 8u)));
        b.Return(func);
    });

    auto* src = R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %v:ptr<function, subgroup_matrix_left<f32, 8, 8>, read_write> = var undef
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(ValidateSubgroupMatrix);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, 8x2) {
    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        auto* v = b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.f32(), 8u, 2u)));
        mod.SetSource(v, Source{{1, 2}});
        b.Return(func);
    });

    auto* src = R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %v:ptr<function, subgroup_matrix_left<f32, 8, 2>, read_write> = var undef
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto result = RunWithFailure(ValidateSubgroupMatrix);
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason,
              R"(1:2 error: MSL requires a subgroup_matrix to have dimensions of 8x8)");
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, i8) {
    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        auto* v = b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.i8(), 8u, 8u)));
        mod.SetSource(v, Source{{9, 12}});
        b.Return(func);
    });

    auto* src = R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %v:ptr<function, subgroup_matrix_left<i8, 8, 8>, read_write> = var undef
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto result = RunWithFailure(ValidateSubgroupMatrix);
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason,
              R"(9:12 error: MSL requires a subgroup_matrix to have type `f32` or `f16`)");
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, FunctionParam) {
    auto* f2 = b.Function("f", ty.void_());
    f2->AppendParam(b.FunctionParam("p", ty.subgroup_matrix_left(ty.f32(), 8u, 8u)));
    b.Append(f2->Block(), [&] {  //
        b.Return(f2);
    });

    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        auto* v = b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.f32(), 8u, 8u)));
        b.Call(ty.void_(), f2, b.Load(v));
        b.Return(func);
    });

    auto* src = R"(
%f = func(%p:subgroup_matrix_left<f32, 8, 8>):void {
  $B1: {
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %v:ptr<function, subgroup_matrix_left<f32, 8, 8>, read_write> = var undef
    %5:subgroup_matrix_left<f32, 8, 8> = load %v
    %6:void = call %f, %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(ValidateSubgroupMatrix);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, InvalidFunctionParam_i8) {
    auto* f2 = b.Function("f", ty.void_());
    auto* p = b.FunctionParam("p", ty.subgroup_matrix_left(ty.i8(), 8u, 8u));
    mod.SetSource(p, Source{{3, 4}});
    f2->AppendParam(p);
    b.Append(f2->Block(), [&] {  //
        b.Return(f2);
    });

    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        auto* v = b.Var("v", ty.ptr(function, ty.subgroup_matrix_left(ty.i8(), 8u, 8u)));
        mod.SetSource(v, Source{{9, 12}});
        b.Call(ty.void_(), f2, b.Load(v));
        b.Return(func);
    });

    auto* src = R"(
%f = func(%p:subgroup_matrix_left<i8, 8, 8>):void {
  $B1: {
    ret
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %v:ptr<function, subgroup_matrix_left<i8, 8, 8>, read_write> = var undef
    %5:subgroup_matrix_left<i8, 8, 8> = load %v
    %6:void = call %f, %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto result = RunWithFailure(ValidateSubgroupMatrix);
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason,
              R"(9:12 error: MSL requires a subgroup_matrix to have type `f32` or `f16`)");
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, FunctionReturn) {
    auto* f2 = b.Function("f", ty.subgroup_matrix_left(ty.f32(), 8u, 8u));
    b.Append(f2->Block(), [&] {  //
        b.Return(f2, b.Composite(ty.subgroup_matrix_left(ty.f32(), 8u, 8u), 5_f));
    });

    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        b.Let("v", b.Call(ty.subgroup_matrix_left(ty.f32(), 8u, 8u), f2));
        b.Return(func);
    });

    auto* src = R"(
%f = func():subgroup_matrix_left<f32, 8, 8> {
  $B1: {
    ret subgroup_matrix_left<f32, 8, 8>(5.0f)
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:subgroup_matrix_left<f32, 8, 8> = call %f
    %v:subgroup_matrix_left<f32, 8, 8> = let %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(ValidateSubgroupMatrix);

    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_ValidateSubgroupMatrixTest, InvalidFunctionReturn_i32) {
    auto* f2 = b.Function("f", ty.subgroup_matrix_left(ty.i32(), 8u, 8u));
    b.Append(f2->Block(), [&] {  //
        b.Return(f2, b.Composite(ty.subgroup_matrix_left(ty.i32(), 8u, 8u), 5_i));
    });

    auto* func = b.ComputeFunction("main");
    b.Append(func->Block(), [&] {  //
        auto* c = b.Call(ty.subgroup_matrix_left(ty.i32(), 8u, 8u), f2);
        mod.SetSource(c, Source{{1, 2}});
        b.Let("v", c);
        b.Return(func);
    });

    auto* src = R"(
%f = func():subgroup_matrix_left<i32, 8, 8> {
  $B1: {
    ret subgroup_matrix_left<i32, 8, 8>(5i)
  }
}
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:subgroup_matrix_left<i32, 8, 8> = call %f
    %v:subgroup_matrix_left<i32, 8, 8> = let %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto result = RunWithFailure(ValidateSubgroupMatrix);
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason,
              R"(1:2 error: MSL requires a subgroup_matrix to have type `f32` or `f16`)");
}

}  // namespace
}  // namespace tint::msl::writer::raise
