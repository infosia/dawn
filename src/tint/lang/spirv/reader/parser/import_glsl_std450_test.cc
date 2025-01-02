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

#include "src/tint/lang/spirv/reader/parser/helper_test.h"

namespace tint::spirv::reader {
namespace {

std::string Preamble() {
    return R"(
  OpCapability Shader
  %glsl = OpExtInstImport "GLSL.std.450"
  OpMemoryModel Logical GLSL450
  OpEntryPoint GLCompute %100 "main"
  OpExecutionMode %100 LocalSize 1 1 1

  OpName %u1 "u1"
  OpName %u2 "u2"
  OpName %u3 "u3"
  OpName %i1 "i1"
  OpName %i2 "i2"
  OpName %i3 "i3"
  OpName %f1 "f1"
  OpName %f2 "f2"
  OpName %f3 "f3"
  OpName %v2u1 "v2u1"
  OpName %v2u2 "v2u2"
  OpName %v2u3 "v2u3"
  OpName %v2i1 "v2i1"
  OpName %v2i2 "v2i2"
  OpName %v2i3 "v2i3"
  OpName %v2f1 "v2f1"
  OpName %v2f2 "v2f2"
  OpName %v2f3 "v2f3"
  OpName %v3f1 "v3f1"
  OpName %v3f2 "v3f2"
  OpName %v4f1 "v4f1"
  OpName %v4f2 "v4f2"
  OpName %m2x2f1 "m2x2f1"
  OpName %m3x3f1 "m3x3f1"
  OpName %m4x4f1 "m4x4f1"

  %void = OpTypeVoid
  %voidfn = OpTypeFunction %void

  %uint = OpTypeInt 32 0
  %int = OpTypeInt 32 1
  %float = OpTypeFloat 32

  %uint_10 = OpConstant %uint 10
  %uint_15 = OpConstant %uint 15
  %uint_20 = OpConstant %uint 20
  %int_30 = OpConstant %int 30
  %int_35 = OpConstant %int 35
  %int_40 = OpConstant %int 40
  %float_50 = OpConstant %float 50
  %float_60 = OpConstant %float 60
  %float_70 = OpConstant %float 70

  %v2uint = OpTypeVector %uint 2
  %v2int = OpTypeVector %int 2
  %v2float = OpTypeVector %float 2
  %v3float = OpTypeVector %float 3
  %v4float = OpTypeVector %float 4
  %mat2v2float = OpTypeMatrix %v2float 2
  %mat3v3float = OpTypeMatrix %v3float 3
  %mat4v4float = OpTypeMatrix %v4float 4

  %v2uint_10_20 = OpConstantComposite %v2uint %uint_10 %uint_20
  %v2uint_20_10 = OpConstantComposite %v2uint %uint_20 %uint_10
  %v2uint_15_15 = OpConstantComposite %v2uint %uint_15 %uint_15
  %v2int_30_40 = OpConstantComposite %v2int %int_30 %int_40
  %v2int_40_30 = OpConstantComposite %v2int %int_40 %int_30
  %v2int_35_35 = OpConstantComposite %v2int %int_35 %int_35
  %v2float_50_60 = OpConstantComposite %v2float %float_50 %float_60
  %v2float_60_50 = OpConstantComposite %v2float %float_60 %float_50
  %v2float_70_70 = OpConstantComposite %v2float %float_70 %float_70

  %v3float_50_60_70 = OpConstantComposite %v3float %float_50 %float_60 %float_70
  %v3float_60_70_50 = OpConstantComposite %v3float %float_60 %float_70 %float_50

  %v4float_50_50_50_50 = OpConstantComposite %v4float %float_50 %float_50 %float_50 %float_50

  %mat2v2float_50_60 = OpConstantComposite %mat2v2float %v2float_50_60 %v2float_50_60
  %mat3v3float_50_60_70 = OpConstantComposite %mat3v3float %v3float_50_60_70 %v3float_50_60_70 %v3float_50_60_70
  %mat4v4float_50_50_50_50 = OpConstantComposite %mat4v4float %v4float_50_50_50_50 %v4float_50_50_50_50 %v4float_50_50_50_50 %v4float_50_50_50_50

  %100 = OpFunction %void None %voidfn
  %entry = OpLabel

  %u1 = OpCopyObject %uint %uint_10
  %u2 = OpCopyObject %uint %uint_15
  %u3 = OpCopyObject %uint %uint_20

  %i1 = OpCopyObject %int %int_30
  %i2 = OpCopyObject %int %int_35
  %i3 = OpCopyObject %int %int_40

  %f1 = OpCopyObject %float %float_50
  %f2 = OpCopyObject %float %float_60
  %f3 = OpCopyObject %float %float_70

  %v2u1 = OpCopyObject %v2uint %v2uint_10_20
  %v2u2 = OpCopyObject %v2uint %v2uint_20_10
  %v2u3 = OpCopyObject %v2uint %v2uint_15_15

  %v2i1 = OpCopyObject %v2int %v2int_30_40
  %v2i2 = OpCopyObject %v2int %v2int_40_30
  %v2i3 = OpCopyObject %v2int %v2int_35_35

  %v2f1 = OpCopyObject %v2float %v2float_50_60
  %v2f2 = OpCopyObject %v2float %v2float_60_50
  %v2f3 = OpCopyObject %v2float %v2float_70_70

  %v3f1 = OpCopyObject %v3float %v3float_50_60_70
  %v3f2 = OpCopyObject %v3float %v3float_60_70_50

  %v4f1 = OpCopyObject %v4float %v4float_50_50_50_50
  %v4f2 = OpCopyObject %v4float %v4f1

  %m2x2f1 = OpCopyObject %mat2v2float %mat2v2float_50_60
  %m3x3f1 = OpCopyObject %mat3v3float %mat3v3float_50_60_70
  %m4x4f1 = OpCopyObject %mat4v4float %mat4v4float_50_50_50_50
)";
}

struct GlslStd450Case {
    std::string opcode;
    std::string wgsl_func;
};
inline std::ostream& operator<<(std::ostream& out, GlslStd450Case c) {
    out << "GlslStd450Case(" << c.opcode << " " << c.wgsl_func << ")";
    return out;
}

// Nomenclature:
// Float = scalar float
// Floating = scalar float or vector-of-float
// Float3 = 3-element vector of float
// Int = scalar signed int
// Inting = scalar int or vector-of-int
// Uint = scalar unsigned int
// Uinting = scalar unsigned or vector-of-unsigned

using SpirvParserTest_GlslStd450_Float_Floating = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Float_FloatingFloating = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Floating_Floating = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Floating_FloatingFloating =
    SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Floating_FloatingFloatingFloating =
    SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Floating_FloatingInting = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Float3_Float3Float3 = SpirvParserTestWithParam<GlslStd450Case>;

using SpirvParserTest_GlslStd450_Inting_Inting = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing =
    SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Inting_IntingInting = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Inting_IntingIntingInting =
    SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Uinting_Uinting = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Uinting_UintingUinting = SpirvParserTestWithParam<GlslStd450Case>;
using SpirvParserTest_GlslStd450_Uinting_UintingUintingUinting =
    SpirvParserTestWithParam<GlslStd450Case>;

TEST_P(SpirvParserTest_GlslStd450_Float_Floating, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1
     OpReturn
     OpFunctionEnd
  )",

              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Float_Floating, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %v2f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Float_FloatingFloating, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1 %f2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f, 60.0f
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Float_FloatingFloating, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %v2f1 %v2f2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f), vec2<f32>(60.0f, 50.0f)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_Floating, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f
    ret
  }
})");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_Floating, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl )" +
                  GetParam().opcode + R"( %v2f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingFloating, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1 %f2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f, 60.0f
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingFloating, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl )" +
                  GetParam().opcode + R"( %v2f1 %v2f2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f), vec2<f32>(60.0f, 50.0f)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingFloatingFloating, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1 %f2 %f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f, 60.0f, 70.0f
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingFloatingFloating, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl )" +
                  GetParam().opcode +
                  R"( %v2f1 %v2f2 %v2f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f), vec2<f32>(60.0f, 50.0f), vec2<f32>(70.0f)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingInting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl )" +
                  GetParam().opcode + R"( %f1 %i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = )" + GetParam().wgsl_func +
                  R"( 50.0f, 30i
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Floating_FloatingInting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl )" +
                  GetParam().opcode +
                  R"( %v2f1 %v2i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<f32>(50.0f, 60.0f), vec2<i32>(30i, 40i)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Float3_Float3Float3, SpirvParser) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v3float %glsl )" +
                  GetParam().opcode +
                  R"( %v3f1 %v3f2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec3<f32> = )" +
                  GetParam().wgsl_func +
                  R"( vec3<f32>(50.0f, 60.0f, 70.0f), vec3<f32>(60.0f, 70.0f, 50.0f)
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Float_Floating,
                         ::testing::Values(GlslStd450Case{"Length", "length"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Float_FloatingFloating,
                         ::testing::Values(GlslStd450Case{"Distance", "distance"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Floating_Floating,
                         ::testing::ValuesIn(std::vector<GlslStd450Case>{
                             {"Acos", "acos"},                //
                             {"Asin", "asin"},                //
                             {"Atan", "atan"},                //
                             {"Ceil", "ceil"},                //
                             {"Cos", "cos"},                  //
                             {"Cosh", "cosh"},                //
                             {"Degrees", "degrees"},          //
                             {"Exp", "exp"},                  //
                             {"Exp2", "exp2"},                //
                             {"FAbs", "abs"},                 //
                             {"FSign", "sign"},               //
                             {"Floor", "floor"},              //
                             {"Fract", "fract"},              //
                             {"InverseSqrt", "inverseSqrt"},  //
                             {"Log", "log"},                  //
                             {"Log2", "log2"},                //
                             {"Radians", "radians"},          //
                             {"Round", "round"},              //
                             {"RoundEven", "round"},          //
                             {"Sin", "sin"},                  //
                             {"Sinh", "sinh"},                //
                             {"Sqrt", "sqrt"},                //
                             {"Tan", "tan"},                  //
                             {"Tanh", "tanh"},                //
                             {"Trunc", "trunc"},              //
                         }));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Floating_FloatingFloating,
                         ::testing::ValuesIn(std::vector<GlslStd450Case>{
                             {"Atan2", "atan2"},
                             {"NMax", "max"},
                             {"NMin", "min"},
                             {"FMax", "max"},  // WGSL max promises more for NaN
                             {"FMin", "min"},  // WGSL min promises more for NaN
                             {"Pow", "pow"},
                             {"Step", "step"},
                         }));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Floating_FloatingInting,
                         ::testing::Values(GlslStd450Case{"Ldexp", "ldexp"}));
// For ldexp with unsigned second argument, see below.

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Float3_Float3Float3,
                         ::testing::Values(GlslStd450Case{"Cross", "cross"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Floating_FloatingFloatingFloating,
                         ::testing::ValuesIn(std::vector<GlslStd450Case>{
                             {"NClamp", "clamp"},
                             {"FClamp", "clamp"},  // WGSL FClamp promises more for NaN
                             {"Fma", "fma"},
                             {"FMix", "mix"},
                             {"SmoothStep", "smoothstep"}}));

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl )" +
                  GetParam().opcode +
                  R"( %i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = )" + GetParam().wgsl_func +
                  R"( 30i
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing, Scalar_UnsignedArg) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl )" +
                  GetParam().opcode +
                  R"( %u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = bitcast 10u
    %3:i32 = sign %2
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing, Scalar_UnsignedResult) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl )" +
                  GetParam().opcode +
                  R"( %i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = sign 30i
    %3:u32 = bitcast %2
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2int %glsl )" +
                  GetParam().opcode +
                  R"( %v2i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<i32>(30i, 40i)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing, Vector_UnsignedArg) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2int %glsl )" +
                  GetParam().opcode +
                  R"( %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = bitcast vec2<u32>(10u, 20u)
    %3:vec2<i32> = sign %2
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing, Vector_UnsignedResult) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2uint %glsl )" +
                  GetParam().opcode +
                  R"( %v2i1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = sign vec2<i32>(30i, 40i)
    %3:vec2<u32> = bitcast %2
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_IntingInting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl )" +
                  GetParam().opcode +
                  R"( %i1 %i2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = )" + GetParam().wgsl_func +
                  R"( 30i, 35i
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_IntingInting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2int %glsl )" +
                  GetParam().opcode +
                  R"( %v2i1 %v2i2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<i32>(30i, 40i), vec2<i32>(40i, 30i)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_IntingIntingInting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl )" +
                  GetParam().opcode +
                  R"( %i1 %i2 %i3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = )" + GetParam().wgsl_func +
                  R"( 30i, 35i, 40i
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Inting_IntingIntingInting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2int %glsl )" +
                  GetParam().opcode +
                  R"( %v2i1 %v2i2 %v2i3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<i32>(30i, 40i), vec2<i32>(40i, 30i), vec2<i32>(35i)
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Inting_Inting,
                         ::testing::Values(GlslStd450Case{"SAbs", "abs"},
                                           GlslStd450Case{"FindILsb", "firstTrailingBit"},
                                           GlslStd450Case{"FindSMsb", "firstLeadingBit"},
                                           GlslStd450Case{"SSign", "sign"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Inting_Inting_SignednessCoercing,
                         ::testing::Values(GlslStd450Case{"SSign", "sign"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Inting_IntingInting,
                         ::testing::Values(GlslStd450Case{"SMax", "max"},
                                           GlslStd450Case{"SMin", "min"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Inting_IntingIntingInting,
                         ::testing::Values(GlslStd450Case{"SClamp", "clamp"}));

TEST_P(SpirvParserTest_GlslStd450_Uinting_Uinting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl )" +
                  GetParam().opcode +
                  R"( %u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = )" + GetParam().wgsl_func +
                  R"( 10u
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Uinting_Uinting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2uint %glsl )" +
                  GetParam().opcode +
                  R"( %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<u32>(10u, 20u)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Uinting_UintingUinting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl )" +
                  GetParam().opcode + R"( %u1 %u2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = )" + GetParam().wgsl_func +
                  R"( 10u, 15u
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Uinting_UintingUinting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2uint %glsl )" +
                  GetParam().opcode +
                  R"( %v2u1 %v2u2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<u32>(10u, 20u), vec2<u32>(20u, 10u)
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Uinting_UintingUintingUinting, Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl )" +
                  GetParam().opcode + R"( %u1 %u2 %u3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = )" + GetParam().wgsl_func +
                  R"( 10u, 15u, 20u
    ret
  }
}
)");
}

