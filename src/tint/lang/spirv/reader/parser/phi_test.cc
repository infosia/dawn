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

TEST_F(SpirvParserTest, Phi_FromBlock) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
       %true = OpConstantTrue %bool
      %int_2 = OpConstant %int 2
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %1
          %1 = OpLabel
          %2 = OpPhi %int %int_2 %main_start
          %3 = OpIAdd %int %2 %2
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = let 2i
    %3:i32 = spirv.add<i32> %2, %2
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_If_Undef) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
       %true = OpConstantTrue %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %1
          %1 = OpLabel
               OpSelectionMerge %5 None
               OpBranchConditional %true %3 %4
          %3 = OpLabel
          %6 = OpUndef %int
               OpBranch %5
          %4 = OpLabel
          %7 = OpUndef %int
               OpBranch %5
          %5 = OpLabel
          %8 = OpPhi %int %6 %3 %7 %4
          %9 = OpIAdd %int %8 %8
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, i32, read_write> = var undef
    if true [t: $B2, f: $B3] {  # if_1
      $B2: {  # true
        store %2, 0i
        exit_if  # if_1
      }
      $B3: {  # false
        store %2, 0i
        exit_if  # if_1
      }
    }
    %3:i32 = load %2
    %4:i32 = spirv.add<i32> %3, %3
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_If_ThenAndElse) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
       %true = OpConstantTrue %bool
      %int_1 = OpConstant %int 1
      %int_2 = OpConstant %int 2
      %int_3 = OpConstant %int 3
      %int_4 = OpConstant %int 4
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %1
          %1 = OpLabel
               OpSelectionMerge %5 None
               OpBranchConditional %true %3 %4
          %3 = OpLabel
          %6 = OpIAdd %int %int_1 %int_2
               OpBranch %5
          %4 = OpLabel
          %7 = OpIAdd %int %int_3 %int_4
               OpBranch %5
          %5 = OpLabel
          %8 = OpPhi %int %6 %3 %7 %4
          %9 = OpIAdd %int %8 %8
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, i32, read_write> = var undef
    if true [t: $B2, f: $B3] {  # if_1
      $B2: {  # true
        %3:i32 = spirv.add<i32> 1i, 2i
        store %2, %3
        exit_if  # if_1
      }
      $B3: {  # false
        %4:i32 = spirv.add<i32> 3i, 4i
        store %2, %4
        exit_if  # if_1
      }
    }
    %5:i32 = load %2
    %6:i32 = spirv.add<i32> %5, %5
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_If_ThenNoElse) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
       %true = OpConstantTrue %bool
      %int_1 = OpConstant %int 1
      %int_2 = OpConstant %int 2
      %int_3 = OpConstant %int 3
      %int_4 = OpConstant %int 4
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %1
          %1 = OpLabel
               OpSelectionMerge %5 None
               OpBranchConditional %true %3 %5
          %3 = OpLabel
          %6 = OpIAdd %int %int_1 %int_2
               OpBranch %5
          %5 = OpLabel
          %8 = OpPhi %int %6 %3 %int_2 %1
          %9 = OpIAdd %int %8 %8
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, i32, read_write> = var undef
    store %2, 2i
    if true [t: $B2, f: $B3] {  # if_1
      $B2: {  # true
        %3:i32 = spirv.add<i32> 1i, 2i
        store %2, %3
        exit_if  # if_1
      }
      $B3: {  # false
        exit_if  # if_1
      }
    }
    %4:i32 = load %2
    %5:i32 = spirv.add<i32> %4, %4
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_Loop_ContinueIsHeader) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
      %false = OpConstantFalse %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %false %20
               OpLoopMerge %99 %20 None
               OpBranchConditional %101 %99 %20
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, bool, read_write> = var undef
    store %2, true
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        %3:bool = load %2
        if %3 [t: $B4, f: $B5] {  # if_1
          $B4: {  # true
            exit_loop  # loop_1
          }
          $B5: {  # false
            continue  # -> $B3
          }
        }
        continue  # -> $B3
      }
      $B3: {  # continuing
        store %2, false
        next_iteration  # -> $B2
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_Loop_WithContinue) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
      %false = OpConstantFalse %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %false %30
               OpLoopMerge %99 %30 None
               OpBranchConditional %101 %99 %25
         %25 = OpLabel
               OpBranch %30
         %30 = OpLabel
        %102 = OpCopyObject %bool %101
               OpBranch %20
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, bool, read_write> = var undef
    store %2, true
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        %3:bool = load %2
        if %3 [t: $B4, f: $B5] {  # if_1
          $B4: {  # true
            exit_loop  # loop_1
          }
          $B5: {  # false
            continue  # -> $B3
          }
        }
        continue  # -> $B3
      }
      $B3: {  # continuing
        %4:bool = let %3
        store %2, false
        next_iteration  # -> $B2
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_Loop_BranchConditionalBreak) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
        %int = OpTypeInt 32 1
       %true = OpConstantTrue %bool
      %int_1 = OpConstant %int 1
      %int_2 = OpConstant %int 2
      %int_3 = OpConstant %int 3
      %int_4 = OpConstant %int 4
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %1
          %1 = OpLabel
               OpLoopMerge %99 %20 None
               OpBranchConditional %true %50 %20
         %50 = OpLabel
          %6 = OpIAdd %int %int_1 %int_2
               OpBranch %99
         %20 = OpLabel
               OpBranch %1
         %99 = OpLabel
          %8 = OpPhi %int %6 %50
          %9 = OpIAdd %int %8 %8
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, i32, read_write> = var undef
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        if true [t: $B4, f: $B5] {  # if_1
          $B4: {  # true
            %3:i32 = spirv.add<i32> 1i, 2i
            store %2, %3
            exit_loop  # loop_1
          }
          $B5: {  # false
            exit_loop  # loop_1
          }
        }
        continue  # -> $B3
      }
      $B3: {  # continuing
        next_iteration  # -> $B2
      }
    }
    %4:i32 = load %2
    %5:i32 = spirv.add<i32> %4, %4
    ret
  }
}
)");
}

