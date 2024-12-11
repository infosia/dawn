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

#include "src/tint/lang/core/ir/analysis/integer_range_analysis.h"

#include <utility>

#include "src/tint/lang/core/ir/ir_helper_test.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::core::ir::analysis {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

class IR_IntegerRangeAnalysisTest : public IRTestHelper {};

TEST_F(IR_IntegerRangeAnalysisTest, LocalInvocationIndex) {
    auto* func = b.ComputeFunction("my_func", 4_u, 1_u, 1_u);
    auto* localInvocationIndex = b.FunctionParam("localInvocationIndex", mod.Types().u32());
    localInvocationIndex->SetBuiltin(tint::core::BuiltinValue::kLocalInvocationIndex);
    func->SetParams({localInvocationIndex});

    b.Append(func->Block(), [&] {
        auto* dst = b.Var(ty.ptr<function, array<u32, 4u>>());
        auto* access_dst = b.Access(ty.ptr<function, u32>(), dst, localInvocationIndex);
        b.Store(access_dst, localInvocationIndex);
        b.Return(func);
    });

    auto* src = R"(
%my_func = @compute @workgroup_size(4u, 1u, 1u) func(%localInvocationIndex:u32 [@local_invocation_index]):void {
  $B1: {
    %3:ptr<function, array<u32, 4>, read_write> = var
    %4:ptr<function, u32, read_write> = access %3, %localInvocationIndex
    store %4, %localInvocationIndex
    ret
  }
}
)";
    EXPECT_EQ(src, str());
    EXPECT_EQ(Validate(mod), Success);

    IntegerRangeAnalysis analysis(*func);
    auto* info = analysis.GetInfo(localInvocationIndex);
    ASSERT_NE(nullptr, info);
    ASSERT_TRUE(std::holds_alternative<IntegerRangeInfo::UnsignedIntegerRange>(info->range));

    const auto& range = std::get<IntegerRangeInfo::UnsignedIntegerRange>(info->range);
    EXPECT_EQ(0u, range.min_bound);
    EXPECT_EQ(3u, range.max_bound);
}

}  // namespace
}  // namespace tint::core::ir::analysis