TEST_P(SpirvParserTest_GlslStd450_Uinting_UintingUintingUinting, Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2uint %glsl )" +
                  GetParam().opcode +
                  R"( %v2u1 %v2u2 %v2u3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = )" +
                  GetParam().wgsl_func +
                  R"( vec2<u32>(10u, 20u), vec2<u32>(20u, 10u), vec2<u32>(15u)
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Uinting_Uinting,
                         ::testing::Values(GlslStd450Case{"FindILsb", "firstTrailingBit"},
                                           GlslStd450Case{"FindUMsb", "firstLeadingBit"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Uinting_UintingUinting,
                         ::testing::Values(GlslStd450Case{"UMax", "max"},
                                           GlslStd450Case{"UMin", "min"}));

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_Uinting_UintingUintingUinting,
                         ::testing::Values(GlslStd450Case{"UClamp", "clamp"}));

// Test Normalize.  WGSL does not have a scalar form of the normalize builtin.
// So we have to test it separately, as it does not fit the patterns tested
// above.

TEST_F(SpirvParserTest, Normalize_Scalar) {
    // Scalar normalize maps to sign.
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl Normalize %f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = sign 50.0f
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Normalize_Vector2) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl Normalize %v2f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = normalize vec2<f32>(50.0f, 60.0f)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Normalize_Vector3) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v3float %glsl Normalize %v3f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec3<f32> = normalize vec3<f32>(50.0f, 60.0f, 70.0f)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, Normalize_Vector4) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v4float %glsl Normalize %v4f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec4<f32> = normalize vec4<f32>(50.0f)
    ret
  }
}
)");
}

