// Copyright 2023 The Dawn & Tint Authors
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

#include <string>
#include <tuple>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "src/tint/lang/core/address_space.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/const_param_validator.h"
#include "src/tint/lang/core/ir/function_param.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/lang/core/ir/type/array_count.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/core/texel_format.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/memory_view.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/utils/text/string.h"

namespace tint::core::ir {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class IR_ConstParamValidatorTest : public IRTestHelper {
  public:
    /// Builds and returns a basic 'compute' entry point function, named @p name
    Function* ComputeEntryPoint(const std::string& name = "f") { return b.ComputeFunction(name); }

    /// Builds and returns a basic 'fragment' entry point function, named @p name
    Function* FragmentEntryPoint(const std::string& name = "f") {
        return b.Function(name, ty.void_(), Function::PipelineStage::kFragment);
    }

    /// Builds and returns a basic 'vertex' entry point function, named @p name
    Function* VertexEntryPoint(const std::string& name = "f") {
        auto* f = b.Function(name, ty.vec4<f32>(), Function::PipelineStage::kVertex);
        f->SetReturnBuiltin(BuiltinValue::kPosition);
        return f;
    }

    /// Adds to a function an input param named @p name of type @p type, and decorated with @p
    /// builtin
    void AddBuiltinParam(Function* func,
                         const std::string& name,
                         BuiltinValue builtin,
                         const core::type::Type* type) {
        auto* p = b.FunctionParam(name, type);
        p->SetBuiltin(builtin);
        func->AppendParam(p);
    }

    /// Adds to a function an return value of type @p type with attributes @p attr.
    /// If there is an already existing non-structured return, both values are moved into a
    /// structured return using @p name as the name.
    /// If there is an already existing structured return, then this ICEs, since that is beyond the
    /// scope of this implementation.
    void AddReturn(Function* func,
                   const std::string& name,
                   const core::type::Type* type,
                   const IOAttributes& attr = {}) {
        if (func->ReturnType()->Is<core::type::Struct>()) {
            TINT_ICE() << "AddReturn does not support adding to structured returns";
        }

        if (func->ReturnType() == ty.void_()) {
            func->SetReturnAttributes(attr);
            func->SetReturnType(type);
            return;
        }

        std::string old_name =
            func->ReturnAttributes().builtin == BuiltinValue::kPosition ? "pos" : "old_ret";
        auto* str_ty =
            ty.Struct(mod.symbols.New("OutputStruct"),
                      {
                          {mod.symbols.New(old_name), func->ReturnType(), func->ReturnAttributes()},
                          {mod.symbols.New(name), type, attr},
                      });

        func->SetReturnAttributes({});
        func->SetReturnType(str_ty);
    }

    /// Adds to a function an return value of type @p type, and decorated with @p builtin.
    /// See @ref AddReturn for more details
    void AddBuiltinReturn(Function* func,
                          const std::string& name,
                          BuiltinValue builtin,
                          const core::type::Type* type) {
        IOAttributes attr;
        attr.builtin = builtin;
        AddReturn(func, name, type, attr);
    }
};

TEST_F(IR_ConstParamValidatorTest, CorrectDomainQuantizeF16) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kQuantizeToF16, 65504_f);
        b.Return(func, shuffle_func->Result(0));
    });

    ;
    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %2:f32 = quantizeToF16 65504.0f
    ret %2
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_EQ(res, Success);
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainQuantizeF16) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kQuantizeToF16, 65505_f);
        b.Return(func, shuffle_func->Result(0));
    });

    ;
    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %2:f32 = quantizeToF16 65505.0f
    ret %2
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:3:14 error: quantizeToF16 outside domain. 
)");
}

TEST_F(IR_ConstParamValidatorTest, CorrectDomainSubgroupsShuffleXor) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffleXor, e, 127_u);
        b.Return(func, shuffle_func->Result(0));
    });

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffleXor %a, 127u
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_EQ(res, Success);
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainSubgroupsShuffleXor) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffleXor, e, 128_u);
        b.Return(func, shuffle_func->Result(0));
    });

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffleXor %a, 128u
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:4:14 error: subgroupShuffleXor mask
)");
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainSubgroupsShuffleDown) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffleDown, e, 128_u);
        b.Return(func, shuffle_func->Result(0));
    });

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffleDown %a, 128u
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:4:14 error: subgroupShuffleDown delta
)");
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainSubgroupsShuffle) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffle, e, 128_u);
        b.Return(func, shuffle_func->Result(0));
    });

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffle %a, 128u
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:4:14 error: subgroupShuffle sourceLaneIndex
)");
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainSubgroupsShuffle_SignedHigh) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffle, e, 128_i);
        b.Return(func, shuffle_func->Result(0));
    });

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffle %a, 128i
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:4:14 error: subgroupShuffle sourceLaneIndex
)");
}

TEST_F(IR_ConstParamValidatorTest, IncorrectDomainSubgroupsShuffle_SignedLow) {
    auto* func = b.Function("foo", ty.f32());
    b.Append(func->Block(), [&] {
        auto* e = b.Let("a", 777_f);
        auto* shuffle_func = b.Call(ty.f32(), core::BuiltinFn::kSubgroupShuffle, e, -128_i);
        b.Return(func, shuffle_func->Result(0));
    });
    ASSERT_EQ(ir::Validate(mod), Success);

    auto* src = R"(
%foo = func():f32 {
  $B1: {
    %a:f32 = let 777.0f
    %3:f32 = subgroupShuffle %a, -128i
    ret %3
  }
}
)";
    EXPECT_EQ(src, str());

    auto res = ir::ValidateConstParam(mod);
    ASSERT_NE(res, Success);
    EXPECT_EQ(res.Failure().reason.Str(),
              R"(:4:14 error: subgroupShuffle sourceLaneIndex
)");
}

}  // namespace
}  // namespace tint::core::ir
