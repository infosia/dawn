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

#include "src/tint/lang/spirv/reader/lower/atomics.h"

#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/spirv/ir/builtin_call.h"

namespace tint::spirv::reader::lower {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using SpirvReader_AtomicsTest = core::ir::transform::TransformTest;

TEST_F(SpirvReader_AtomicsTest, DISABLED_ArrayStore) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block,
             [&] { wg = b.Var("wg", ty.ptr<workgroup, array<u32, 4>, read_write>()); });

    b.Append(f->Block(), [&] {  //
        auto* a = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a, 1_u, 0_u,
                                       0_u);
        b.Return(f);
    });

    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, array<u32, 4>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 1i
    %4:void = spirv.atomic_store %3, 1u, 0u, 0u
    ret
  }
}
)";
    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ArrayStore_CopiedObject) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block,
             [&] { wg = b.Var("wg", ty.ptr<workgroup, array<u32, 4>, read_write>()); });

    b.Append(f->Block(), [&] {  //
        auto* l = b.Let(wg);
        auto* a = b.Access(ty.ptr<workgroup, u32, read_write>(), l, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a, 1_u, 0_u,
                                       2_u);
        b.Return(f);
    });

    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, array<u32, 4>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, array<u32, 4>, read_write> = let %wg
    %4:ptr<workgroup, u32, read_write> = access %3, 1i
    %5:void = spirv.atomic_store %4, 1u, 0u, 2u
    ret
  }
}
)";
    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ArrayNested) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] {
        wg = b.Var("wg", ty.ptr<workgroup, array<array<array<u32, 1>, 2>, 3>, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_i, 1_i, 0_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a, 2_u, 0_u,
                                       1_u);
        b.Return(f);
    });
    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, array<array<array<u32, 1>, 2>, 3>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 2i, 1i, 0i
    %4:void = spirv.atomic_store %3, 2u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_FlatSingleAtomic) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("x"), ty.i32()},
                                                   {mod.symbols.New("a"), ty.u32()},
                                                   {mod.symbols.New("y"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, sb, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<workgroup, i32, read_write>(), wg, 0_u);
        b.Store(a0, 0_i);
        auto* a1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a1, 2_u, 0_u,
                                       0_u);
        auto* a2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_u);
        b.Store(a2, 0_u);
        b.Return(f);
    });
    auto* src = R"(
S = struct @align(4) {
  x:i32 @offset(0)
  a:u32 @offset(4)
  y:u32 @offset(8)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, i32, read_write> = access %wg, 0u
    store %3, 0i
    %4:ptr<workgroup, u32, read_write> = access %wg, 1u
    %5:void = spirv.atomic_store %4, 2u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 2u
    store %6, 0u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_FlatMultipleAtomics) {
    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("x"), ty.i32()},
                                                   {mod.symbols.New("a"), ty.u32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, sb, read_write)); });

    auto* f = b.ComputeFunction("main");
    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<workgroup, i32, read_write>(), wg, 0_u);
        b.Store(a0, 0_i);
        auto* a1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a1, 2_u, 0_u,
                                       0_u);
        auto* a2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a2, 2_u, 0_u,
                                       0_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  x:i32 @offset(0)
  a:u32 @offset(4)
  b:u32 @offset(8)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, i32, read_write> = access %wg, 0u
    store %3, 0i
    %4:ptr<workgroup, u32, read_write> = access %wg, 1u
    %5:void = spirv.atomic_store %4, 2u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 2u
    %7:void = spirv.atomic_store %6, 2u, 0u, 0u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_Nested) {
    auto* f = b.ComputeFunction("main");

    auto* s0 = ty.Struct(mod.symbols.New("S0"), {
                                                    {mod.symbols.New("x"), ty.i32()},
                                                    {mod.symbols.New("a"), ty.u32()},
                                                    {mod.symbols.New("y"), ty.i32()},
                                                    {mod.symbols.New("z"), ty.i32()},
                                                });
    auto* s1 = ty.Struct(mod.symbols.New("S1"), {
                                                    {mod.symbols.New("x"), ty.i32()},
                                                    {mod.symbols.New("a"), s0},
                                                    {mod.symbols.New("y"), ty.i32()},
                                                    {mod.symbols.New("z"), ty.i32()},
                                                });
    auto* s2 = ty.Struct(mod.symbols.New("S2"), {
                                                    {mod.symbols.New("x"), ty.i32()},
                                                    {mod.symbols.New("y"), ty.i32()},
                                                    {mod.symbols.New("z"), ty.i32()},
                                                    {mod.symbols.New("a"), s1},
                                                });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, s2, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<workgroup, i32, read_write>(), wg, 3_u, 1_u, 0_u);
        b.Store(a0, 0_i);

        auto* a1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 3_u, 1_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a1, 2_u, 0_u,
                                       0_u);

        auto* a2 = b.Access(ty.ptr<workgroup, i32, read_write>(), wg, 3_u, 1_u, 2_u);
        b.Store(a2, 0_i);
        b.Return(f);
    });

    auto* src = R"(
