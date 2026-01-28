// Copyright 2026 The Dawn & Tint Authors
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

#include "src/tint/lang/hlsl/writer/raise/extract_ternary_values.h"

#include "src/tint/lang/core/ir/block.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/constant.h"
#include "src/tint/lang/core/ir/exit.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/switch.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/hlsl/ir/ternary.h"
#include "src/tint/utils/containers/vector.h"

namespace tint::hlsl::writer::raise {
namespace {

using namespace tint::core::fluent_types;  // NOLINT

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// Process the module.
    void Process() {
        // Collect the ternary instructions that need to be modified.
        Vector<hlsl::ir::Ternary*, 4> worklist;
        for (auto* inst : ir.Instructions()) {
            if (auto* ternary = inst->As<hlsl::ir::Ternary>()) {
                if (ternary->Cmp()->Is<core::ir::Constant>() &&
                    ternary->Cmp()->Type()->Is<core::type::Bool>()) {  // No need to process vecs
                    worklist.Push(ternary);
                }
            }
        }

        // Process the collected ternaries.
        for (auto* ternary : worklist) {
            // Constant scalar bool values as the ternary cond may result in unintended
            // optimizations in DXC that prevent both sides of the expression from being
            // evaluated. Extracting the true and false values to variables prevents this.
            core::ir::Instruction* new_ternary = nullptr;
            b.InsertBefore(ternary, [&] {
                // Create vars for the true and false values.
                auto* new_false = b.Let(ternary->False());
                auto* new_true = b.Let(ternary->True());

                // Create a new ternary with the same operands, but using the var results.
                new_ternary = ir.CreateInstruction<hlsl::ir::Ternary>(
                    b.InstructionResult(ternary->Result()->Type()),
                    Vector{new_false->Result(), new_true->Result(), ternary->Cmp()});
                b.Append(new_ternary);
            });

            // Replace all uses of the old ternary's result with the new one.
            ternary->Result()->ReplaceAllUsesWith(new_ternary->Result());
            ternary->Destroy();
        }
    }
};

}  // namespace

Result<SuccessType> ExtractTernaryValues(core::ir::Module& ir) {
    TINT_CHECK_RESULT(ValidateAndDumpIfNeeded(ir, "hlsl.ExtractTernaryValues",
                                              core::ir::Capabilities{
                                                  core::ir::Capability::kAllowNonCoreTypes,
                                                  core::ir::Capability::kAllowVectorElementPointer,
                                              }));

    State{ir}.Process();

    return Success;
}

}  // namespace tint::hlsl::writer::raise
