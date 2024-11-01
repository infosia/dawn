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

#include "src/tint/lang/core/ir/transform/substitute_overrides.h"

#include <limits>
#include <utility>

#include "gtest/gtest.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"

namespace tint::core::ir::transform {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using IR_SubstituteOverridesTest = TransformTest;

TEST_F(IR_SubstituteOverridesTest, NoOverridesNoChange) {
    auto* func = b.Function("foo", ty.void_());
    func->Block()->Append(b.Return(func));

    auto* expect = R"(
%foo = func():void {
  $B1: {
    ret
  }
}
)";

    SubstituteOverridesConfig cfg{};
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, UnsetOverrideTriggersError) {
    b.Append(mod.root_block, [&] {
        auto* o = b.Override(Source{{1, 2}}, "a", ty.i32());
        o->SetOverrideId({1});
    });

    auto* src = R"(
$B1: {  # root
  %a:i32 = override @id(1)
}

)";
    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    auto result = RunWithFailure(SubstituteOverrides, cfg);
    ASSERT_NE(result, Success);
    EXPECT_EQ(result.Failure().reason.Str(),
              R"(1:2 error: Initializer not provided for override, and override not overridden.)");
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithDefault) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        o = b.Override(Source{{1, 2}}, "a", 2_u);
        o->SetOverrideId({1});
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %a:u32 = override, 2u @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 2u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithDefaultWithOverride) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        o = b.Override(Source{{1, 2}}, "a", 2_u);
        o->SetOverrideId({1});
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %a:u32 = override, 2u @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 55u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{1}] = 55;
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithoutDefaultWithOverride) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        o = b.Override(Source{{1, 2}}, "a", ty.u32());
        o->SetOverrideId({1});
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %a:u32 = override @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 55u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{1}] = 55;
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithComplexInitNoOverrides) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        auto* add = b.Add(ty.u32(), 2_u, 4_u);

        o = b.Override(Source{{1, 2}}, "a", ty.u32());
        o->SetOverrideId({1});
        o->SetInitializer(add->Result(0));
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %1:u32 = add 2u, 4u
  %a:u32 = override, %1 @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 6u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithComplexInitComponentOverride) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        auto* add = b.Add(ty.u32(), 2_u, 4_u);

        o = b.Override(Source{{1, 2}}, "a", ty.u32());
        o->SetOverrideId({1});
        o->SetInitializer(add->Result(0));
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %1:u32 = add 2u, 4u
  %a:u32 = override, %1 @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 55u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{1}] = 55;
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithComplexIncludingOverride) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        auto* x = b.Override("x", ty.u32());
        x->SetOverrideId({2});

        auto* add = b.Add(ty.u32(), x, 4_u);

        o = b.Override(Source{{1, 2}}, "a", ty.u32());
        o->SetOverrideId({1});
        o->SetInitializer(add->Result(0));
    });

    auto* func = b.Function("foo", ty.u32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %x:u32 = override @id(2)
  %2:u32 = add %x, 4u
  %a:u32 = override, %2 @id(1)
}

%foo = func():u32 {
  $B2: {
    ret %a
  }
}
)";

    auto* expect = R"(
%foo = func():u32 {
  $B1: {
    ret 9u
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{2}] = 5;
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

TEST_F(IR_SubstituteOverridesTest, OverrideWithComplexGenError) {
    core::ir::Override* o = nullptr;
    b.Append(mod.root_block, [&] {
        auto* x = b.Override("x", ty.f32());
        x->SetOverrideId({2});

        auto* add = b.Add(ty.f32(), x, f32(std::numeric_limits<float>::max() - 1));
        b.ir.SetSource(add, Source{{1, 2}});

        o = b.Override("a", ty.f32());
        o->SetOverrideId({1});
        o->SetInitializer(add->Result(0));
    });

    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] { b.Return(func, o->Result(0)); });

    auto* src = R"(
$B1: {  # root
  %x:f32 = override @id(2)
  %2:f32 = add %x, 340282346638528859811704183484516925440.0f
  %a:f32 = override, %2 @id(1)
}

%foo = func():f32 {
  $B2: {
    ret %a
  }
}
)";
    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{2}] = static_cast<double>(std::numeric_limits<float>::max());
    auto result = RunWithFailure(SubstituteOverrides, cfg);
    ASSERT_NE(result, Success);
    EXPECT_EQ(
        result.Failure().reason.Str(),
        R"(1:2 error: '340282346638528859811704183484516925440.0 + 340282346638528859811704183484516925440.0' cannot be represented as 'f32')");
}

TEST_F(IR_SubstituteOverridesTest, OverrideWorkgroupSize) {
    core::ir::Override* o = nullptr;
    core::ir::Override* x = nullptr;
    b.Append(mod.root_block, [&] {
        x = b.Override("x", ty.u32());
        x->SetOverrideId({2});

        auto* add = b.Add(ty.u32(), x, 4_u);

        o = b.Override(Source{{1, 2}}, "a", ty.u32());
        o->SetOverrideId({1});
        o->SetInitializer(add->Result(0));
    });

    auto* func = b.ComputeFunction("foo", o, x, o);
    b.Append(func->Block(), [&] { b.Return(func); });

    auto* src = R"(
$B1: {  # root
  %x:u32 = override @id(2)
  %2:u32 = add %x, 4u
  %a:u32 = override, %2 @id(1)
}

%foo = @compute @workgroup_size(%a, %x, %a) func():void {
  $B2: {
    ret
  }
}
)";

    auto* expect = R"(
%foo = @compute @workgroup_size(9u, 5u, 9u) func():void {
  $B1: {
    ret
  }
}
)";

    EXPECT_EQ(src, str());

    SubstituteOverridesConfig cfg{};
    cfg.map[OverrideId{2}] = 5;
    Run(SubstituteOverrides, cfg);

    EXPECT_EQ(expect, str());
}

// TODO(dsinclair): Support array type overrides
TEST_F(IR_SubstituteOverridesTest, DISABLED_OverrideArraySize) {
    FAIL();
}

}  // namespace
}  // namespace tint::core::ir::transform