// Phis must act as if they are simultaneously assigned. %101 and %102 should exchange values on
// each iteration, and never have the same value.
TEST_F(SpirvParserTest, DISABLED_Phi_SimultaneousAssignment) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpName %101 "default_true"
               OpName %102 "default_false"
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
      %false = OpConstantFalse %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %102 %20
        %102 = OpPhi %bool %false %10 %101 %20
               OpLoopMerge %99 %20 None
               OpBranchConditional %true %99 %20
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %default_false:ptr<function, bool, read_write> = var undef
    %default_true:ptr<function, bool, read_write> = var undef
    store %default_true, true
    store %default_false, false
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        %4:bool = load %default_true
        %5:bool = load %default_false
        if true [t: $B4, f: $B5] {  # if_1
          $B4: {  # true
            exit_loop  # loop_1
          }
          $B5: {  # false
            continue  # -> $B3
          }
        }
        continue  # -> $B3
      }
      $B3: {  # continuing
        store %default_true, %5
        store %default_false, %4
        next_iteration  # -> $B2
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_SingleBlockLoopIndex) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
               OpName %2 "computed"
               OpName %3 "copied"
       %void = OpTypeVoid
       %bool = OpTypeBool
       %uint = OpTypeInt 32 0
        %pty = OpTypePointer Private %uint
          %1 = OpVariable %pty Private
    %boolpty = OpTypePointer Private %bool
          %7 = OpVariable %boolpty Private
          %8 = OpVariable %boolpty Private
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
 %main_start = OpLabel
               OpBranch %10
