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

#include "src/tint/lang/hlsl/writer/raise/num_workgroups_from_uniform.h"

#include <gtest/gtest.h>

#include <string>

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/core/type/builtin_structs.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::hlsl::writer::raise {
namespace {

using HlslWriterNumWorkgroupsFromUniformTest = core::ir::transform::TransformTest;

TEST_F(HlslWriterNumWorkgroupsFromUniformTest, NoNumWorkgroups) {
    auto* func = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    func->SetWorkgroupSize(1, 1, 1);
    b.Append(func->Block(), [&] { b.Return(func); });

    auto* src = R"(
%foo = @compute @workgroup_size(1, 1, 1) func():void {
  $B1: {
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = src;
    NumWorkgroupsFromUniformConfig config;
    config.ubo_binding = {};
    Run(NumWorkgroupsFromUniform, config);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterNumWorkgroupsFromUniformTest, Basic) {
    core::IOAttributes attrs;
    attrs.builtin = core::BuiltinValue::kNumWorkgroups;
    auto* struct_ty = ty.Struct(mod.symbols.New("foo_inputs"),
                                {{mod.symbols.New("num_wgs"), ty.vec3<u32>(), attrs}});

    auto* foo_inner = b.Function("foo_inner", ty.void_());
    foo_inner->SetParams({b.FunctionParam("num_wgs", ty.vec3<u32>())});
    b.Append(foo_inner->Block(), [&] {
        auto* access1 = b.Access(ty.u32(), foo_inner->Params()[0], 0_u);
        b.Let("a", access1);
        b.Return(foo_inner);
    });

    auto* foo = b.Function("foo", ty.void_(), core::ir::Function::PipelineStage::kCompute);
    foo->SetWorkgroupSize(1, 1, 1);
    foo->SetParams({b.FunctionParam(struct_ty)});

    b.Append(foo->Block(), [&] {
        auto* access = b.Access(ty.vec3<u32>(), foo->Params()[0], 0_u);
        b.Call(foo_inner, access);
        b.Return(foo);
    });

    auto* src = R"(
foo_inputs = struct @align(16) {
  num_wgs:vec3<u32> @offset(0), @builtin(num_workgroups)
}

%foo_inner = func(%num_wgs:vec3<u32>):void {
  $B1: {
    %3:u32 = access %num_wgs, 0u
    %a:u32 = let %3
    ret
  }
}
%foo = @compute @workgroup_size(1, 1, 1) func(%6:foo_inputs):void {
  $B2: {
    %7:vec3<u32> = access %6, 0u
    %8:void = call %foo_inner, %7
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
foo_inputs = struct @align(16) {
  num_wgs:vec3<u32> @offset(0), @builtin(num_workgroups)
}

tint_symbol_1 = struct @align(16) {
  tint_symbol:vec3<u32> @offset(0)
}

$B1: {  # root
  %1:ptr<uniform, tint_symbol_1, read> = var @binding_point(0, 0)
}

%foo_inner = func(%num_wgs:vec3<u32>):void {
  $B2: {
    %4:u32 = access %num_wgs, 0u
    %a:u32 = let %4
    ret
  }
}
%foo = @compute @workgroup_size(1, 1, 1) func(%7:foo_inputs):void {
  $B3: {
    %8:ptr<uniform, vec3<u32>, read> = access %1, 0u
    %9:vec3<u32> = load %8
    %10:void = call %foo_inner, %9
    ret
  }
}
)";
    NumWorkgroupsFromUniformConfig config;
    config.ubo_binding = {};
    Run(NumWorkgroupsFromUniform, config);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::hlsl::writer::raise