// Check that we convert signedness of operands and result type.
// This is needed for each of the integer-based extended instructions.

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_SAbs) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl SAbs %u1
     %2 = OpExtInst %v2uint %glsl SAbs %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<u32>(abs(bitcast<i32>(u1)));
    let x_2 = bitcast<vec2u>(abs(bitcast<vec2i>(v2u1)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_SMax) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl SMax %u1 %u2
     %2 = OpExtInst %v2uint %glsl SMax %v2u1 %v2u2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<u32>(max(bitcast<i32>(u1), bitcast<i32>(u2)));
    let x_2 = bitcast<vec2u>(max(bitcast<vec2i>(v2u1), bitcast<vec2i>(v2u2)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_SMin) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl SMin %u1 %u2
     %2 = OpExtInst %v2uint %glsl SMin %v2u1 %v2u2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<u32>(min(bitcast<i32>(u1), bitcast<i32>(u2)));
    let x_2 = bitcast<vec2u>(min(bitcast<vec2i>(v2u1), bitcast<vec2i>(v2u2)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_SClamp) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl SClamp %u1 %i2 %u3
     %2 = OpExtInst %v2uint %glsl SClamp %v2u1 %v2i2 %v2u3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<u32>(clamp(bitcast<i32>(u1), i2, bitcast<i32>(u3)));
    let x_2 = bitcast<vec2u>(clamp(bitcast<vec2i>(v2u1), v2i2, bitcast<vec2i>(v2u3)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_UMax) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl UMax %i1 %i2
     %2 = OpExtInst %v2int %glsl UMax %v2i1 %v2i2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<i32>(max(bitcast<u32>(i1), bitcast<u32>(i2)));
    let x_2 = bitcast<vec2i>(max(bitcast<vec2u>(v2i1), bitcast<vec2u>(v2i2)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_UMin) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl UMin %i1 %i2
     %2 = OpExtInst %v2int %glsl UMin %v2i1 %v2i2
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<i32>(min(bitcast<u32>(i1), bitcast<u32>(i2)));
    let x_2 = bitcast<vec2i>(min(bitcast<vec2u>(v2i1), bitcast<vec2u>(v2i2)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_UClamp) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %int %glsl UClamp %i1 %u2 %i3
     %2 = OpExtInst %v2int %glsl UClamp %v2i1 %v2u2 %v2i3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<i32>(clamp(bitcast<u32>(i1), u2, bitcast<u32>(i3)));
    let x_2 = bitcast<vec2i>(clamp(bitcast<vec2u>(v2i1), v2u2, bitcast<vec2u>(v2i3)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_FindILsb) {
    // Check conversion of:
    //   signed results to unsigned result to match first arg.
    //   unsigned results to signed result to match first arg.
    // This is the first extended instruction we've supported which goes both
    // ways.
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %uint %glsl FindILsb %i1
     %2 = OpExtInst %v2uint %glsl FindILsb %v2i1
     %3 = OpExtInst %int %glsl FindILsb %u1
     %4 = OpExtInst %v2int %glsl FindILsb %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<u32>(firstTrailingBit(i1));
    let x_2 = bitcast<vec2u>(firstTrailingBit(v2i1));
    let x_3 = bitcast<i32>(firstTrailingBit(u1));
    let x_4 = bitcast<vec2i>(firstTrailingBit(v2u1));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_FindSMsb) {
    // Check signedness conversion of arguments and results.
    //   SPIR-V signed arg -> keep it
    //      signed result -> keep it
    //      unsigned result -> cast result to unsigned
    //
    //   SPIR-V unsigned arg -> cast it to signed
    //      signed result -> keept it
    //      unsigned result -> cast result to unsigned
    EXPECT_IR(Preamble() + R"(
     ; signed arg
     ;    signed result
     %1 = OpExtInst %int %glsl FindSMsb %i1
     %2 = OpExtInst %v2int %glsl FindSMsb %v2i1

     ; signed arg
     ;    unsigned result
     %3 = OpExtInst %uint %glsl FindSMsb %i1
     %4 = OpExtInst %v2uint %glsl FindSMsb %v2i1

     ; unsigned arg
     ;    signed result
     %5 = OpExtInst %int %glsl FindSMsb %u1
     %6 = OpExtInst %v2int %glsl FindSMsb %v2u1

     ; unsigned arg
     ;    unsigned result
     %7 = OpExtInst %uint %glsl FindSMsb %u1
     %8 = OpExtInst %v2uint %glsl FindSMsb %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = firstLeadingBit(i1);
    let x_2 = firstLeadingBit(v2i1);
    let x_3 = bitcast<u32>(firstLeadingBit(i1));
    let x_4 = bitcast<vec2u>(firstLeadingBit(v2i1));
    let x_5 = firstLeadingBit(bitcast<i32>(u1));
    let x_6 = firstLeadingBit(bitcast<vec2i>(v2u1));
    let x_7 = bitcast<u32>(firstLeadingBit(bitcast<i32>(u1)));
    let x_8 = bitcast<vec2u>(firstLeadingBit(bitcast<vec2i>(v2u1)));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_RectifyOperandsAndResult_FindUMsb) {
    // Check signedness conversion of arguments and results.
    //   SPIR-V signed arg -> cast arg to unsigned
    //      signed result -> cast result to signed
    //      unsigned result -> keep it
    //
    //   SPIR-V unsigned arg -> keep it
    //      signed result -> cast result to signed
    //      unsigned result -> keep it
    EXPECT_IR(Preamble() + R"(
     ; signed arg
     ;    signed result
     %1 = OpExtInst %int %glsl FindUMsb %i1
     %2 = OpExtInst %v2int %glsl FindUMsb %v2i1

     ; signed arg
     ;    unsigned result
     %3 = OpExtInst %uint %glsl FindUMsb %i1
     %4 = OpExtInst %v2uint %glsl FindUMsb %v2i1

     ; unsigned arg
     ;    signed result
     %5 = OpExtInst %int %glsl FindUMsb %u1
     %6 = OpExtInst %v2int %glsl FindUMsb %v2u1

     ; unsigned arg
     ;    unsigned result
     %7 = OpExtInst %uint %glsl FindUMsb %u1
     %8 = OpExtInst %v2uint %glsl FindUMsb %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = bitcast<i32>(firstLeadingBit(bitcast<u32>(i1)));
    let x_2 = bitcast<vec2i>(firstLeadingBit(bitcast<vec2u>(v2i1)));
    let x_3 = firstLeadingBit(bitcast<u32>(i1));
    let x_4 = firstLeadingBit(bitcast<vec2u>(v2i1));
    let x_5 = bitcast<i32>(firstLeadingBit(u1));
    let x_6 = bitcast<vec2i>(firstLeadingBit(v2u1));
    let x_7 = firstLeadingBit(u1);
    let x_8 = firstLeadingBit(v2u1);
  }
}
)");
}

struct DataPackingCase {
    std::string opcode;
    std::string wgsl_func;
    uint32_t vec_size;
};

inline std::ostream& operator<<(std::ostream& out, DataPackingCase c) {
    out << "DataPacking(" << c.opcode << ")";
    return out;
}

using SpirvParserTest_GlslStd450_DataPacking = SpirvParserTestWithParam<DataPackingCase>;

TEST_P(SpirvParserTest_GlslStd450_DataPacking, Valid) {
    auto param = GetParam();
    EXPECT_IR(Preamble() + R"(
  %1 = OpExtInst %uint %glsl )" +
                  param.opcode + (param.vec_size == 2 ? " %v2f1" : " %v4f1") + R"(
  OpReturn
  OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = )" + param.wgsl_func +
                  " vec" + std::to_string(param.vec_size) + "<f32>(50.0f" +
                  (param.vec_size == 4 ? "" : ", 60.0f") + R"()
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_DataPacking,
                         ::testing::ValuesIn(std::vector<DataPackingCase>{
                             {"PackSnorm4x8", "pack4x8snorm", 4},
                             {"PackUnorm4x8", "pack4x8unorm", 4},
                             {"PackSnorm2x16", "pack2x16snorm", 2},
                             {"PackUnorm2x16", "pack2x16unorm", 2},
                             {"PackHalf2x16", "pack2x16float", 2}}));

using SpirvParserTest_GlslStd450_DataUnpacking = SpirvParserTestWithParam<DataPackingCase>;

TEST_P(SpirvParserTest_GlslStd450_DataUnpacking, Valid) {
    auto param = GetParam();
    EXPECT_IR(Preamble() + R"(
  %1 = OpExtInst )" +
                  (param.vec_size == 2 ? "%v2float" : "%v4float") + std::string(" %glsl ") +
                  param.opcode + R"( %u1
  OpReturn
  OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec)" + std::to_string(param.vec_size) +
                  R"(<f32> = )" + param.wgsl_func +
                  R"( 10u
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(SpirvParser,
                         SpirvParserTest_GlslStd450_DataUnpacking,
                         ::testing::ValuesIn(std::vector<DataPackingCase>{
                             {"UnpackSnorm4x8", "unpack4x8snorm", 4},
                             {"UnpackUnorm4x8", "unpack4x8unorm", 4},
                             {"UnpackSnorm2x16", "unpack2x16snorm", 2},
                             {"UnpackUnorm2x16", "unpack2x16unorm", 2},
                             {"UnpackHalf2x16", "unpack2x16float", 2}}));

TEST_F(SpirvParserTest, DISABLED_GlslStd450_Refract_Scalar) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl Refract %f1 %f2 %f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = refract(vec2f(f1, 0.0f), vec2f(f2, 0.0f), f3).x;
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_Refract_Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl Refract %v2f1 %v2f2 %f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = refract(v2f1, v2f2, f3);
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_FaceForward_Scalar) {
    // The %99 sum only has one use.  Ensure it is evaluated only once by
    // making a let-declaration for it, since it is the normal operand to
    // the builtin function, and code generation uses it twice.
    EXPECT_IR(Preamble() + R"(
     %99 = OpFAdd %float %f1 %f1 ; normal operand has only one use
     %1 = OpExtInst %float %glsl FaceForward %99 %f2 %f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = select(-(x_99), x_99, ((f2 * f3) < 0.0f));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_FaceForward_Vector) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl FaceForward %v2f1 %v2f2 %v2f3
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = faceForward vec2<f32>(50.0f, 60.0f), vec2<f32>(60.0f, 50.0f), vec2<f32>(70.0f)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_Reflect_Scalar) {
    EXPECT_IR(Preamble() + R"(
     %98 = OpFAdd %float %f1 %f1 ; has only one use
     %99 = OpFAdd %float %f2 %f2 ; has only one use
     %1 = OpExtInst %float %glsl Reflect %98 %99
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = (x_98 - (2.0f * (x_99 * (x_99 * x_98))));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_Reflect_Vector) {
    EXPECT_IR(Preamble() + R"(
     %98 = OpFAdd %v2float %v2f1 %v2f1
     %99 = OpFAdd %v2float %v2f2 %v2f2
     %1 = OpExtInst %v2float %glsl Reflect %98 %99
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<f32> = add vec2<f32>(50.0f, 60.0f), vec2<f32>(50.0f, 60.0f)
    %3:vec2<f32> = add vec2<f32>(60.0f, 50.0f), vec2<f32>(60.0f, 50.0f)
    %4:vec2<f32> = reflect %2, %3
    ret
  }
}
)");
}

// For ldexp with signed second argument, see above.
TEST_F(SpirvParserTest, DISABLED_GlslStd450_Ldexp_Scalar_Float_Uint) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl Ldexp %f1 %u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = ldexp(f1, i32(u1));
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_GlslStd450_Ldexp_Vector_Floatvec_Uintvec) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %v2float %glsl Ldexp %v2f1 %v2u1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    let x_1 = ldexp(v2f1, vec2i(v2u1));
  }
}
)");
}

