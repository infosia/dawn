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

#include "src/tint/lang/core/ir/transform/print.h"

#include <utility>

#include "src/tint/lang/core/ir/transform/helper_test.h"

namespace tint::core::ir::transform {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using IR_PrintTest = TransformTest;

TEST_F(IR_PrintTest, DirectUse) {
    auto* buffer = b.Var<uniform, vec4<u32>>("buffer");
    buffer->SetBindingPoint(0, 0);
    mod.root_block->Append(buffer);

    auto* func = b.ComputeFunction("foo");
    b.Append(func->Block(), [&] {
        b.Call<u32>(BuiltinFn::kPrint, b.Load(buffer));
        b.Return(func);
    });

    auto* src = R"(
$B1: {  # root
  %buffer:ptr<uniform, vec4<u32>, read> = var @binding_point(0, 0)
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B2: {
    %3:vec4<u32> = load %buffer
    %4:u32 = print %3
    ret
  }
}
)";
    EXPECT_EQ(src, str());

    auto* expect = R"(
TintPrintEntry = struct @align(16) {
  context:vec4<u32> @offset(0)
  data:vec4<u32> @offset(16)
}

TintPrintBuffer = struct @align(16) {
  print_index:atomic<u32> @offset(0)
  print_entries:array<TintPrintEntry, 100> @offset(16)
}

$B1: {  # root
  %buffer:ptr<uniform, vec4<u32>, read> = var @binding_point(0, 0)
  %tint_print_invocation_id:ptr<private, vec3<u32>, read_write> = var
  %tint_print_buffer:ptr<storage, TintPrintBuffer, read_write> = var @binding_point(1, 2)
}

%foo = @compute @workgroup_size(1u, 1u, 1u) func(%tint_symbol:vec3<u32> [@global_invocation_id]):void {
  $B2: {
    store %tint_print_invocation_id, %tint_symbol
    %6:vec4<u32> = load %buffer
    %7:vec3<u32> = load %tint_print_invocation_id
    %8:vec4<u32> = construct 1u, %7
    %9:TintPrintEntry = construct %8, %6
    %10:ptr<storage, atomic<u32>, read_write> = access %tint_print_buffer, 0u
    %11:u32 = atomicAdd %10, 1u
    %12:bool = lt %11, 100u
    if %12 [t: $B3] {  # if_1
      $B3: {  # true
        %13:ptr<storage, TintPrintEntry, read_write> = access %tint_print_buffer, 1u, %11
        store %13, %9
        exit_if  # if_1
      }
    }
    ret
  }
}
)";

    Run(Print, BindingPoint{1, 2});

    EXPECT_EQ(expect, str());
}

// TODO(jrprice): Test all the types
// TODO(jrprice): Compute vs Fragment vs Vertex
// TODO(jrprice): With and without pre-existing builtins

}  // namespace
}  // namespace tint::core::ir::transform