S0 = struct @align(4) {
  x:i32 @offset(0)
  a:u32 @offset(4)
  y:i32 @offset(8)
  z:i32 @offset(12)
}

S1 = struct @align(4) {
  x_1:i32 @offset(0)
  a_1:S0 @offset(4)
  y_1:i32 @offset(20)
  z_1:i32 @offset(24)
}

S2 = struct @align(4) {
  x_2:i32 @offset(0)
  y_2:i32 @offset(4)
  z_2:i32 @offset(8)
  a_2:S1 @offset(12)
}

$B1: {  # root
  %wg:ptr<workgroup, S2, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, i32, read_write> = access %wg, 3u, 1u, 0u
    store %3, 0i
    %4:ptr<workgroup, u32, read_write> = access %wg, 3u, 1u, 1u
    %5:void = spirv.atomic_store %4, 2u, 0u, 0u
    %6:ptr<workgroup, i32, read_write> = access %wg, 3u, 1u, 2u
    store %6, 0i
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ArrayOfStruct) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("x"), ty.i32()},
                                                   {mod.symbols.New("a"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block,
             [&] { wg = b.Var("wg", ty.ptr(workgroup, ty.array(sb, 10), read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* a = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 4_i, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a, 2_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  x:i32 @offset(0)
  a:u32 @offset(4)
}

$B1: {  # root
  %wg:ptr<workgroup, array<S, 10>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 4i, 1u
    %4:void = spirv.atomic_store %3, 2u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_StructOfArray) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("x"), ty.i32()},
                                                   {mod.symbols.New("a"), ty.array(ty.u32(), 10)},
                                                   {mod.symbols.New("y"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, sb, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* a = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u, 4_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a, 2_u, 0_u,
                                       1_u);
        b.Return(f);
    });
    auto* src = R"(
S = struct @align(4) {
  x:i32 @offset(0)
  a:array<u32, 10> @offset(4)
  y:u32 @offset(44)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 1u, 4i
    %4:void = spirv.atomic_store %3, 2u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicAdd) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIAdd, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIAdd, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_i_add %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_i_add %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_i_add %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_i_add %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicSub) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicISub, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicISub, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicISub, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicISub, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_i_sub %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_i_sub %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_i_sub %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_i_sub %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicAnd) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicAnd, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicAnd, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicAnd, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicAnd, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_and %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_and %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_and %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_and %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicOr) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicOr, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicOr, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicOr, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicOr, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });
    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_or %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_or %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_or %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_or %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicXor) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicXor, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicXor, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicXor, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicXor, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_xor %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_xor %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_xor %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_xor %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicMax) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicSMax, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicUMax, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicSMax, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicUMax, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_s_max %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_u_max %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_s_max %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_u_max %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicMin) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicSMin, a0, 1_u, 0_u, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicUMin, a1, 1_u, 0_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicSMin, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicUMin, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_s_min %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_u_min %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_s_min %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_u_min %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicExchange) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicExchange, a0, 1_u, 0_u,
                                       1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicExchange, a1, 1_u, 0_u,
                                       1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicExchange, wg_i32, 1_u,
                                       0_u, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicExchange, wg_u32, 1_u,
                                       0_u, 1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_exchange %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_exchange %7, 1u, 0u, 1u
    %9:i32 = spirv.atomic_exchange %wg_i32, 1u, 0u, 1i
    %10:u32 = spirv.atomic_exchange %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicCompareExchange) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicCompareExchange, a0, 1_u,
                                       0_u, 0_u, 1_i, 1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicCompareExchange, a1, 1_u,
                                       0_u, 0_u, 1_u, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicCompareExchange, wg_i32,
                                       1_u, 0_u, 0_u, 1_i, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicCompareExchange, wg_u32,
                                       1_u, 0_u, 0_u, 1_u, 1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_compare_exchange %5, 1u, 0u, 0u, 1i, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_compare_exchange %7, 1u, 0u, 0u, 1u, 1u
    %9:i32 = spirv.atomic_compare_exchange %wg_i32, 1u, 0u, 0u, 1i, 1i
    %10:u32 = spirv.atomic_compare_exchange %wg_u32, 1u, 0u, 0u, 1u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicLoad) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicLoad, a0, 1_u, 0_u);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, a1, 1_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicLoad, wg_i32, 1_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, wg_u32, 1_u, 0_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_load %5, 1u, 0u
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_load %7, 1u, 0u
    %9:i32 = spirv.atomic_load %wg_i32, 1u, 0u
    %10:u32 = spirv.atomic_load %wg_u32, 1u, 0u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicStore) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a0, 1_u, 0_u,
                                       1_i);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, a1, 1_u, 0_u,
                                       1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, wg_i32, 1_u, 0_u,
                                       1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, wg_u32, 1_u, 0_u,
                                       1_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:void = spirv.atomic_store %5, 1u, 0u, 1i
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:void = spirv.atomic_store %7, 1u, 0u, 1u
    %9:void = spirv.atomic_store %wg_i32, 1u, 0u, 1i
    %10:void = spirv.atomic_store %wg_u32, 1u, 0u, 1u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicDecrement) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIDecrement, a0, 1_u, 0_u);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIDecrement, a1, 1_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIDecrement, wg_i32, 1_u,
                                       0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIDecrement, wg_u32, 1_u,
                                       0_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_i_decrement %5, 1u, 0u
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_i_decrement %7, 1u, 0u
    %9:i32 = spirv.atomic_i_decrement %wg_i32, 1u, 0u
    %10:u32 = spirv.atomic_i_decrement %wg_u32, 1u, 0u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_AtomicIncrement) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.i32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                               });

    core::ir::Var* wg_u32 = nullptr;
    core::ir::Var* wg_i32 = nullptr;
    core::ir::Var* sg = nullptr;
    b.Append(mod.root_block, [&] {
        sg = b.Var("sb", ty.ptr(storage, sb, read_write));
        sg->SetBindingPoint(0, 0);

        wg_i32 = b.Var("wg_i32", ty.ptr<workgroup, i32, read_write>());
        wg_u32 = b.Var("wg_u32", ty.ptr<workgroup, u32, read_write>());
    });

    b.Append(f->Block(), [&] {  //
        auto* a0 = b.Access(ty.ptr<storage, i32, read_write>(), sg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIIncrement, a0, 1_u, 0_u);

        auto* a1 = b.Access(ty.ptr<storage, u32, read_write>(), sg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIIncrement, a1, 1_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kAtomicIIncrement, wg_i32, 1_u,
                                       0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIIncrement, wg_u32, 1_u,
                                       0_u);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:i32 @offset(0)
  b:u32 @offset(4)
}

$B1: {  # root
  %sb:ptr<storage, S, read_write> = var undef @binding_point(0, 0)
  %wg_i32:ptr<workgroup, i32, read_write> = var undef
  %wg_u32:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<storage, i32, read_write> = access %sb, 0u
    %6:i32 = spirv.atomic_i_increment %5, 1u, 0u
    %7:ptr<storage, u32, read_write> = access %sb, 1u
    %8:u32 = spirv.atomic_i_increment %7, 1u, 0u
    %9:i32 = spirv.atomic_i_increment %wg_i32, 1u, 0u
    %10:u32 = spirv.atomic_i_increment %wg_u32, 1u, 0u
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

// TODO(dsinclair): Convert these to SPV tests and get results....

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_Scalar) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, ty.u32(), read_write)); });

    b.Append(f->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, wg, 1_u, 0_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, wg, 0_u, 0_u,
                                       0_u);
        auto* l1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, wg, 0_u, 0_u);
        b.Let("a", l1);

        auto* v = b.Var("b", ty.ptr(function, ty.u32(), read_write));
        auto* l2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, wg, 0_u, 0_u);
        b.Store(v, l2);
        b.Return(f);
    });

    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:u32 = load %wg
    %4:u32 = spirv.atomic_i_add %3, 1u, 0u, 0u
    store %wg, 0u
    %5:u32 = load %wg
    %a:u32 = let %5
    %b:ptr<function, u32, read_write> = var undef
    %8:u32 = load %wg
    store %b, %8
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_Struct) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, sb, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);

        auto* l2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l2, 0_u, 0_u,
                                       0_u);

        auto* l3 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l3, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l4 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Store(b_, v2);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:u32 @offset(0)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 0u
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 0u
    store %6, 0u
    %8:ptr<workgroup, u32, read_write> = access %wg, 0u
    %9:u32 = load %8
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:ptr<workgroup, u32, read_write> = access %wg, 0u
    %13:u32 = load %12
    store %b, %13
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_NestedStruct) {
    auto* f = b.ComputeFunction("main");

    auto* s0 = ty.Struct(mod.symbols.New("S0"), {
                                                    {mod.symbols.New("a"), ty.u32()},
                                                });
    auto* s1 = ty.Struct(mod.symbols.New("S1"), {
                                                    {mod.symbols.New("s0"), s0},
                                                });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, s1, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);
        auto* l2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l2, 0_u, 0_u,
                                       0_u);
        auto* l3 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u, 0_u);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l3, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l4 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u, 0_u);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Store(b_, v2);

        b.Return(f);
    });

    auto* src = R"(