; Use an outer loop to show we put the new variable in the
; smallest enclosing scope.
         %10 = OpLabel
        %101 = OpLoad %bool %7
        %102 = OpLoad %bool %8
        %103 = OpIAdd %uint %uint_0 %uint_0
               OpLoopMerge %99 %89 None
               OpBranchConditional %101 %99 %20
         %20 = OpLabel
          %2 = OpPhi %uint %103 %10 %4 %20  ; gets computed value
          %3 = OpPhi %uint %uint_1 %10 %3 %20  ; gets itself
          %4 = OpIAdd %uint %2 %uint_1
               OpLoopMerge %79 %20 None
               OpBranchConditional %102 %79 %20
         %79 = OpLabel
               OpBranch %89
         %89 = OpLabel
               OpBranch %10
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<private, u32, read_write> = var undef
  %2:ptr<private, bool, read_write> = var undef
  %3:ptr<private, bool, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %copied:ptr<function, u32, read_write> = var undef
    %computed:ptr<function, u32, read_write> = var undef
    loop [b: $B3, c: $B4] {  # loop_1
      $B3: {  # body
        %7:bool = load %2
        %8:bool = load %3
        %9:u32 = spirv.add<u32> 0u, 0u
        store %computed, %9
        store %copied, 1u
        if %7 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_loop  # loop_1
          }
          $B6: {  # false
            loop [b: $B7, c: $B8] {  # loop_2
              $B7: {  # body
                %10:u32 = load %computed
                %11:u32 = load %copied
                %12:u32 = spirv.add<u32> %10, 1u
                if %8 [t: $B9, f: $B10] {  # if_2
                  $B9: {  # true
                    exit_loop  # loop_2
                  }
                  $B10: {  # false
                    continue  # -> $B8
                  }
                }
                continue  # -> $B8
              }
              $B8: {  # continuing
                store %computed, %12
                store %copied, %11
                next_iteration  # -> $B7
              }
            }
            continue  # -> $B4
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_MultiBlockLoopIndex) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %uint = OpTypeInt 32 0
        %pty = OpTypePointer Private %uint
          %1 = OpVariable %pty Private
    %boolpty = OpTypePointer Private %bool
          %7 = OpVariable %boolpty Private
          %8 = OpVariable %boolpty Private
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
          %5 = OpLabel
               OpBranch %10
; Use an outer loop to show we put the new variable in the
; smallest enclosing scope.
         %10 = OpLabel
        %101 = OpLoad %bool %7
        %102 = OpLoad %bool %8
               OpLoopMerge %99 %89 None
               OpBranchConditional %101 %99 %20
         %20 = OpLabel
          %2 = OpPhi %uint %uint_0 %10 %4 %30  ; gets computed value
          %3 = OpPhi %uint %uint_1 %10 %3 %30  ; gets itself
               OpLoopMerge %79 %30 None
               OpBranchConditional %102 %79 %30
         %30 = OpLabel
          %4 = OpIAdd %uint %2 %uint_1
               OpBranch %20
         %79 = OpLabel
               OpBranch %89
         %89 = OpLabel ; continue target for outer loop
               OpBranch %10
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<private, u32, read_write> = var undef
  %2:ptr<private, bool, read_write> = var undef
  %3:ptr<private, bool, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<function, u32, read_write> = var undef
    %6:ptr<function, u32, read_write> = var undef
    loop [b: $B3, c: $B4] {  # loop_1
      $B3: {  # body
        %7:bool = load %2
        %8:bool = load %3
        store %6, 0u
        store %5, 1u
        if %7 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_loop  # loop_1
          }
          $B6: {  # false
            loop [b: $B7, c: $B8] {  # loop_2
              $B7: {  # body
                %9:u32 = load %6
                %10:u32 = load %5
                if %8 [t: $B9, f: $B10] {  # if_2
                  $B9: {  # true
                    exit_loop  # loop_2
                  }
                  $B10: {  # false
                    %11:u32 = spirv.add<u32> %9, 1u
                    store %6, %11
                    store %5, %10
                    exit_loop  # loop_2
                  }
                }
                continue  # -> $B8
              }
              $B8: {  # continuing
                %12:u32 = spirv.add<u32> %9, 1u
                store %6, %12
                store %5, %10
                next_iteration  # -> $B7
              }
            }
            continue  # -> $B4
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        next_iteration  # -> $B3
      }
    }
    ret
  }
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_ValueFromLoopBodyAndContinuing) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %uint = OpTypeInt 32 0
        %pty = OpTypePointer Private %uint
          %1 = OpVariable %pty Private
    %boolpty = OpTypePointer Private %bool
         %17 = OpVariable %boolpty Private
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
          %9 = OpLabel
        %101 = OpLoad %bool %17
               OpBranch %10
