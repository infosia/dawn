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
// FOR ANY INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "src/tint/lang/msl/writer/raise/derivative_polyfill.h"

#include "gtest/gtest.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::msl::writer::raise {
namespace {

using MslWriter_DerivativePolyfillTest = core::ir::transform::TransformTest;

TEST_F(MslWriter_DerivativePolyfillTest, DpdxFineScalar) {
    auto* f = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(f->Block(), [&] {
        auto* local_var = b.Var("val", ty.ptr(function, ty.f32(), read_write));
        b.Call<core::ir::CoreBuiltinCall>(ty.f32(), core::BuiltinFn::kDpdxFine, b.Load(local_var));
        b.Return(f);
    });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    %val:ptr<function, f32, read_write> = var undef
    %3:f32 = load %val
    %4:f32 = dpdxFine %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %tint_thread_index_in_quadgroup:ptr<private, u32, read_write> = var undef
}

%foo = @fragment func(%tint_thread_index_in_quadgroup_1:u32 [@__thread_index_in_quadgroup]):void {  # %tint_thread_index_in_quadgroup_1: 'tint_thread_index_in_quadgroup'
  $B2: {
    store %tint_thread_index_in_quadgroup, %tint_thread_index_in_quadgroup_1
    %val:ptr<function, f32, read_write> = var undef
    %5:f32 = load %val
    %6:f32 = quadSwapX %5
    %7:u32 = load %tint_thread_index_in_quadgroup
    %8:u32 = and %7, 1u
    %9:bool = neq %8, 0u
    %10:f32 = sub %5, %6
    %11:f32 = sub %6, %5
    %12:f32 = select %11, %10, %9
    ret
  }
}
)";

    Run(DerivativePolyfill);
    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DerivativePolyfillTest, FwidthFineScalar) {
    auto* f = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(f->Block(), [&] {
        auto* local_var = b.Var("val", ty.ptr(function, ty.f32(), read_write));
        b.Call<core::ir::CoreBuiltinCall>(ty.f32(), core::BuiltinFn::kFwidthFine,
                                          b.Load(local_var));
        b.Return(f);
    });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    %val:ptr<function, f32, read_write> = var undef
    %3:f32 = load %val
    %4:f32 = fwidthFine %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%foo = @fragment func():void {
  $B1: {
    %val:ptr<function, f32, read_write> = var undef
    %3:f32 = load %val
    %4:f32 = quadSwapX %3
    %5:f32 = quadSwapY %3
    %6:f32 = sub %3, %4
    %7:f32 = sub %3, %5
    %8:f32 = abs %6
    %9:f32 = abs %7
    %10:f32 = add %8, %9
    ret
  }
}
)";

    Run(DerivativePolyfill);
    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DerivativePolyfillTest, DpdyFineScalar) {
    auto* f = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(f->Block(), [&] {
        auto* local_var = b.Var("val", ty.ptr(function, ty.f32(), read_write));
        b.Call<core::ir::CoreBuiltinCall>(ty.f32(), core::BuiltinFn::kDpdyFine, b.Load(local_var));
        b.Return(f);
    });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    %val:ptr<function, f32, read_write> = var undef
    %3:f32 = load %val
    %4:f32 = dpdyFine %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %tint_thread_index_in_quadgroup:ptr<private, u32, read_write> = var undef
}