S0 = struct @align(4) {
  a:u32 @offset(0)
}

S1 = struct @align(4) {
  s0:S0 @offset(0)
}

$B1: {  # root
  %wg:ptr<workgroup, S1, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 0u, 0u
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 0u, 0u
    store %6, 0u
    %8:ptr<workgroup, u32, read_write> = access %wg, 0u, 0u
    %9:u32 = load %8
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:ptr<workgroup, u32, read_write> = access %wg, 0u, 0u
    %13:u32 = load %12
    store %b, %13
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_StructMultipleAtomics) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.u32()},
                                                   {mod.symbols.New("b"), ty.u32()},
                                                   {mod.symbols.New("c"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, sb, read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);
        auto* l2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l2, 1_u, 0_u, 0_u);
        auto* l3 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l3, 0_u, 0_u,
                                       0_u);
        auto* l4 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l5 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 0_u);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l5, 0_u, 0_u);
        b.Store(b_, v2);

        auto* l6 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l6, 0_u, 0_u,
                                       0_u);

        auto* l7 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        auto* v3 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l7, 0_u, 0_u);
        b.Let("c", v3);
        auto* d = b.Var("d", ty.ptr<function, u32, read_write>());

        auto* l8 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_u);
        auto* v4 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l8, 0_u, 0_u);
        b.Store(d, v4);

        auto* l9 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_u);
        b.Store(l9, 0_u);

        auto* l10 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_u);
        auto* v5 = b.Load(l10);
        b.Let("e", v5);

        auto* f_1 = b.Var("f_1", ty.ptr<function, u32, read_write>());
        auto* l11 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 2_u);
        auto* v6 = b.Load(l11);
        b.Store(f_1, v6);

        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:u32 @offset(0)
  b:u32 @offset(4)
  c:u32 @offset(8)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 0u
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 1u
    %7:u32 = load %6
    %8:u32 = spirv.atomic_i_add %7, 1u, 0u, 0u
    %9:ptr<workgroup, u32, read_write> = access %wg, 0u
    store %9, 0u
    %11:ptr<workgroup, u32, read_write> = access %wg, 0u
    %12:u32 = load %11
    %a:u32 = let %12
    %b:ptr<function, u32, read_write> = var undef
    %15:ptr<workgroup, u32, read_write> = access %wg, 0u
    %16:u32 = load %15
    store %b, %16
    %18:ptr<workgroup, u32, read_write> = access %wg, 1u
    store %18, 0u
    %20:ptr<workgroup, u32, read_write> = access %wg, 1u
    %21:u32 = load %20
    %c:u32 = let %21
    %d:ptr<function, u32, read_write> = var undef
    %24:ptr<workgroup, u32, read_write> = access %wg, 1u
    %25:u32 = load %24
    store %d, %25
    %26:ptr<workgroup, u32, read_write> = access %wg, 2u
    store %26, 0u
    %27:ptr<workgroup, u32, read_write> = access %wg, 2u
    %28:u32 = load %27
    %e:u32 = let %28
    %main_1:ptr<function, u32, read_write> = var undef
    %31:ptr<workgroup, u32, read_write> = access %wg, 2u
    %32:u32 = load %31
    store %main_1, %32
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_ArrayOfScalar) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block,
             [&] { wg = b.Var("wg", ty.ptr<workgroup, array<u32, 4>, read_write>()); });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);
        auto* l2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l2, 0_u, 0_u,
                                       0_u);
        auto* l3 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l3, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l4 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Store(b_, v2);

        b.Return(f);
    });

    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, array<u32, 4>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 1i
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 1i
    store %6, 0u
    %8:ptr<workgroup, u32, read_write> = access %wg, 1i
    %9:u32 = load %8
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:ptr<workgroup, u32, read_write> = access %wg, 1i
    %13:u32 = load %12
    store %b, %13
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_ArrayOfStruct) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("a"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block,
             [&] { wg = b.Var("wg", ty.ptr(workgroup, ty.array(sb, 4), read_write)); });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);
        auto* l2 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l2, 0_u, 0_u,
                                       0_u);
        auto* l3 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i, 0_u);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l3, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l4 = b.Access(ty.ptr<workgroup, u32, read_write>(), wg, 1_i, 0_u);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Store(b_, v2);

        b.Return(f);
    });
    auto* src = R"(
