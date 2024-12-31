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

#include "src/tint/lang/spirv/reader/lower/builtins.h"

#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/spirv/ir/builtin_call.h"

namespace tint::spirv::reader::lower {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using SpirvParser_BuiltinsTest = core::ir::transform::TransformTest;

TEST_F(SpirvParser_BuiltinsTest, Scalar_UnsignedArg) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kSignUI, b.Constant(10_u));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = spirv.sign_u_i 10u
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = bitcast 10u
    %3:i32 = sign %2
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Scalar_UnsignedResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kSignIU, b.Constant(10_i));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = spirv.sign_i_u 10i
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = sign 10i
    %3:u32 = bitcast %2
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Scalar_UnsignedArgAndResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.u32(), spirv::BuiltinFn::kSignU, b.Constant(10_u));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = spirv.sign_u 10u
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = bitcast 10u
    %3:i32 = sign %2
    %4:u32 = bitcast %3
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Scalar_SignedArgAndResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.i32(), spirv::BuiltinFn::kSignI, b.Constant(10_i));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = spirv.sign_i 10i
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = sign 10i
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Vector_UnsignedArg) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.vec2<i32>(), spirv::BuiltinFn::kSignUI,
                                       b.Splat(ty.vec2<u32>(), (10_u)));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = spirv.sign_u_i vec2<u32>(10u)
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = bitcast vec2<u32>(10u)
    %3:vec2<i32> = sign %2
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Vector_UnsignedResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.vec2<u32>(), spirv::BuiltinFn::kSignIU,
                                       b.Splat(ty.vec2<i32>(), 10_i));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = spirv.sign_i_u vec2<i32>(10i)
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = sign vec2<i32>(10i)
    %3:vec2<u32> = bitcast %2
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Vector_UnsignedArgAndResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.vec2<u32>(), spirv::BuiltinFn::kSignU,
                                       b.Splat(ty.vec2<u32>(), 10_u));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = spirv.sign_u vec2<u32>(10u)
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = bitcast vec2<u32>(10u)
    %3:vec2<i32> = sign %2
    %4:vec2<u32> = bitcast %3
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Vector_SignedArgAndResult) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.vec2<i32>(), spirv::BuiltinFn::kSignI,
                                       b.Splat(ty.vec2<i32>(), 10_i));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = spirv.sign_i vec2<i32>(10i)
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = sign vec2<i32>(10i)
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Normalize_Scalar) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.f32(), spirv::BuiltinFn::kNormalize, 10_f);
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = spirv.normalize 10.0f
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = sign 10.0f
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

TEST_F(SpirvParser_BuiltinsTest, Normalize_Vector) {
    auto* ep = b.ComputeFunction("foo");

    b.Append(ep->Block(), [&] {  //
        b.Call<spirv::ir::BuiltinCall>(ty.vec2<f32>(), spirv::BuiltinFn::kNormalize,
                                       b.Splat(ty.vec2<f32>(), 10_f));
        b.Return(ep);
    });

    auto* src = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = spirv.normalize vec2<f32>(10.0f)
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    Run(Builtins);

    auto* expect = R"(
%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = normalize vec2<f32>(10.0f)
    ret
  }
}
)";
    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::spirv::reader::lower