; Use an outer loop to show we put the new variable in the
; smallest enclosing scope.
         %10 = OpLabel
               OpLoopMerge %99 %89 None
               OpBranch %20
         %20 = OpLabel
          %2 = OpPhi %uint %uint_0 %10 %4 %30  ; gets computed value
          %5 = OpPhi %uint %uint_1 %10 %7 %30
          %4 = OpIAdd %uint %2 %uint_1 ; define %4
          %6 = OpIAdd %uint %4 %uint_1 ; use %4
               OpLoopMerge %79 %30 None
               OpBranchConditional %101 %79 %30
         %30 = OpLabel
          %7 = OpIAdd %uint %4 %6 ; use %4 again
          %8 = OpCopyObject %uint %5 ; use %5
               OpBranch %20
         %79 = OpLabel
               OpBranch %89
         %89 = OpLabel
               OpBranch %10
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<private, u32, read_write> = var undef
  %2:ptr<private, bool, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %4:ptr<function, u32, read_write> = var undef
    %5:ptr<function, u32, read_write> = var undef
    %6:bool = load %2
    loop [b: $B3, c: $B4] {  # loop_1
      $B3: {  # body
        store %5, 0u
        store %4, 1u
        loop [b: $B5, c: $B6] {  # loop_2
          $B5: {  # body
            %7:u32 = load %5
            %8:u32 = load %4
            %9:u32 = spirv.add<u32> %7, 1u
            %10:u32 = spirv.add<u32> %9, 1u
            if %6 [t: $B7, f: $B8] {  # if_1
              $B7: {  # true
                exit_loop  # loop_2
              }
              $B8: {  # false
                %11:u32 = spirv.add<u32> %9, %10
                %12:u32 = let %8
                store %5, %9
                store %4, %11
                exit_loop  # loop_2
              }
            }
            continue  # -> $B6
          }
          $B6: {  # continuing
            %13:u32 = spirv.add<u32> %9, %10
            %14:u32 = let %8
            store %5, %9
            store %4, %13
            next_iteration  # -> $B5
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_FromElseAndThen) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %uint = OpTypeInt 32 0
        %pty = OpTypePointer Private %uint
          %1 = OpVariable %pty Private
    %boolpty = OpTypePointer Private %bool
          %7 = OpVariable %boolpty Private
          %8 = OpVariable %boolpty Private
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
     %uint_3 = OpConstant %uint 3
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
          %5 = OpLabel
        %101 = OpLoad %bool %7
        %102 = OpLoad %bool %8
               OpBranch %10
; Use an outer loop to show we put the new variable in the
; smallest enclosing scope.
         %10 = OpLabel
               OpLoopMerge %99 %89 None
               OpBranchConditional %101 %99 %20
         %20 = OpLabel ; if seleciton
               OpSelectionMerge %79 None
               OpBranchConditional %102 %30 %40
         %30 = OpLabel
               OpBranch %89
         %40 = OpLabel
               OpBranch %89
         %79 = OpLabel ; disconnected selection merge node
               OpBranch %89
         %89 = OpLabel
          %2 = OpPhi %uint %uint_0 %30 %uint_1 %40 %uint_3 %79
               OpStore %1 %2
               OpBranch %10
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
$B1: {  # root
  %1:ptr<private, u32, read_write> = var undef
  %2:ptr<private, bool, read_write> = var undef
  %3:ptr<private, bool, read_write> = var undef
}

%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %5:ptr<function, u32, read_write> = var undef
    %6:bool = load %2
    %7:bool = load %3
    loop [b: $B3, c: $B4] {  # loop_1
      $B3: {  # body
        if %6 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_loop  # loop_1
          }
          $B6: {  # false
            if %7 [t: $B7, f: $B8] {  # if_2
              $B7: {  # true
                store %5, 0u
                continue  # -> $B4
              }
              $B8: {  # false
                store %5, 1u
                continue  # -> $B4
              }
            }
            if true [t: $B9, f: $B10] {  # if_3
              $B9: {  # true
                store %5, 3u
                continue  # -> $B4
              }
              $B10: {  # false
                unreachable
              }
            }
            store %5, 3u
            continue  # -> $B4
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %8:u32 = load %5
        store %1, %8
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, DISABLED_Phi_FromHeaderAndThen) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %uint = OpTypeInt 32 0
        %pty = OpTypePointer Private %uint
          %1 = OpVariable %pty Private
    %boolpty = OpTypePointer Private %bool
          %7 = OpVariable %boolpty Private
          %8 = OpVariable %boolpty Private
     %uint_0 = OpConstant %uint 0
     %uint_1 = OpConstant %uint 1
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
          %5 = OpLabel
        %101 = OpLoad %bool %7
        %102 = OpLoad %bool %8
               OpBranch %10
; Use an outer loop to show we put the new variable in the
; smallest enclosing scope.
         %10 = OpLabel
               OpLoopMerge %99 %89 None
               OpBranchConditional %101 %99 %20
         %20 = OpLabel ; if seleciton
               OpSelectionMerge %79 None
               OpBranchConditional %102 %30 %89
         %30 = OpLabel
               OpBranch %89
         %79 = OpLabel ; disconnected selection merge node
               OpUnreachable
         %89 = OpLabel
          %2 = OpPhi %uint %uint_0 %20 %uint_1 %30
               OpStore %1 %2
               OpBranch %10
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
let x_101 = x_7;
let x_102 = x_8;
loop {
  var x_2 : u32;
  if (x_101) {
    break;
  }
  x_2 = 0u;
  if (x_102) {
    x_2 = 1u;
    continue;
  } else {
    continue;
  }
  return;

  continuing {
    x_1 = x_2;
  }
}
return;
)");
}

// If the only use of a combinatorially computed ID is as the value in an OpPhi, then we still have
// to emit it.  The algorithm fix is to always count uses in Phis. This is the reduced case from the
// bug report.
//
// * The only use of %12 is in the phi.
// * The only use of %11 is in %12.
// * Both definintions need to be emitted to the output.
//
// https://crbug.com/215
TEST_F(SpirvParserTest, DISABLED_Phi_UseInPhiCountsAsUse) {
    EXPECT_IR(
        R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
         %11 = OpLogicalAnd %bool %true %true
         %12 = OpLogicalNot %bool %11  ;
               OpSelectionMerge %99 None
               OpBranchConditional %true %20 %99
         %20 = OpLabel
               OpBranch %99
         %99 = OpLabel
        %101 = OpPhi %bool %11 %10 %12 %20
        %102 = OpCopyObject %bool %101  ;; ensure a use of %101
               OpReturn
               OpFunctionEnd
)",
        R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:ptr<function, bool, read_write> = var undef
    %3:bool = and true, true
    %4:bool = not %3
    store %2, %3
    if true [t: $B2, f: $B3] {  # if_1
      $B2: {  # true
        store %2, %4
        exit_if  # if_1
      }
      $B3: {  # false
        exit_if  # if_1
      }
    }
    %5:bool = load %2
    %6:bool = let %5
    ret
  }
}
)");
}