S = struct @align(4) {
  a:u32 @offset(0)
}

$B1: {  # root
  %wg:ptr<workgroup, array<S, 4>, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 1i, 0u
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 1i, 0u
    store %6, 0u
    %8:ptr<workgroup, u32, read_write> = access %wg, 1i, 0u
    %9:u32 = load %8
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:ptr<workgroup, u32, read_write> = access %wg, 1i, 0u
    %13:u32 = load %12
    store %b, %13
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_StructOfArray) {
    auto* f = b.ComputeFunction("main");

    auto* sb =
        ty.Struct(mod.symbols.New("S"), {
                                            {mod.symbols.New("a"), ty.runtime_array(ty.u32())},
                                        });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] {
        wg = b.Var("s", ty.ptr(storage, sb, read_write));
        wg->SetBindingPoint(0, 1);
    });

    b.Append(f->Block(), [&] {  //
        auto* l1 = b.Access(ty.ptr<storage, u32, read_write>(), wg, 0_u, 4_i);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, l1, 1_u, 0_u, 0_u);
        auto* l2 = b.Access(ty.ptr<storage, u32, read_write>(), wg, 0_u, 4_i);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, l2, 0_u, 0_u,
                                       0_u);
        auto* l3 = b.Access(ty.ptr<storage, u32, read_write>(), wg, 0_u, 4_i);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l3, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* l4 = b.Access(ty.ptr<storage, u32, read_write>(), wg, 0_u, 4_i);
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, l4, 0_u, 0_u);
        b.Store(b_, v2);
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:array<u32> @offset(0)
}