%foo = @fragment func(%tint_thread_index_in_quadgroup_1:u32 [@__thread_index_in_quadgroup]):void {  # %tint_thread_index_in_quadgroup_1: 'tint_thread_index_in_quadgroup'
  $B2: {
    store %tint_thread_index_in_quadgroup, %tint_thread_index_in_quadgroup_1
    %val:ptr<function, f32, read_write> = var undef
    %5:f32 = load %val
    %6:f32 = quadSwapY %5
    %7:u32 = load %tint_thread_index_in_quadgroup
    %8:u32 = and %7, 2u
    %9:bool = neq %8, 0u
    %10:f32 = sub %5, %6
    %11:f32 = sub %6, %5
    %12:f32 = select %11, %10, %9
    ret
  }
}
)";

    Run(DerivativePolyfill);
    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DerivativePolyfillTest, DpdxFineVector) {
    auto* f = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(f->Block(), [&] {
        auto* local_var = b.Var("val", ty.ptr(function, ty.vec4f(), read_write));
        b.Call<core::ir::CoreBuiltinCall>(ty.vec4<f32>(), core::BuiltinFn::kDpdxFine,
                                          b.Load(local_var));
        b.Return(f);
    });

    auto* src = R"(
%foo = @fragment func():void {
  $B1: {
    %val:ptr<function, vec4<f32>, read_write> = var undef
    %3:vec4<f32> = load %val
    %4:vec4<f32> = dpdxFine %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %tint_thread_index_in_quadgroup:ptr<private, u32, read_write> = var undef
}

%foo = @fragment func(%tint_thread_index_in_quadgroup_1:u32 [@__thread_index_in_quadgroup]):void {  # %tint_thread_index_in_quadgroup_1: 'tint_thread_index_in_quadgroup'
  $B2: {
    store %tint_thread_index_in_quadgroup, %tint_thread_index_in_quadgroup_1
    %val:ptr<function, vec4<f32>, read_write> = var undef
    %5:vec4<f32> = load %val
    %6:vec4<f32> = quadSwapX %5
    %7:u32 = load %tint_thread_index_in_quadgroup
    %8:u32 = and %7, 1u
    %9:bool = neq %8, 0u
    %10:vec4<f32> = sub %5, %6
    %11:vec4<f32> = sub %6, %5
    %12:vec4<f32> = select %11, %10, %9
    ret
  }
}
)";

    Run(DerivativePolyfill);
    EXPECT_EQ(expect, str());
}

TEST_F(MslWriter_DerivativePolyfillTest, DpdxFineInHelperFunction) {
    auto* helper_fn = b.Function("bar", ty.f32());
    helper_fn->SetParams({b.FunctionParam("p", ty.f32())});
    b.Append(helper_fn->Block(), [&] {
        auto* p = helper_fn->Params()[0];
        auto* call = b.Call<core::ir::CoreBuiltinCall>(ty.f32(), core::BuiltinFn::kDpdxFine, p);
        b.Return(helper_fn, call);
    });

    auto* f = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kFragment);
    b.Append(f->Block(), [&] {
        auto* local_var = b.Var("val", ty.ptr(function, ty.f32(), read_write));
        b.Call(ty.f32(), helper_fn, b.Load(local_var));
        b.Return(f);
    });

    auto* src = R"(
%bar = func(%p:f32):f32 {
  $B1: {
    %3:f32 = dpdxFine %p
    ret %3
  }
}
%foo = @fragment func():void {
  $B2: {
    %val:ptr<function, f32, read_write> = var undef
    %6:f32 = load %val
    %7:f32 = call %bar, %6
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
$B1: {  # root
  %tint_thread_index_in_quadgroup:ptr<private, u32, read_write> = var undef
}

%bar = func(%p:f32):f32 {
  $B2: {
    %4:f32 = quadSwapX %p
    %5:u32 = load %tint_thread_index_in_quadgroup
    %6:u32 = and %5, 1u
    %7:bool = neq %6, 0u
    %8:f32 = sub %p, %4
    %9:f32 = sub %4, %p
    %10:f32 = select %9, %8, %7
    ret %10
  }
}
%foo = @fragment func(%tint_thread_index_in_quadgroup_1:u32 [@__thread_index_in_quadgroup]):void {  # %tint_thread_index_in_quadgroup_1: 'tint_thread_index_in_quadgroup'
  $B3: {
    store %tint_thread_index_in_quadgroup, %tint_thread_index_in_quadgroup_1
    %val:ptr<function, f32, read_write> = var undef
    %14:f32 = load %val
    %15:f32 = call %bar, %14
    ret
  }
}
)";

    Run(DerivativePolyfill);
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::msl::writer::raise