// Value %999 is defined deep in control flow, then we arrange for it to dominate the backedge of
// the outer loop. The %999 value is then fed back into the phi in the loop header.  So %999 needs
// to be hoisted out of the loop.  The phi assignment needs to use the hoisted variable. The hoisted
// variable needs to be placed such that its scope encloses that phi in the header of the outer
// loop. The compiler needs to "see" that there is an implicit use of %999 in the backedge block of
// that outer loop.
//
// https://crbug.com/1649
TEST_F(SpirvParserTest, DISABLED_Phi_PhiInLoopHeader_FedByHoistedVar_PhiUnused) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %999 %80
               OpLoopMerge %99 %80 None
               OpBranchConditional %true %30 %99
         %30 = OpLabel
               OpSelectionMerge %50 None
               OpBranchConditional %true %40 %50
         %40 = OpLabel
        %999 = OpCopyObject %bool %true
               OpBranch %60
         %50 = OpLabel
               OpReturn
         %60 = OpLabel ; if merge
               OpBranch %80
         %80 = OpLabel ; continue target
               OpBranch %20
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
loop {
  var x_999 : bool;
  if (true) {
  } else {
    break;
  }
  if (true) {
    x_999 = true;
    continue;
  }
  return;
}
return;
)");
}

// Value %999 is defined deep in control flow, then we arrange for it to dominate the backedge of
// the outer loop. The %999 value is then fed back into the phi in the loop header.  So %999 needs
// to be hoisted out of the loop.  The phi assignment needs to use the hoisted variable. The hoisted
// variable needs to be placed such that its scope encloses that phi in the header of the outer
// loop. The compiler needs to "see" that there is an implicit use of %999 in the backedge block of
// that outer loop.
//
// https://crbug.com/1649
TEST_F(SpirvParserTest, DISABLED_Phi_PhiInLoopHeader_FedByHoistedVar_PhiUsed) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %999 %80
               OpLoopMerge %99 %80 None
               OpBranchConditional %true %30 %99
         %30 = OpLabel
               OpSelectionMerge %50 None
               OpBranchConditional %true %40 %50
         %40 = OpLabel
        %999 = OpCopyObject %bool %true
               OpBranch %60
         %50 = OpLabel
               OpReturn
         %60 = OpLabel ; if merge
               OpBranch %80
         %80 = OpLabel ; continue target
               OpBranch %20
         %99 = OpLabel
       %1000 = OpCopyObject %bool %101
               OpReturn
               OpFunctionEnd
)",
              R"(
var x_101 : bool;
x_101 = true;
loop {
  var x_999 : bool;
  if (true) {
  } else {
    break;
  }
  if (true) {
    x_999 = true;
    continue;
  }
  return;

  continuing {
    x_101 = x_999;
  }
}
let x_1000 = x_101;
return;
)");
}

