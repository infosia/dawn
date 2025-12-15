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

#include "gmock/gmock.h"
#include "src/tint/lang/wgsl/resolver/resolver.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

namespace tint::resolver {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

struct ResolverImmediateVariableTest : public resolver::TestHelper, public testing::Test {};

TEST_F(ResolverImmediateVariableTest, ScalarAccess) {
    // var<immediate> v : u32;
    // fn f() {
    //   let x = v;
    // }
    auto* v = GlobalVar("v", ty.u32(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    EXPECT_EQ(slots.Length(), 1u);
    EXPECT_TRUE(slots[0]);
}

TEST_F(ResolverImmediateVariableTest, VectorWholeAccess) {
    // var<immediate> v : vec4<u32>;
    // fn f() {
    //   let x = v;
    // }
    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    EXPECT_EQ(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, VectorComponentAccess_Index) {
    // var<immediate> v : vec4<u32>;
    // fn f() {
    //   let x = v[1];
    // }
    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", IndexAccessor("v", 1_i))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, VectorComponentAccess_Swizzle) {
    // var<immediate> v : vec4<u32>;
    // fn f() {
    //   let x = v.z;
    // }
    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("v", "z"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, VectorSwizzleMulti) {
    // var<immediate> v : vec4<u32>;
    // fn f() {
    //   let x = v.xz;
    // }
    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("v", "xz"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    EXPECT_EQ(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, MatrixAccess_Column) {
    // var<immediate> v : mat4x4<f32>;
    // fn f() {
    //   let x = v[1];
    // }
    auto* v = GlobalVar("v", ty.mat4x4<f32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", IndexAccessor("v", 1_i))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 16u);
    for (uint32_t i = 0; i < 16; ++i) {
        EXPECT_TRUE(slots[i]);
    }
}

TEST_F(ResolverImmediateVariableTest, MatrixAccess_Element) {
    // var<immediate> v : mat4x4<f32>;
    // fn f() {
    //   let x = v[1][2];
    // }
    auto* v = GlobalVar("v", ty.mat4x4<f32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", IndexAccessor(IndexAccessor("v", 1_i), 2_i))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 16u);
    for (uint32_t i = 0; i < 16; ++i) {
        EXPECT_TRUE(slots[i]);
    }
}

TEST_F(ResolverImmediateVariableTest, StructAccess) {
    // struct S {
    //   a : u32,
    //   b : u32,
    // }
    // var<immediate> v : S;
    // fn f() {
    //   let x = v.b;
    // }
    auto* s = Structure("S", Vector{Member("a", ty.u32()), Member("b", ty.u32())});
    auto* v = GlobalVar("v", ty.Of(s), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("v", "b"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 2u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
}

TEST_F(ResolverImmediateVariableTest, DynamicIndex) {
    // var<immediate> v : vec4<u32>;
    // fn f(i : u32) {
    //   let x = v[i];
    // }
    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);
    auto* i = Param("i", ty.u32());
    Func("f", Vector{i}, ty.void_(),
         Vector{
             Decl(Let("x", IndexAccessor("v", "i"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Dynamic index should mark all slots.
    EXPECT_EQ(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, PaddingBytes) {
    // struct S {
    //   a : u32,
    //   // 28 bytes padding
    //   b : mat4x4<f32>,
    // }
    // var<immediate> v : S;
    // fn f() {
    //   let x = v.a;
    //   let y = v.b;
    // }
    auto* s = Structure("S", Vector{Member("a", ty.u32()),
                                    Member("b", ty.mat4x4<f32>(), Vector{MemberAlign(32_u)})});
    auto* v = GlobalVar("v", ty.Of(s), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("v", "a"))),
             Decl(Let("y", MemberAccessor("v", "b"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // v.a is at offset 0, size 4 (1 slot).
    // v.b is at offset 32, size 64 (16 slots).
    // Padding is from offset 4 to 32 (slots 1 to 7).

    EXPECT_GE(slots.Length(), 24u);  // 32/4 + 64/4 = 8 + 16 = 24
    EXPECT_TRUE(slots[0]);           // a
    for (uint32_t i = 1; i < 8; ++i) {
        EXPECT_FALSE(slots[i]) << "Padding slot " << i << " should not be accessed";
    }
    for (uint32_t i = 8; i < 24; ++i) {
        EXPECT_TRUE(slots[i]) << "b slot " << i << " should be accessed";
    }
}

TEST_F(ResolverImmediateVariableTest, MatrixPadding) {
    // var<immediate> v : mat2x3<f32>;
    // fn f() {
    //   let x = v;
    // }
    // mat2x3<f32>:
    // col 0: offset 0, size 12. Slots 0, 1, 2.
    // padding: offset 12, size 4. Slot 3.
    // col 1: offset 16, size 12. Slots 4, 5, 6.
    // padding: offset 28, size 4. Slot 7.

    auto* v = GlobalVar("v", ty.mat2x3<f32>(), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    EXPECT_GE(slots.Length(), 8u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_FALSE(slots[3]);
    EXPECT_TRUE(slots[4]);
    EXPECT_TRUE(slots[5]);
    EXPECT_TRUE(slots[6]);
    EXPECT_FALSE(slots[7]);
}

TEST_F(ResolverImmediateVariableTest, PartialAccess) {
    // struct S {
    //   a : u32,
    //   b : u32,
    // }
    // var<immediate> v : S;
    // fn f() {
    //   let x = v.a;
    // }
    auto* s = Structure("S", Vector{Member("a", ty.u32()), Member("b", ty.u32())});
    auto* v = GlobalVar("v", ty.Of(s), core::AddressSpace::kImmediate);
    Func("f", tint::Empty, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("v", "a"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 2u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
}

TEST_F(ResolverImmediateVariableTest, FunctionCall_WholeVariable) {
    // var<immediate> v : vec4<u32>;
    // fn foo(p : vec4<u32>) {
    //   let x = p[1];
    // }
    // fn main() {
    //   foo(v);
    // }

    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);

    auto* p = Param("p", ty.vec4<u32>());
    Func("foo", Vector{p}, ty.void_(),
         Vector{
             Decl(Let("x", IndexAccessor("p", 1_i))),
         });

    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("foo", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, FunctionCall_NestedStruct) {
    // struct Inner {
    //   a : u32,
    //   b : u32,
    // }
    // struct Outer {
    //   x : Inner,
    //   y : u32,
    // }
    // var<immediate> v : Outer;
    // fn foo(p : Inner) {
    //   let x = p.b;
    // }
    // fn main() {
    //   foo(v.x);
    // }

    auto* inner = Structure("Inner", Vector{Member("a", ty.u32()), Member("b", ty.u32())});
    auto* outer = Structure("Outer", Vector{Member("x", ty.Of(inner)), Member("y", ty.u32())});
    auto* v = GlobalVar("v", ty.Of(outer), core::AddressSpace::kImmediate);

    auto* p = Param("p", ty.Of(inner));
    Func("foo", Vector{p}, ty.void_(),
         Vector{
             Decl(Let("x", MemberAccessor("p", "b"))),
         });

    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("foo", MemberAccessor("v", "x"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 3u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
}

TEST_F(ResolverImmediateVariableTest, FunctionCall_Chain) {
    // var<immediate> v : vec4<u32>;
    // fn bar(q : u32) {
    //   let x = q;
    // }
    // fn foo(p : vec4<u32>) {
    //   bar(p[2]);
    // }
    // fn main() {
    //   foo(v);
    // }

    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);

    auto* q = Param("q", ty.u32());
    Func("bar", Vector{q}, ty.void_(),
         Vector{
             Decl(Let("x", Expr("q"))),
         });

    auto* p = Param("p", ty.vec4<u32>());
    Func("foo", Vector{p}, ty.void_(),
         Vector{
             CallStmt(Call("bar", IndexAccessor("p", 2_i))),
         });

    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("foo", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, FunctionCall_PartialVariable) {
    // var<immediate> v : vec4<u32>;
    // fn foo(p : u32) {
    //   let x = p;
    // }
    // fn main() {
    //   foo(v.z);
    // }

    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);

    auto* p = Param("p", ty.u32());
    Func("foo", Vector{p}, ty.void_(),
         Vector{
             Decl(Let("x", Expr("p"))),
         });

    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("foo", MemberAccessor("v", "z"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

TEST_F(ResolverImmediateVariableTest, FunctionCall_Nested) {
    // var<immediate> v : vec4<u32>;
    // fn bar(q : u32) {
    //   let x = q;
    // }
    // fn foo(p : vec4<u32>) {
    //   bar(p.y);
    // }
    // fn main() {
    //   foo(v);
    // }

    auto* v = GlobalVar("v", ty.vec4<u32>(), core::AddressSpace::kImmediate);

    auto* q = Param("q", ty.u32());
    Func("bar", Vector{q}, ty.void_(),
         Vector{
             Decl(Let("x", Expr("q"))),
         });

    auto* p = Param("p", ty.vec4<u32>());
    Func("foo", Vector{p}, ty.void_(),
         Vector{
             CallStmt(Call("bar", MemberAccessor("p", "y"))),
         });

    Func("main", tint::Empty, ty.void_(),
         Vector{
             CallStmt(Call("foo", Expr("v"))),
         });

    ASSERT_TRUE(r()->Resolve()) << r()->error();

    auto* sem_v = Sem().Get<sem::GlobalVariable>(v);
    ASSERT_NE(sem_v, nullptr);
    const auto& slots = sem_v->Attributes().immediate_accessible_slots;

    // Any access marks the whole variable.
    EXPECT_GE(slots.Length(), 4u);
    EXPECT_TRUE(slots[0]);
    EXPECT_TRUE(slots[1]);
    EXPECT_TRUE(slots[2]);
    EXPECT_TRUE(slots[3]);
}

}  // namespace
}  // namespace tint::resolver