$B1: {  # root
  %s:ptr<storage, S, read_write> = var undef @binding_point(0, 1)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<storage, u32, read_write> = access %s, 0u, 4i
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<storage, u32, read_write> = access %s, 0u, 4i
    store %6, 0u
    %8:ptr<storage, u32, read_write> = access %s, 0u, 4i
    %9:u32 = load %8
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:ptr<storage, u32, read_write> = access %s, 0u, 4i
    %13:u32 = load %12
    store %b, %13
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceAssignsAndDecls_Let) {
    auto* f = b.ComputeFunction("main");

    auto* sb = ty.Struct(mod.symbols.New("S"), {
                                                   {mod.symbols.New("i"), ty.u32()},
                                               });

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] {
        wg = b.Var("s", ty.ptr(storage, sb, read_write));
        wg->SetBindingPoint(0, 1);
    });

    b.Append(f->Block(), [&] {  //
        auto* p0 = b.Let("p0", wg);
        auto* a = b.Access(ty.ptr<storage, u32, read_write>(), p0, 0_u);
        auto* p1 = b.Let("p1", a);
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, p1, 1_u, 0_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, p1, 0_u, 0_u,
                                       0_u);
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, p1, 0_u, 0_u);
        b.Let("a", v1);

        auto* b_ = b.Var("b", ty.ptr<function, u32, read_write>());
        auto* v2 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, p1, 0_u, 0_u);
        b.Store(b_, v2);

        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  i:u32 @offset(0)
}

