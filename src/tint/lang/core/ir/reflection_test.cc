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

#include "src/tint/lang/core/ir/reflection.h"

#include <string>

#include "gmock/gmock.h"
#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/utils/result.h"

namespace tint::core::ir {
namespace {

using ::testing::ElementsAre;

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class IR_ReflectionTest : public IRTestHelper {
  protected:
    /// @returns the module as a disassembled string
    std::string Disassemble() const { return "\n" + ir::Disassembler(mod).Plain(); }
};

TEST_F(IR_ReflectionTest, GetWorkgroupInfoBasic) {
    // Referenced.
    auto* var_a = mod.root_block->Append(b.Var<workgroup, u32>("a"));
    auto* foo = b.ComputeFunction("foo", u32(3), u32(5), u32(7));
    b.Append(foo->Block(), [&] {  //
        b.Load(var_a);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<workgroup, u32, read_write> = var undef
}

%foo = @compute @workgroup_size(3u, 5u, 7u) func():void {
  $B2: {
    %3:u32 = load %a
    ret
  }
}
)";
    EXPECT_EQ(src, Disassemble());

    auto res = GetWorkgroupInfo(mod);
    EXPECT_TRUE(res == tint::Success);
    EXPECT_EQ(res->x, 3u);
    EXPECT_EQ(res->y, 5u);
    EXPECT_EQ(res->z, 7u);
    EXPECT_EQ(res->storage_size, 16u);
}

TEST_F(IR_ReflectionTest, GetWorkgroupInfoFailNoWorkgroupSize) {
    // Referenced.
    auto* var_a = mod.root_block->Append(b.Var<workgroup, u32>("a"));
    auto* foo = b.Function("foo", ty.void_(), Function::PipelineStage::kCompute);
    b.Append(foo->Block(), [&] {  //
        b.Load(var_a);
        b.Return(foo);
    });

    auto* src = R"(
$B1: {  # root
  %a:ptr<workgroup, u32, read_write> = var undef
}

%foo = @compute func():void {
  $B2: {
    %3:u32 = load %a
    ret
  }
}
)";
    EXPECT_EQ(src, Disassemble());

    auto res = GetWorkgroupInfo(mod);
    EXPECT_FALSE(res == tint::Success);
}

}  // namespace
}  // namespace tint::core::ir
