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

#include "src/tint/lang/spirv/reader/lower/lower.h"

#include "src/tint/lang/core/ir/transform/helper_test.h"

namespace tint::spirv::reader::lower {
namespace {

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

using SpirvReader_ControlFlowTest = core::ir::transform::TransformTest;

// Ensure a pointer produced by an access chain in the true branch of an if
// can be used in the false branch after lowering by re-materializing the
// access at the use site (LocalizePointerOperands). Without localization,
// the pointer value would be out-of-scope and validation should fail.
TEST_F(SpirvReader_ControlFlowTest, LocalizeBranchLocalPointer_IfSiblingUse) {
    // Uniform buffer: struct U { x : u32; } @group(0) @binding(1)
    auto* U = ty.Struct(mod.symbols.New("U"), {{mod.symbols.New("x"), ty.u32()}});
    auto* ub = b.Var("ub", ty.ptr(uniform, U));
    ub->SetBindingPoint(0, 1);
    mod.root_block->Append(ub);

    // @compute @workgroup_size(1,1,1) fn main()
    auto* fn = b.ComputeFunction("main", 1_u, 1_u, 1_u);
    b.Append(fn->Block(), [&] {
        auto* cond = b.Equal(ty.bool_(), 0_u, 0_u);  // true
        auto* iff = b.If(cond);

        core::ir::Value* branch_ptr = nullptr;
        b.Append(iff->True(), [&] {
            // Branch-local pointer, does not dominate the false branch
            auto* p = b.Access(ty.ptr<uniform, u32>(), ub, 0_u);
            branch_ptr = p->Result(0);
            b.ExitIf(iff);
        });
        b.Append(iff->False(), [&] {
            // Use the branch-local pointer in sibling branch
            auto* x = b.Load(branch_ptr);
            // Do something with the value to keep it live
            auto* y = b.Add(ty.u32(), x, 1_u);
            // Prevent unused warnings by storing to a function var
            auto* tmp = b.Var<function, u32>("tmp");
            b.Store(tmp, y);
            b.ExitIf(iff);
        });

        b.Return(fn);
    });

    // Run Lower. This relies on LocalizePointerOperands to re-materialize the
    // access at the use, keeping validation happy.
    Run(Lower);

    auto after = str();
    // Expect to still see an if
    ASSERT_NE(std::string::npos, after.find("if ")) << after;
    // Expect two access operations to the same uniform element: one from the true branch,
    // and one localized in the false branch
    size_t first = after.find("access %ub, 0u");
    ASSERT_NE(std::string::npos, first) << after;
    size_t second = after.find("access %ub, 0u", first + 1);
    ASSERT_NE(std::string::npos, second) << after;
}

}  // namespace
}  // namespace tint::spirv::reader::lower
