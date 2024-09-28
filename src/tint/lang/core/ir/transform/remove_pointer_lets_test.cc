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

#include "src/tint/lang/core/ir/transform/remove_pointer_lets.h"

#include <utility>

#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/ir/transform/value_to_let.h"

namespace tint::core::ir::transform {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using IR_RemovePointerLetsTest = TransformTest;

TEST_F(IR_RemovePointerLetsTest, NoModify_NonPointer) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* l = b.Let("x", 2_f);
        b.Let("y", l);
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %x:f32 = let 2.0f
    %y:f32 = let %x
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, Pointer) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* a = b.Var("a", 2_f);
        auto* l = b.Let("x", a);
        b.Let("y", b.Load(l));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, f32, read_write> = var, 2.0f
    %x:ptr<function, f32, read_write> = let %a
    %4:f32 = load %x
    %y:f32 = let %4
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, f32, read_write> = var, 2.0f
    %3:f32 = load %a
    %y:f32 = let %3
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, Chained) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* a = b.Var("a", 2_f);
        auto* x = b.Let("x", a);
        auto* k = b.Let("k", x);
        b.Let("y", b.Load(k));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, f32, read_write> = var, 2.0f
    %x:ptr<function, f32, read_write> = let %a
    %k:ptr<function, f32, read_write> = let %x
    %5:f32 = load %k
    %y:f32 = let %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, f32, read_write> = var, 2.0f
    %3:f32 = load %a
    %y:f32 = let %3
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, PointerIndex) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* a = b.Var("a", ty.ptr<function, array<f32, 2>, read_write>());
        auto* x = b.Let("x", a);
        auto* k = b.Access(ty.ptr<function, f32, read_write>(), x, 1_u);
        b.Let("y", b.Load(k));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<f32, 2>, read_write> = var
    %x:ptr<function, array<f32, 2>, read_write> = let %a
    %4:ptr<function, f32, read_write> = access %x, 1u
    %5:f32 = load %4
    %y:f32 = let %5
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<f32, 2>, read_write> = var
    %3:ptr<function, f32, read_write> = access %a, 1u
    %4:f32 = load %3
    %y:f32 = let %4
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, SimpleChain) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* a = b.Var("a", ty.ptr<function, array<f32, 2>, read_write>());
        auto* x = b.Let("x", a);
        auto* k = b.Access(ty.ptr<function, f32, read_write>(), x, 1_u);
        auto* vp = b.Let("vp", k);
        b.Let("y", b.Load(vp));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<f32, 2>, read_write> = var
    %x:ptr<function, array<f32, 2>, read_write> = let %a
    %4:ptr<function, f32, read_write> = access %x, 1u
    %vp:ptr<function, f32, read_write> = let %4
    %6:f32 = load %vp
    %y:f32 = let %6
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<f32, 2>, read_write> = var
    %3:ptr<function, f32, read_write> = access %a, 1u
    %4:f32 = load %3
    %y:f32 = let %4
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, ComplexChain) {
    auto* func = b.Function("foo", ty.void_());
    b.Append(func->Block(), [&] {
        auto* a = b.Var("a", ty.ptr<function, array<mat4x4<f32>, 2>, read_write>());
        auto* x = b.Let("x", a);
        auto* k = b.Access(ty.ptr<function, mat4x4<f32>, read_write>(), x, 1_u);
        auto* mp = b.Let("mp", k);
        auto* k2 = b.Access(ty.ptr<function, vec4<f32>, read_write>(), mp, 2_u);
        auto* vp = b.Let("vp", k2);
        b.Let("y", b.Load(vp));
        b.Return(func);
    });

    auto* src = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<mat4x4<f32>, 2>, read_write> = var
    %x:ptr<function, array<mat4x4<f32>, 2>, read_write> = let %a
    %4:ptr<function, mat4x4<f32>, read_write> = access %x, 1u
    %mp:ptr<function, mat4x4<f32>, read_write> = let %4
    %6:ptr<function, vec4<f32>, read_write> = access %mp, 2u
    %vp:ptr<function, vec4<f32>, read_write> = let %6
    %8:vec4<f32> = load %vp
    %y:vec4<f32> = let %8
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = func():void {
  $B1: {
    %a:ptr<function, array<mat4x4<f32>, 2>, read_write> = var
    %3:ptr<function, mat4x4<f32>, read_write> = access %a, 1u
    %4:ptr<function, vec4<f32>, read_write> = access %3, 2u
    %5:vec4<f32> = load %4
    %y:vec4<f32> = let %5
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, SavedVars) {
    auto* S = ty.Struct(mod.symbols.New("S"), {
                                                  {mod.symbols.New("i"), ty.i32()},
                                              });

    auto* arr_func = b.Function("arr", ty.void_());
    b.Append(arr_func->Block(), [&] {
        auto* a = b.Var("a", ty.ptr(function, ty.array(S, 2), read_write));
        auto* i = b.Var("i", 0_i);
        auto* j = b.Var("j", 0_i);
        auto* li = b.Load(i);
        auto* lj = b.Load(j);
        auto* idx = b.Add(ty.i32(), li, lj);
        auto* ac = b.Access(ty.ptr<function, i32, read_write>(), a, idx, 0_u);
        auto* p = b.Let("p", ac);
        b.Store(i, 2_i);
        b.Store(p, 4_i);
        b.Return(arr_func);
    });

    auto* mat_func = b.Function("matrix", ty.void_());
    b.Append(mat_func->Block(), [&] {
        auto* m = b.Var("m", ty.ptr(function, ty.mat3x3<f32>(), read_write));
        auto* i = b.Var("i", 0_i);
        auto* j = b.Var("j", 0_i);
        auto* li = b.Load(i);
        auto* lj = b.Load(j);
        auto* idx = b.Add(ty.i32(), li, lj);
        auto* ac = b.Access(ty.ptr<function, vec3<f32>, read_write>(), m, idx);
        auto* p = b.Let("p", ac);
        b.Store(i, 2_i);
        b.Store(p, b.Construct(ty.vec3<f32>(), 4_f, 5_f, 6_f));
        b.Return(mat_func);
    });

    auto* src = R"(
S = struct @align(4) {
  i:i32 @offset(0)
}

%arr = func():void {
  $B1: {
    %a:ptr<function, array<S, 2>, read_write> = var
    %i:ptr<function, i32, read_write> = var, 0i
    %j:ptr<function, i32, read_write> = var, 0i
    %5:i32 = load %i
    %6:i32 = load %j
    %7:i32 = add %5, %6
    %8:ptr<function, i32, read_write> = access %a, %7, 0u
    %p:ptr<function, i32, read_write> = let %8
    store %i, 2i
    store %p, 4i
    ret
  }
}
%matrix = func():void {
  $B2: {
    %m:ptr<function, mat3x3<f32>, read_write> = var
    %i_1:ptr<function, i32, read_write> = var, 0i  # %i_1: 'i'
    %j_1:ptr<function, i32, read_write> = var, 0i  # %j_1: 'j'
    %14:i32 = load %i_1
    %15:i32 = load %j_1
    %16:i32 = add %14, %15
    %17:ptr<function, vec3<f32>, read_write> = access %m, %16
    %p_1:ptr<function, vec3<f32>, read_write> = let %17  # %p_1: 'p'
    store %i_1, 2i
    %19:vec3<f32> = construct 4.0f, 5.0f, 6.0f
    store %p_1, %19
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
S = struct @align(4) {
  i:i32 @offset(0)
}

%arr = func():void {
  $B1: {
    %a:ptr<function, array<S, 2>, read_write> = var
    %i:ptr<function, i32, read_write> = var, 0i
    %j:ptr<function, i32, read_write> = var, 0i
    %5:i32 = load %i
    %6:i32 = load %j
    %7:i32 = add %5, %6
    %8:ptr<function, i32, read_write> = access %a, %7, 0u
    store %i, 2i
    store %8, 4i
    ret
  }
}
%matrix = func():void {
  $B2: {
    %m:ptr<function, mat3x3<f32>, read_write> = var
    %i_1:ptr<function, i32, read_write> = var, 0i  # %i_1: 'i'
    %j_1:ptr<function, i32, read_write> = var, 0i  # %j_1: 'j'
    %13:i32 = load %i_1
    %14:i32 = load %j_1
    %15:i32 = add %13, %14
    %16:ptr<function, vec3<f32>, read_write> = access %m, %15
    store %i_1, 2i
    %17:vec3<f32> = construct 4.0f, 5.0f, 6.0f
    store %16, %17
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

TEST_F(IR_RemovePointerLetsTest, LetsWithFunctionCalls) {
    auto* f = b.Function("f", ty.i32());
    b.Append(f->Block(), [&] { b.Return(f, 0_i); });

    auto* g = b.Function("g", ty.i32());
    b.Append(g->Block(), [&] { b.Return(f, 0_i); });

    auto* foo = b.Function("foo", ty.void_());
    b.Append(foo->Block(), [&] {
        auto* arr = b.Var("arr", ty.ptr<function, array<i32, 4>, read_write>());
        auto* c = b.Call(ty.i32(), f);
        auto* access = b.Access(ty.ptr<function, i32, read_write>(), arr, c);
        auto* p = b.Let("p", access);
        auto* c2 = b.Call(ty.i32(), g);
        b.Let("y", c2);
        b.Let("x", b.Load(p));
        b.Return(foo);
    });

    auto* src = R"(
%f = func():i32 {
  $B1: {
    ret 0i
  }
}
%g = func():i32 {
  $B2: {
    ret 0i
  }
}
%foo = func():void {
  $B3: {
    %arr:ptr<function, array<i32, 4>, read_write> = var
    %5:i32 = call %f
    %6:ptr<function, i32, read_write> = access %arr, %5
    %p:ptr<function, i32, read_write> = let %6
    %8:i32 = call %g
    %y:i32 = let %8
    %10:i32 = load %p
    %x:i32 = let %10
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%f = func():i32 {
  $B1: {
    ret 0i
  }
}
%g = func():i32 {
  $B2: {
    ret 0i
  }
}
%foo = func():void {
  $B3: {
    %arr:ptr<function, array<i32, 4>, read_write> = var
    %5:i32 = call %f
    %6:ptr<function, i32, read_write> = access %arr, %5
    %7:i32 = call %g
    %y:i32 = let %7
    %9:i32 = load %6
    %x:i32 = let %9
    ret
  }
}
)";

    Run(RemovePointerLets);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::core::ir::transform