$B1: {  # root
  %s:ptr<storage, S, read_write> = var undef @binding_point(0, 1)
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %p0:ptr<storage, S, read_write> = let %s
    %4:ptr<storage, u32, read_write> = access %p0, 0u
    %p1:ptr<storage, u32, read_write> = let %4
    %6:u32 = load %p1
    %7:u32 = spirv.atomic_i_add %6, 1u, 0u, 0u
    store %7, 0u
    %9:u32 = load %p1
    %a:u32 = let %9
    %b:ptr<function, u32, read_write> = var undef
    %12:u32 = load %p1, 0u, 0u
    store %b, %12
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceBitcastArgument_Scalar) {
    auto* f = b.ComputeFunction("main");

    core::ir::Var* wg = nullptr;
    b.Append(mod.root_block, [&] { wg = b.Var("wg", ty.ptr(workgroup, ty.u32(), read_write)); });

    b.Append(f->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicIAdd, wg, 1_u, 0_u, 0_u);
        b.Call<spirv::ir::BuiltinCall>(ty.void_(), spirv::BuiltinFn::kAtomicStore, wg, 0_u, 0_u,
                                       0_u);
        auto* b_ = b.Var("b", ty.ptr<function, f32, read_write>());
        auto* v1 =
            b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kAtomicLoad, wg, 0_u, 0_u);
        auto* bc = b.Bitcast(ty.f32(), v1);
        b.Store(b_, bc);
        b.Return(f);
    });

    auto* src = R"(
$B1: {  # root
  %wg:ptr<workgroup, u32, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:u32 = load %wg
    %4:u32 = spirv.atomic_i_add %3, 1u, 0u, 0u
    store %4, 0u
    %b:ptr<function, f32, read_write> = var undef
    %7:u32 = load %wg
    %8:f32 = bitcast %7
    store %b, %8
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

TEST_F(SpirvReader_AtomicsTest, DISABLED_ReplaceBitcastArgument_Struct) {
    auto* f = b.ComputeFunction("main");

    b.Append(f->Block(), [&] {  //
        b.Return(f);
    });

    auto* src = R"(
S = struct @align(4) {
  a:u32 @offset(0)
}

$B1: {  # root
  %wg:ptr<workgroup, S, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:ptr<workgroup, u32, read_write> = access %wg, 0u
    %4:u32 = load %3
    %5:u32 = spirv.atomic_i_add %4, 1u, 0u, 0u
    %6:ptr<workgroup, u32, read_write> = access %wg, 0u
    store %6, 0u
    %b:ptr<function, f32, read_write> = var undef
    %8:ptr<workgroup, u32, read_write> = access %wg, 0u
    %9:u32 = load %8
    %10:f32 = bitcast %9
    store %b, %10
    ret
  }
}
)";

    ASSERT_EQ(src, str());
    Run(Atomics);

    auto* expect = R"(
UNIMPLEMENTED
)";
    ASSERT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::spirv::reader::lower
