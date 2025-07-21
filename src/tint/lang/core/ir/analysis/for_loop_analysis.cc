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

#include <map>

#include "src/tint/lang/core/ir/access.h"
#include "src/tint/lang/core/ir/binary.h"
#include "src/tint/lang/core/ir/bitcast.h"
#include "src/tint/lang/core/ir/exit_if.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/load_vector_element.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/lang/core/ir/store.h"
#include "src/tint/lang/core/ir/swizzle.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/unary.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/utils/rtti/switch.h"

namespace tint::core::ir::analysis {

namespace {
enum class SinkType {
    Full,     // All usages fall into this sink instruction
    Partial,  // Some but not all usages are used by the sink instruction
    None,     // Instruction not used by the sink instruction
    Error,    // Analysis could not be completed
};

// Determines if 'sink' is a usage sink for 'inst'. A 'Full' sink means this 'inst' is transitively
// exclusively used by 'sink'.
SinkType UsageSinkAnalysis(const core::ir::Instruction* inst,
                           const core::ir::Instruction* sink,
                           std::map<const core::ir::Instruction*, SinkType>& memorize_cache) {
    if (inst == sink) {
        return SinkType::Full;
    }
    if (memorize_cache.find(sink) != memorize_cache.end()) {
        return memorize_cache[inst];
    }

    auto assumed_type = SinkType::None;
    bool has_assigned_first = false;
    for (auto& each_result : inst->Results()) {
        for (auto& each_usage : each_result->UsagesSorted()) {
            if (!each_usage.instruction) {
                continue;
            }
            auto usage_is_sink = UsageSinkAnalysis(each_usage.instruction, sink, memorize_cache);
            if (!has_assigned_first) {
                assumed_type = usage_is_sink;
                has_assigned_first = true;
            } else if (assumed_type == SinkType::Full && usage_is_sink != SinkType::Full) {
                assumed_type = SinkType::Partial;
            } else if (assumed_type == SinkType::None && usage_is_sink != SinkType::None) {
                assumed_type = SinkType::Partial;
            }
        }
    }

    memorize_cache[inst] = assumed_type;
    return assumed_type;
}
}  // namespace

void ForLoopAnalysis::AttemptForLoopDeduction(const Loop* loop) {
    // Find the first 'if' which should be the condition for the for-loop.
    // Order here is critical since instructions in blocks are forward dependencies.
    for (auto* inst : *loop->Body()) {
        if (auto* if_ = inst->As<const core::ir::If>()) {
            if (if_->Results().IsEmpty() && if_->True()->Length() == 1 &&
                if_->False()->Length() == 1 && tint::Is<core::ir::ExitIf>(if_->True()->Front()) &&
                tint::Is<core::ir::ExitLoop>(if_->False()->Front())) {
                // Matched the loop condition as it was converted from the 'for' originally.
                if_replace = if_;
                break;
            }
        } else if (inst->Is<const Binary>() || inst->Is<const Call>() || inst->Is<const Access>() ||
                   inst->Is<const Load>() || inst->Is<const Swizzle>() || inst->Is<const Unary>() ||
                   inst->Is<const LoadVectorElement>()) {
            // Allowed instructions since either load or operate on values.
            continue;
        } else {
            // Conservatively fail for all other functions that could potentially store/mutate
            // memory.
            return;
        }
    }
    if (!if_replace) {
        printf(" \n failed ot find if !! \n");
        return;
    }

    for_condition = if_replace->Condition()->As<core::ir::InstructionResult>();
    if (!for_condition) {
        return;
    }

    auto* for_condition_inst = for_condition->Instruction();
    if (!for_condition_inst) {
        return;
    }

    std::map<const Instruction*, SinkType> mem_cache;
    for (auto* inst : *loop->Body()) {
        if (for_condition_inst == inst) {
            cond_instructions.insert(inst);
            break;
        }

        auto sink_type = UsageSinkAnalysis(inst, for_condition_inst, mem_cache);
        if (sink_type == SinkType::Partial) {
            printf("sink is partial");
            return;
        }
        if (sink_type == SinkType::Full) {
            cond_instructions.insert(inst);
        }
    }
    // Add the if instruction to set to avoid adding it to the body.
    cond_instructions.insert(if_replace);
}

ForLoopAnalysis::ForLoopAnalysis(const Loop& loop) {
    AttemptForLoopDeduction(&loop);
}
ForLoopAnalysis::~ForLoopAnalysis() = default;

}  // namespace tint::core::ir::analysis
