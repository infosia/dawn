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

#include "src/tint/lang/msl/ir/reinterpret_cast.h"

#include "gmock/gmock.h"
#include "src/tint/lang/core/ir/ir_helper_test.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::msl::ir {
namespace {

using MslIRTest = core::ir::IRTestHelper;

TEST_F(MslIRTest, SetsUsage) {
    auto* val = b.Constant(u32(1));
    auto* offset = b.Constant(u32(0));
    auto* t = b.ir.CreateInstruction<ReinterpretCast>(b.InstructionResult(ty.f32()), val, offset);

    EXPECT_THAT(val->UsagesUnsorted(), testing::UnorderedElementsAre(core::ir::Usage{t, 0u}));
}

TEST_F(MslIRTest, Result) {
    auto* val = b.Constant(u32(1));
    auto* offset = b.Constant(u32(0));
    auto* t = b.ir.CreateInstruction<ReinterpretCast>(b.InstructionResult(ty.i32()), val, offset);

    EXPECT_EQ(t->Results().Length(), 1u);

    EXPECT_TRUE(t->Result()->Is<core::ir::InstructionResult>());
    EXPECT_EQ(t, t->Result()->Instruction());
}

TEST_F(MslIRTest, Clone) {
    auto* val = b.Constant(u32(1));
    auto* offset = b.Constant(u32(0));
    auto* t = b.ir.CreateInstruction<ReinterpretCast>(b.InstructionResult(ty.f32()), val, offset);

    auto* new_t = clone_ctx.Clone(t);

    EXPECT_NE(t, new_t);

    EXPECT_NE(t->Result(), new_t->Result());
    EXPECT_EQ(ty.f32(), new_t->Result()->Type());

    EXPECT_NE(nullptr, new_t->Val());
    EXPECT_EQ(t->Val(), new_t->Val());
}

}  // namespace
}  // namespace tint::msl::ir