// This is a reduction of one of the hard parts of test case
// vk-gl-cts/graphicsfuzz/stable-binarysearch-tree-false-if-discard-loop/1.spvasm
// In particular, see the data flow around %114 in that case.
//
// Here value %999 is is a *phi* defined deep in control flow, then we arrange for it to dominate
// the backedge of the outer loop. The %999 value is then fed back into the phi in the loop header.
// The variable generated to hold the %999 value needs to be placed such that its scope encloses
// that phi in the header of the outer loop. The compiler needs to "see" that there is an implicit
// use of %999 in the backedge block of that outer loop.
//
// https://crbug.com/1649
TEST_F(SpirvParserTest, DISABLED_Phi_PhiInLoopHeader_FedByPhi_PhiUnused) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
      %false = OpConstantFalse %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %999 %80
               OpLoopMerge %99 %80 None
               OpBranchConditional %true %99 %30
         %30 = OpLabel
               OpLoopMerge %70 %60 None
               OpBranch %40
         %40 = OpLabel
               OpBranchConditional %true %60 %50
         %50 = OpLabel
               OpBranch %60
         %60 = OpLabel ; inner continue
        %999 = OpPhi %bool %true %40 %false %50
               OpBranchConditional %true %70 %30
         %70 = OpLabel  ; inner merge
               OpBranch %80
         %80 = OpLabel ; outer continue target
               OpBranch %20
         %99 = OpLabel
               OpReturn
               OpFunctionEnd
)",
              R"(
loop {
  var x_999 : bool;
  if (true) {
    break;
  }
  loop {
    x_999 = true;
    if (true) {
      continue;
    }
    x_999 = false;

    continuing {
      break if true;
    }
  }
}
return;
)");
}

// This is a reduction of one of the hard parts of test case
// vk-gl-cts/graphicsfuzz/stable-binarysearch-tree-false-if-discard-loop/1.spvasm
// In particular, see the data flow around %114 in that case.
//
// Here value %999 is is a *phi* defined deep in control flow, then we arrange for it to dominate
// the backedge of the outer loop. The %999 value is then fed back into the phi in the loop header.
// The variable generated to hold the %999 value needs to be placed such that its scope encloses
// that phi in the header of the outer loop. The compiler needs to "see" that there is an implicit
// use of %999 in the backedge block of that outer loop.
//
// https://crbug.com/1649
TEST_F(SpirvParserTest, DISABLED_Phi_PhiInLoopHeader_FedByPhi_PhiUsed) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %bool = OpTypeBool
       %true = OpConstantTrue %bool
      %false = OpConstantFalse %bool
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %20
         %20 = OpLabel
        %101 = OpPhi %bool %true %10 %999 %80
               OpLoopMerge %99 %80 None
               OpBranchConditional %true %99 %30
         %30 = OpLabel
               OpLoopMerge %70 %60 None
               OpBranch %40
         %40 = OpLabel
               OpBranchConditional %true %60 %50
         %50 = OpLabel
               OpBranch %60
         %60 = OpLabel ; inner continue
        %999 = OpPhi %bool %true %40 %false %50
               OpBranchConditional %true %70 %30
         %70 = OpLabel  ; inner merge
               OpBranch %80
         %80 = OpLabel ; outer continue target
               OpBranch %20
         %99 = OpLabel
       %1000 = OpCopyObject %bool %101
               OpReturn
               OpFunctionEnd
)",
              R"(
var x_101 : bool;
x_101 = true;
loop {
  var x_999 : bool;
  if (true) {
    break;
  }
  loop {
    x_999 = true;
    if (true) {
      continue;
    }
    x_999 = false;

    continuing {
      break if true;
    }
  }

  continuing {
    x_101 = x_999;
  }
}
let x_1000 = x_101;
return;
)");
}

// A phi in an unreachable block may have no operands.
TEST_F(SpirvParserTest, DISABLED_Phi_UnreachableLoopMerge) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %uint = OpTypeInt 32 0
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
         %10 = OpLabel
               OpBranch %99
         %99 = OpLabel
               OpLoopMerge %101 %99 None
               OpBranch %99
        %101 = OpLabel
        %102 = OpPhi %uint
               OpUnreachable
               OpFunctionEnd
)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        continue  # -> $B3
      }
      $B3: {  # continuing
        next_iteration  # -> $B2
      }
    }
    unreachable
  }
}
)");
}

}  // namespace
}  // namespace tint::spirv::reader