struct DeterminantData {
    std::string in;
    std::string out;
};

inline std::ostream& operator<<(std::ostream& out, DeterminantData c) {
    out << "Determinant(" << c.in << ")";
    return out;
}

using SpirvParserTest_GlslStd450_Determinant = SpirvParserTestWithParam<DeterminantData>;

TEST_P(SpirvParserTest_GlslStd450_Determinant, DISABLED_Test) {
    auto param = GetParam();

    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %float %glsl Determinant %)" +
                  param.in + R"(
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = determinant )" +
                  param.out + R"(
    ret
  }
}
)");
}

INSTANTIATE_TEST_SUITE_P(
    SpirvParser,
    SpirvParserTest_GlslStd450_Determinant,
    ::testing::Values(DeterminantData{"m2x2f1", "mat2x2<f32>(vec2<f32>(50.0f, 60.0f))"},
                      DeterminantData{"m3x3f1", "mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f))"},
                      DeterminantData{"m4x4f1", "mat4x4<f32>(vec4<f32>(50.0f))"}));

TEST_F(SpirvParserTest, GlslStd450_MatrixInverse_mat2x2) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %mat2v2float %glsl MatrixInverse %m2x2f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = determinant mat2x2<f32>(vec2<f32>(50.0f, 60.0f))
    %3:f32 = div 1.0f, %2
    %4:f32 = negation %3
    %5:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 0u
    %6:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 1u
    %7:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 0u
    %8:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 1u
    %9:f32 = mul %3, %8
    %10:f32 = mul %4, %6
    %11:f32 = mul %4, %7
    %12:f32 = mul %3, %5
    %13:vec2<f32> = construct %9, %10
    %14:vec2<f32> = construct %11, %12
    %15:mat2x2<f32> = construct %13, %14
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, GlslStd450_MatrixInverse_mat3x3) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %mat3v3float %glsl MatrixInverse %m3x3f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = determinant mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f))
    %3:f32 = div 1.0f, %2
    %4:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 0u, 0u
    %5:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 0u, 1u
    %6:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 0u, 2u
    %7:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 1u, 0u
    %8:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 1u, 1u
    %9:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 1u, 2u
    %10:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 2u, 0u
    %11:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 2u, 1u
    %12:f32 = access mat3x3<f32>(vec3<f32>(50.0f, 60.0f, 70.0f)), 2u, 2u
    %13:f32 = mul %8, %12
    %14:f32 = mul %9, %11
    %15:f32 = sub %13, %14
    %16:f32 = mul %6, %11
    %17:f32 = mul %5, %12
    %18:f32 = sub %16, %17
    %19:f32 = mul %5, %9
    %20:f32 = mul %6, %8
    %21:f32 = sub %19, %20
    %22:f32 = mul %9, %10
    %23:f32 = mul %7, %12
    %24:f32 = sub %22, %23
    %25:f32 = mul %4, %12
    %26:f32 = mul %6, %10
    %27:f32 = sub %25, %26
    %28:f32 = mul %6, %7
    %29:f32 = mul %4, %9
    %30:f32 = sub %28, %29
    %31:f32 = mul %7, %11
    %32:f32 = mul %8, %10
    %33:f32 = sub %31, %32
    %34:f32 = mul %5, %10
    %35:f32 = mul %4, %11
    %36:f32 = sub %34, %35
    %37:f32 = mul %4, %8
    %38:f32 = mul %5, %7
    %39:f32 = sub %37, %38
    %40:vec3<f32> = construct %15, %18, %21
    %41:vec3<f32> = construct %24, %27, %30
    %42:vec3<f32> = construct %33, %36, %39
    %43:mat3x3<f32> = construct %40, %41, %42
    %44:mat3x3<f32> = mul %3, %43
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, GlslStd450_MatrixInverse_mat4x4) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %mat4v4float %glsl MatrixInverse %m4x4f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = determinant mat4x4<f32>(vec4<f32>(50.0f))
    %3:f32 = div 1.0f, %2
    %4:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 0u, 0u
    %5:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 0u, 1u
    %6:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 0u, 2u
    %7:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 0u, 3u
    %8:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 1u, 0u
    %9:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 1u, 1u
    %10:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 1u, 2u
    %11:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 1u, 3u
    %12:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 2u, 0u
    %13:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 2u, 1u
    %14:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 2u, 2u
    %15:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 2u, 3u
    %16:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 3u, 0u
    %17:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 3u, 1u
    %18:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 3u, 2u
    %19:f32 = access mat4x4<f32>(vec4<f32>(50.0f)), 3u, 3u
    %20:f32 = mul %14, %19
    %21:f32 = mul %15, %18
    %22:f32 = sub %20, %21
    %23:f32 = mul %13, %19
    %24:f32 = mul %15, %17
    %25:f32 = sub %23, %24
    %26:f32 = mul %13, %18
    %27:f32 = mul %14, %17
    %28:f32 = sub %26, %27
    %29:f32 = mul %10, %19
    %30:f32 = mul %11, %18
    %31:f32 = sub %29, %30
    %32:f32 = mul %9, %19
    %33:f32 = mul %11, %17
    %34:f32 = sub %32, %33
    %35:f32 = mul %9, %18
    %36:f32 = mul %10, %17
    %37:f32 = sub %35, %36
    %38:f32 = mul %10, %15
    %39:f32 = mul %11, %14
    %40:f32 = sub %38, %39
    %41:f32 = mul %9, %15
    %42:f32 = mul %11, %13
    %43:f32 = sub %41, %42
    %44:f32 = mul %9, %14
    %45:f32 = mul %10, %13
    %46:f32 = sub %44, %45
    %47:f32 = mul %12, %19
    %48:f32 = mul %15, %16
    %49:f32 = sub %47, %48
    %50:f32 = mul %12, %18
    %51:f32 = mul %14, %16
    %52:f32 = sub %50, %51
    %53:f32 = mul %8, %19
    %54:f32 = mul %11, %16
    %55:f32 = sub %53, %54
    %56:f32 = mul %8, %18
    %57:f32 = mul %10, %16
    %58:f32 = sub %56, %57
    %59:f32 = mul %8, %15
    %60:f32 = mul %11, %12
    %61:f32 = sub %59, %60
    %62:f32 = mul %8, %14
    %63:f32 = mul %10, %12
    %64:f32 = sub %62, %63
    %65:f32 = mul %12, %17
    %66:f32 = mul %13, %16
    %67:f32 = sub %65, %66
    %68:f32 = mul %8, %17
    %69:f32 = mul %9, %16
    %70:f32 = sub %68, %69
    %71:f32 = mul %8, %13
    %72:f32 = mul %9, %12
    %73:f32 = sub %71, %72
    %74:f32 = negation %5
    %75:f32 = mul %9, %22
    %76:f32 = mul %10, %25
    %77:f32 = mul %11, %28
    %78:f32 = sub %75, %76
    %79:f32 = add %78, %77
    %80:f32 = mul %74, %22
    %81:f32 = mul %6, %25
    %82:f32 = mul %7, %28
    %83:f32 = add %80, %81
    %84:f32 = sub %83, %82
    %85:f32 = mul %5, %31
    %86:f32 = mul %6, %34
    %87:f32 = mul %7, %37
    %88:f32 = sub %85, %86
    %89:f32 = add %88, %87
    %90:f32 = mul %74, %40
    %91:f32 = mul %6, %43
    %92:f32 = mul %7, %46
    %93:f32 = add %90, %91
    %94:f32 = sub %93, %92
    %95:f32 = negation %8
    %96:f32 = negation %4
    %97:f32 = mul %95, %22
    %98:f32 = mul %10, %49
    %99:f32 = mul %11, %52
    %100:f32 = add %97, %98
    %101:f32 = sub %100, %99
    %102:f32 = mul %4, %22
    %103:f32 = mul %6, %49
    %104:f32 = mul %7, %52
    %105:f32 = sub %102, %103
    %106:f32 = add %105, %104
    %107:f32 = mul %96, %31
    %108:f32 = mul %6, %55
    %109:f32 = mul %7, %58
    %110:f32 = add %107, %108
    %111:f32 = sub %110, %109
    %112:f32 = mul %4, %40
    %113:f32 = mul %6, %61
    %114:f32 = mul %7, %64
    %115:f32 = sub %112, %113
    %116:f32 = add %115, %114
    %117:f32 = mul %8, %25
    %118:f32 = mul %9, %49
    %119:f32 = mul %11, %67
    %120:f32 = sub %117, %118
    %121:f32 = add %120, %119
    %122:f32 = mul %96, %25
    %123:f32 = mul %5, %49
    %124:f32 = mul %7, %67
    %125:f32 = add %122, %123
    %126:f32 = sub %125, %124
    %127:f32 = mul %4, %34
    %128:f32 = mul %5, %55
    %129:f32 = mul %7, %70
    %130:f32 = sub %127, %128
    %131:f32 = add %130, %129
    %132:f32 = mul %96, %43
    %133:f32 = mul %5, %61
    %134:f32 = mul %7, %73
    %135:f32 = add %132, %133
    %136:f32 = sub %135, %134
    %137:f32 = mul %95, %28
    %138:f32 = mul %9, %52
    %139:f32 = mul %10, %67
    %140:f32 = add %137, %138
    %141:f32 = sub %140, %139
    %142:f32 = mul %4, %28
    %143:f32 = mul %5, %52
    %144:f32 = mul %6, %67
    %145:f32 = sub %142, %143
    %146:f32 = add %145, %144
    %147:f32 = mul %96, %37
    %148:f32 = mul %5, %58
    %149:f32 = mul %6, %70
    %150:f32 = add %147, %148
    %151:f32 = sub %150, %149
    %152:f32 = mul %4, %46
    %153:f32 = mul %5, %64
    %154:f32 = mul %6, %73
    %155:f32 = sub %152, %153
    %156:f32 = add %155, %154
    %157:vec3<f32> = construct %79, %84, %89, %94
    %158:vec3<f32> = construct %101, %106, %111, %116
    %159:vec3<f32> = construct %121, %126, %131, %136
    %160:vec3<f32> = construct %141, %146, %151, %156
    %161:mat4x4<f32> = construct %157, %158, %159, %160
    %162:mat4x4<f32> = mul %3, %161
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, GlslStd450_MatrixInverse_MultipleInScope) {
    EXPECT_IR(Preamble() + R"(
     %1 = OpExtInst %mat2v2float %glsl MatrixInverse %m2x2f1
     %2 = OpExtInst %mat2v2float %glsl MatrixInverse %m2x2f1
     OpReturn
     OpFunctionEnd
  )",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = determinant mat2x2<f32>(vec2<f32>(50.0f, 60.0f))
    %3:f32 = div 1.0f, %2
    %4:f32 = negation %3
    %5:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 0u
    %6:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 1u
    %7:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 0u
    %8:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 1u
    %9:f32 = mul %3, %8
    %10:f32 = mul %4, %6
    %11:f32 = mul %4, %7
    %12:f32 = mul %3, %5
    %13:vec2<f32> = construct %9, %10
    %14:vec2<f32> = construct %11, %12
    %15:mat2x2<f32> = construct %13, %14
    %16:f32 = determinant mat2x2<f32>(vec2<f32>(50.0f, 60.0f))
    %17:f32 = div 1.0f, %16
    %18:f32 = negation %17
    %19:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 0u
    %20:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 0u, 1u
    %21:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 0u
    %22:f32 = access mat2x2<f32>(vec2<f32>(50.0f, 60.0f)), 1u, 1u
    %23:f32 = mul %17, %22
    %24:f32 = mul %18, %20
    %25:f32 = mul %18, %21
    %26:f32 = mul %17, %19
    %27:vec2<f32> = construct %23, %24
    %28:vec2<f32> = construct %25, %26
    %29:mat2x2<f32> = construct %27, %28
    ret
  }
}
)");
}

}  // namespace
}  // namespace tint::spirv::reader
