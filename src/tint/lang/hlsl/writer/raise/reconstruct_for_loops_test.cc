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

#include "src/tint/lang/hlsl/writer/raise/reconstruct_for_loops.h"

#include "src/tint/lang/core/ir/transform/helper_test.h"

#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/transform/helper_test.h"
#include "src/tint/lang/core/number.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::hlsl::writer::raise {
namespace {

using HlslWriterReconstuctForLoopsTest = core::ir::transform::TransformTest;

TEST_F(HlslWriterReconstuctForLoopsTest, SimpleFiniteLoop) {
    auto* func = b.Function("func", ty.void_());
    b.Append(func->Block(), [&] {  //
        auto* loop = b.Loop();
        b.Append(loop->Initializer(), [&] {
            auto* idx = b.Var<function, u32>("idx");
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* ifelse = b.If(b.LessThan<bool>(b.Load(idx), 10_u));
                b.Append(ifelse->True(), [&] {  //
                    b.ExitIf(ifelse);
                });
                b.Append(ifelse->False(), [&] {  //
                    b.ExitLoop(loop);
                });
                b.Continue(loop);

                b.Append(loop->Continuing(), [&] {  //
                    b.Store(idx, b.Add<u32>(b.Load(idx), 1_u));
                    b.NextIteration(loop);
                });
            });
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %idx:ptr<function, u32, read_write> = var undef
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

    auto* expect = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %idx:ptr<function, u32, read_write> = var undef
        next_iteration  # -> $B3
      }
      $B3: {  # body
        continue  # -> $B4
      }
      $B4: {  # continuing
        %3:u32 = load %idx
        %4:u32 = add %3, 1u
        store %idx, %4
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";

    Run(ReconstuctForLoops);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterReconstuctForLoopsTest, NoIfs_Unaffected) {
    auto* func = b.Function("func", ty.void_());
    b.Append(func->Block(), [&] {
        auto* loop = b.Loop();
        b.Append(loop->Body(), [&] {  //
            b.Continue(loop);
        });
        b.Append(loop->Continuing(), [&] {  //
            b.NextIteration(loop);
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [b: $B2, c: $B3] {  # loop_1
      $B2: {  # body
        continue  # -> $B3
      }
      $B3: {  # continuing
        next_iteration  # -> $B2
      }
    }
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    Run(ReconstuctForLoops);

    EXPECT_EQ(src, str());
}

TEST_F(HlslWriterReconstuctForLoopsTest, TwoIfs_Unaffected) {
    auto* func = b.Function("func", ty.void_());
    b.Append(func->Block(), [&] {  //
        auto* loop = b.Loop();
        b.Append(loop->Initializer(), [&] {
            auto* idx = b.Var<function, u32>("idx");
            b.NextIteration(loop);

            b.Append(loop->Body(), [&] {
                auto* ifelse1 = b.If(b.LessThan<bool>(b.Load(idx), 10_u));
                b.Append(ifelse1->True(), [&] { b.ExitIf(ifelse1); });
                b.Append(ifelse1->False(), [&] { b.ExitLoop(loop); });

                auto* ifelse2 = b.If(b.GreaterThan<bool>(b.Load(idx), 0_u));
                b.Append(ifelse2->True(), [&] { b.ExitIf(ifelse2); });
                b.Append(ifelse2->False(), [&] { b.ExitLoop(loop); });

                b.Continue(loop);

                b.Append(loop->Continuing(), [&] {
                    b.Store(idx, b.Add<u32>(b.Load(idx), 1_u));
                    b.NextIteration(loop);
                });
            });
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %idx:ptr<function, u32, read_write> = var undef
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
        %5:u32 = load %idx
        %6:bool = gt %5, 0u
        if %6 [t: $B7, f: $B8] {  # if_2
          $B7: {  # true
            exit_if  # if_2
          }
          $B8: {  # false
            exit_loop  # loop_1
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %7:u32 = load %idx
        %8:u32 = add %7, 1u
        store %idx, %8
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    Run(ReconstuctForLoops);

    EXPECT_EQ(src, str());
}

TEST_F(HlslWriterReconstuctForLoopsTest, NestedFiniteLoops) {
    auto* func = b.Function("func", ty.void_());
    b.Append(func->Block(), [&] {
        auto* outer_loop = b.Loop();
        b.Append(outer_loop->Initializer(), [&] {
            auto* i = b.Var<function, u32>("i");
            b.NextIteration(outer_loop);

            b.Append(outer_loop->Body(), [&] {
                auto* ifelse_outer = b.If(b.LessThan<bool>(b.Load(i), 4_u));
                b.Append(ifelse_outer->True(), [&] { b.ExitIf(ifelse_outer); });
                b.Append(ifelse_outer->False(), [&] { b.ExitLoop(outer_loop); });

                auto* inner_loop = b.Loop();
                b.Append(inner_loop->Initializer(), [&] {
                    auto* j = b.Var<function, u32>("j");
                    b.NextIteration(inner_loop);

                    b.Append(inner_loop->Body(), [&] {
                        auto* ifelse_inner = b.If(b.LessThan<bool>(b.Load(j), 8_u));
                        b.Append(ifelse_inner->True(), [&] { b.ExitIf(ifelse_inner); });
                        b.Append(ifelse_inner->False(), [&] { b.ExitLoop(inner_loop); });

                        b.Continue(inner_loop);

                        b.Append(inner_loop->Continuing(), [&] {
                            b.Store(j, b.Add<u32>(b.Load(j), 1_u));
                            b.NextIteration(inner_loop);
                        });
                    });
                });

                b.Continue(outer_loop);

                b.Append(outer_loop->Continuing(), [&] {
                    b.Store(i, b.Add<u32>(b.Load(i), 1_u));
                    b.NextIteration(outer_loop);
                });
            });
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %i:ptr<function, u32, read_write> = var undef
        next_iteration  # -> $B3
      }
      $B3: {  # body
        %3:u32 = load %i
        %4:bool = lt %3, 4u
        if %4 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_if  # if_1
          }
          $B6: {  # false
            exit_loop  # loop_1
          }
        }
        loop [i: $B7, b: $B8, c: $B9] {  # loop_2
          $B7: {  # initializer
            %j:ptr<function, u32, read_write> = var undef
            next_iteration  # -> $B8
          }
          $B8: {  # body
            %6:u32 = load %j
            %7:bool = lt %6, 8u
            if %7 [t: $B10, f: $B11] {  # if_2
              $B10: {  # true
                exit_if  # if_2
              }
              $B11: {  # false
                exit_loop  # loop_2
              }
            }
            continue  # -> $B9
          }
          $B9: {  # continuing
            %8:u32 = load %j
            %9:u32 = add %8, 1u
            store %j, %9
            next_iteration  # -> $B8
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %10:u32 = load %i
        %11:u32 = add %10, 1u
        store %i, %11
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %i:ptr<function, u32, read_write> = var undef
        next_iteration  # -> $B3
      }
      $B3: {  # body
        loop [i: $B5, b: $B6, c: $B7] {  # loop_2
          $B5: {  # initializer
            %j:ptr<function, u32, read_write> = var undef
            next_iteration  # -> $B6
          }
          $B6: {  # body
            continue  # -> $B7
          }
          $B7: {  # continuing
            %4:u32 = load %j
            %5:u32 = add %4, 1u
            store %j, %5
            next_iteration  # -> $B6
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %6:u32 = load %i
        %7:u32 = add %6, 1u
        store %i, %7
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";

    Run(ReconstuctForLoops);

    EXPECT_EQ(expect, str());
}

TEST_F(HlslWriterReconstuctForLoopsTest, Nested_OuterTransformable) {
    auto* func = b.Function("func", ty.void_());
    b.Append(func->Block(), [&] {
        auto* outer_loop = b.Loop();
        b.Append(outer_loop->Initializer(), [&] {
            auto* i = b.Var<function, u32>("i");
            b.NextIteration(outer_loop);

            b.Append(outer_loop->Body(), [&] {
                auto* ifelse_outer = b.If(b.LessThan<bool>(b.Load(i), 4_u));
                b.Append(ifelse_outer->True(), [&] { b.ExitIf(ifelse_outer); });
                b.Append(ifelse_outer->False(), [&] { b.ExitLoop(outer_loop); });

                auto* inner_loop = b.Loop();
                b.Append(inner_loop->Initializer(), [&] {
                    auto* j = b.Var<function, u32>("j");
                    b.NextIteration(inner_loop);

                    b.Append(inner_loop->Body(), [&] {
                        auto* ifelse_inner1 = b.If(b.LessThan<bool>(b.Load(j), 8_u));
                        b.Append(ifelse_inner1->True(), [&] { b.ExitIf(ifelse_inner1); });
                        b.Append(ifelse_inner1->False(), [&] { b.ExitLoop(inner_loop); });

                        auto* ifelse_inner2 = b.If(b.GreaterThan<bool>(b.Load(j), 0_u));
                        b.Append(ifelse_inner2->True(), [&] { b.ExitIf(ifelse_inner2); });
                        b.Append(ifelse_inner2->False(), [&] { b.ExitLoop(inner_loop); });

                        b.Continue(inner_loop);

                        b.Append(inner_loop->Continuing(), [&] {
                            b.Store(j, b.Add<u32>(b.Load(j), 1_u));
                            b.NextIteration(inner_loop);
                        });
                    });
                });

                b.Continue(outer_loop);

                b.Append(outer_loop->Continuing(), [&] {
                    b.Store(i, b.Add<u32>(b.Load(i), 1_u));
                    b.NextIteration(outer_loop);
                });
            });
        });
        b.Return(func);
    });

    auto* src = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %i:ptr<function, u32, read_write> = var undef
        next_iteration  # -> $B3
      }
      $B3: {  # body
        %3:u32 = load %i
        %4:bool = lt %3, 4u
        if %4 [t: $B5, f: $B6] {  # if_1
          $B5: {  # true
            exit_if  # if_1
          }
          $B6: {  # false
            exit_loop  # loop_1
          }
        }
        loop [i: $B7, b: $B8, c: $B9] {  # loop_2
          $B7: {  # initializer
            %j:ptr<function, u32, read_write> = var undef
            next_iteration  # -> $B8
          }
          $B8: {  # body
            %6:u32 = load %j
            %7:bool = lt %6, 8u
            if %7 [t: $B10, f: $B11] {  # if_2
              $B10: {  # true
                exit_if  # if_2
              }
              $B11: {  # false
                exit_loop  # loop_2
              }
            }
            %8:u32 = load %j
            %9:bool = gt %8, 0u
            if %9 [t: $B12, f: $B13] {  # if_3
              $B12: {  # true
                exit_if  # if_3
              }
              $B13: {  # false
                exit_loop  # loop_2
              }
            }
            continue  # -> $B9
          }
          $B9: {  # continuing
            %10:u32 = load %j
            %11:u32 = add %10, 1u
            store %j, %11
            next_iteration  # -> $B8
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %12:u32 = load %i
        %13:u32 = add %12, 1u
        store %i, %13
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
%func = func():void {
  $B1: {
    loop [i: $B2, b: $B3, c: $B4] {  # loop_1
      $B2: {  # initializer
        %i:ptr<function, u32, read_write> = var undef
        next_iteration  # -> $B3
      }
      $B3: {  # body
        loop [i: $B5, b: $B6, c: $B7] {  # loop_2
          $B5: {  # initializer
            %j:ptr<function, u32, read_write> = var undef
            next_iteration  # -> $B6
          }
          $B6: {  # body
            %4:u32 = load %j
            %5:bool = gt %4, 0u
            if %5 [t: $B8, f: $B9] {  # if_1
              $B8: {  # true
                exit_if  # if_1
              }
              $B9: {  # false
                exit_loop  # loop_2
              }
            }
            continue  # -> $B7
          }
          $B7: {  # continuing
            %6:u32 = load %j
            %7:u32 = add %6, 1u
            store %j, %7
            next_iteration  # -> $B6
          }
        }
        continue  # -> $B4
      }
      $B4: {  # continuing
        %8:u32 = load %i
        %9:u32 = add %8, 1u
        store %i, %9
        next_iteration  # -> $B3
      }
    }
    ret
  }
}
)";

    Run(ReconstuctForLoops);

    EXPECT_EQ(expect, str());
}

}  // namespace
}  // namespace tint::hlsl::writer::raise
