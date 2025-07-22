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

#ifndef SRC_TINT_LANG_CORE_IR_ANALYSIS_LOOP_ANALYSIS_H_
#define SRC_TINT_LANG_CORE_IR_ANALYSIS_LOOP_ANALYSIS_H_

#include <memory>
#include <set>

#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/var.h"

namespace tint::core::ir::analysis {

/// LoopAnalysis is a helper used to analyze loops an find the loop condition
class ForLoopAnalysis {
  public:
    explicit ForLoopAnalysis(const Loop& loop);
    ~ForLoopAnalysis();

    void AttemptForLoopDeduction(const Loop* loop);
    const core::ir::InstructionResult* GetIfCondition() { return for_condition; }
    bool IsConditionInstruction(const core::ir::Instruction* inst) {
        return cond_instructions.find(inst) != cond_instructions.end();
    }

  private:
    std::set<const core::ir::Instruction*> cond_instructions;
    const core::ir::InstructionResult* for_condition = nullptr;
};

}  // namespace tint::core::ir::analysis

#endif  // SRC_TINT_LANG_CORE_IR_ANALYSIS_LOOP_ANALYSIS_H_
