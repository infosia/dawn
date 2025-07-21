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

#include "src/tint/lang/core/ir/analysis/for_loop_analysis.h"

#include <utility>

#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::core::ir::analysis {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class IR_ForLoopAnalysisTest : public IRTestHelper {};

TEST_F(IR_ForLoopAnalysisTest, SimpleLoopCondition) {
    Var* idx = nullptr;
    Loop* loop = nullptr;
    auto* func = b.Function("func", ty.void_());
    If* ifelse = nullptr;
    b.Append(func->Block(), [&] {
        loop = b.Loop();
        b.Append(loop->Initializer(), [&] {  //
            idx = b.Var("idx", 0_u);
            b.NextIteration(loop);
        });
        b.Append(loop->Body(), [&] {
            ifelse = b.If(b.LessThan<bool>(b.Load(idx), 10_u));
            b.Append(ifelse->True(), [&] {  //
                b.ExitIf(ifelse);
            });
            b.Append(ifelse->False(), [&] {  //
                b.ExitLoop(loop);
            });
            b.Continue(loop);
        });
        b.Append(loop->Continuing(), [&] {  //
            b.Store(idx, b.Add<u32>(b.Load(idx), 1_u));
            b.NextIteration(loop);
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %idx:ptr<function, u32, read_write> = var 0u
        next_iteration  # -> $B3
      }
      $B3: {  # body
        %3:u32 = load %idx
        %4:bool = lt %3, 10u
        if %4 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_if  # if_1
          }
          $B6: {  # false
            exit_loop  # loop_1
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %5:u32 = load %idx
        %6:u32 = add %5, 1u
        store %idx, %6
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    EXPECT_EQ(Validate(mod), Success);

    ForLoopAnalysis analysis(*loop);
    auto* info = analysis.GetTargetIf();
    EXPECT_EQ(ifelse, info);
}

TEST_F(IR_ForLoopAnalysisTest, ConditionUniformAccess) {
    Var* U = nullptr;
    b.Append(b.ir.root_block,
             [&] {  //
                 U = b.Var<uniform, array<vec4<u32>, 8>>("U");
                 U->SetBindingPoint(0, 0);
             });

    auto* fn_a = b.Function("a", ty.vec4<i32>());
    auto* fn_a_p = b.FunctionParam("p", ty.ptr<uniform, vec4<i32>>());
    fn_a->SetParams({
        b.FunctionParam("pre", ty.i32()),
        fn_a_p,
        b.FunctionParam("post", ty.i32()),
    });
    b.Append(fn_a->Block(), [&] { b.Return(fn_a, b.Load(fn_a_p)); });
    Var* idx = nullptr;
    Loop* loop = nullptr;
    If* ifelse = nullptr;
    auto* fn_b = b.Function("b", ty.void_());
    b.Append(fn_b->Block(), [&] {
        auto* I = b.Let("I", 3_i);
        auto* access = b.Access(ty.ptr<uniform, vec4<u32>>(), U, I);
        auto* load = b.LoadVectorElement(access, 0_u);
        loop = b.Loop();
        b.Append(loop->Initializer(), [&] {  //
            idx = b.Var("idx", 0_u);
            b.NextIteration(loop);
        });
        b.Append(loop->Body(), [&] {
            ifelse = b.If(b.LessThan<bool>(load, 10_u));
            b.Append(ifelse->True(), [&] {  //
                b.ExitIf(ifelse);
            });
            b.Append(ifelse->False(), [&] {  //
                b.ExitLoop(loop);
            });
            b.Continue(loop);
        });
        b.Append(loop->Continuing(), [&] {  //
            b.Store(idx, b.Add<u32>(b.Load(idx), 1_u));
            b.NextIteration(loop);
        });
        b.Return(fn_b);
    });

    auto* src = R"(
$B1: {  # root
  %U:ptr<uniform, array<vec4<u32>, 8>, read> = var undef @binding_point(0, 0)
}

%a = func(%pre:i32, %p:ptr<uniform, vec4<i32>, read>, %post:i32):vec4<i32> {
  $B2: {
    %6:vec4<i32> = load %p
    ret %6
  }
}
%b = func():void {
  $B3: {
    %I:i32 = let 3i
    %9:ptr<uniform, vec4<u32>, read> = access %U, %I
    %10:u32 = load_vector_element %9, 0u
    loop [i: $B4, b: $B5, c: $B6] {  # loop_1
      $B4: {  # initializer
        %idx:ptr<function, u32, read_write> = var 0u
        next_iteration  # -> $B5
      }
      $B5: {  # body
        %12:bool = lt %10, 10u
        if %12 [t: $B7, f: $B8] {  # if_1
          $B7: {  # true
            exit_if  # if_1
          }
          $B8: {  # false
            exit_loop  # loop_1
          }
        }
        continue  # -> $B6
      }
      $B6: {  # continuing
        %13:u32 = load %idx
        %14:u32 = add %13, 1u
        store %idx, %14
        next_iteration  # -> $B5
      }
    }
    ret
  }
}
)";

    EXPECT_EQ(src, str());
    EXPECT_EQ(Validate(mod), Success);

    ForLoopAnalysis analysis(*loop);
    auto* info = analysis.GetTargetIf();
    EXPECT_EQ(ifelse, info);
}

}  // namespace
}  // namespace tint::core::ir::analysis
