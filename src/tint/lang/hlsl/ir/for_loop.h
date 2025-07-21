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

#ifndef SRC_TINT_LANG_CORE_IR_FOR_LOOP_H_
#define SRC_TINT_LANG_CORE_IR_FOR_LOOP_H_

#include <string>

#include "src/tint/lang/core/ir/call.h"
#include "src/tint/lang/core/ir/clone_context.h"
#include "src/tint/lang/core/ir/instruction_result.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/utils/rtti/castable.h"

#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/utils/containers/const_propagating_ptr.h"

// Forward declarations
namespace tint::core::ir {
class MultiInBlock;
}  // namespace tint::core::ir

namespace tint::hlsl::ir {

/// ForLoop instruction.
///
/// ```
///                     in
///                      ┃
///                      ┣━━━━━━━━━━━┓
///                      ▼           ┃
///             ┌─────────────────┐  ┃
///             │   Initializer   │  ┃
///             │    (optional)   │  ┃
///             └─────────────────┘  ┃
///        NextIteration ┃           ┃
///                      ┃◀━━━━━━━━━━┫
///                      ▼           ┃
///             ┌─────────────────┐  ┃
///          ┏━━│       Body      │  ┃
///          ┃  └─────────────────┘  ┃
///          ┃  Continue ┃           ┃ NextIteration
///          ┃           ▼           ┃
///          ┃  ┌─────────────────┐  ┃ BreakIf(false)
/// ExitLoop ┃  │   Continuing    │━━┛
///          ┃  │  (optional)     │
///          ┃  └─────────────────┘
///          ┃           ┃
///          ┃           ┃ BreakIf(true)
///          ┗━━━━━━━━━━▶┃
///                      ▼
///                     out
///
/// ```
class ForLoop final : public Castable<ForLoop, tint::core::ir::Loop> {
  public:
    /// Constructor (no results, no operands, no blocks)
    /// @param id the instruction id
    explicit ForLoop(Id id);

    /// Constructor
    /// @param id the instruction id
    /// @param i the initializer block
    /// @param cond the condition of the for loop
    /// @param b the body block
    /// @param c the continuing block
    ForLoop(Id id,
            tint::core::ir::Block* i,
            tint::core::ir::MultiInBlock* cond,
            tint::core::ir::MultiInBlock* b,
            tint::core::ir::MultiInBlock* c);
    ~ForLoop() override;

    /// @copydoc Instruction::Clone()
    ForLoop* Clone(tint::core::ir::CloneContext& ctx) override;

    /// @copydoc ControlInstruction::ForeachBlock
    void ForeachBlock(const std::function<void(tint::core::ir::Block*)>& cb) override;

    /// @copydoc ControlInstruction::ForeachBlock
    void ForeachBlock(const std::function<void(const tint::core::ir::Block*)>& cb) const override;

    /// @returns the switch continuing block
    core::ir::MultiInBlock* CondBlock() { return condblock_; }

    /// @returns the switch continuing block
    const core::ir::MultiInBlock* CondBlock() const { return condblock_; }

    /// @returns true if the loop uses an continuing block. If true, then the Loop first branches
    /// to the continuing block, otherwise it first branches to the body block.
    bool HasCondBlock() const;

    /// @param block the new switch continuing block
    void SetCondBlock(core::ir::MultiInBlock* block);

    /// @returns the friendly name for the instruction
    std::string FriendlyName() const override { return "for-loop"; }

    void SetConditionValue(core::ir::Value* cond_val) { condition_value_ = cond_val; }

    core::ir::Value* ConditionValue() const { return condition_value_; }

  private:
    ConstPropagatingPtr<tint::core::ir::MultiInBlock> condblock_;
    core::ir::Value* condition_value_ = nullptr;
};

}  // namespace tint::hlsl::ir

#endif  // SRC_TINT_LANG_CORE_IR_FOR_LOOP_H_
