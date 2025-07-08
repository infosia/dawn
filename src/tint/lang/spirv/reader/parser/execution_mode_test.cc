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

#include "src/tint/lang/spirv/reader/parser/helper_test.h"

namespace tint::spirv::reader {
namespace {

TEST_F(SpirvParserTest, ExecutionMode_DepthGreater) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main"
               OpExecutionMode %main OriginUpperLeft
               OpExecutionMode %main DepthGreater
               OpName %main "main"
       %void = OpTypeVoid
          %6 = OpTypeFunction %void
       %main = OpFunction %void None %6
         %15 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @fragment func():void {
  $B1: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, ExecutionMode_DepthLess) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main"
               OpExecutionMode %main OriginUpperLeft
               OpExecutionMode %main DepthLess
               OpName %main "main"
       %void = OpTypeVoid
          %6 = OpTypeFunction %void
       %main = OpFunction %void None %6
         %15 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @fragment func():void {
  $B1: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, ExecutionMode_DepthUnchanged) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main"
               OpExecutionMode %main OriginUpperLeft
               OpExecutionMode %main DepthUnchanged
               OpName %main "main"
       %void = OpTypeVoid
          %6 = OpTypeFunction %void
       %main = OpFunction %void None %6
         %15 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @fragment func():void {
  $B1: {
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, ExecutionMode_EarlyFragmentTest) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main"
               OpExecutionMode %main OriginUpperLeft
               OpExecutionMode %main EarlyFragmentTests
               OpName %main "main"
       %void = OpTypeVoid
          %6 = OpTypeFunction %void
       %main = OpFunction %void None %6
         %15 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @fragment func():void {
  $B1: {
    ret
  }
}
)");
}

}  // namespace
}  // namespace tint::spirv::reader
