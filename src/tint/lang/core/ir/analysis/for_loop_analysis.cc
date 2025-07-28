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
#include "src/tint/lang/core/ir/builtin_call.h"
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
                           std::map<const core::ir::Instruction*, SinkType>& memoize_cache) {
    if (inst == sink) {
        return SinkType::Full;
    }
    if (memoize_cache.find(sink) != memoize_cache.end()) {
        return memoize_cache[inst];
    }

    auto assumed_type = SinkType::None;
    bool has_assigned_first = false;
    for (auto& each_result : inst->Results()) {
        for (auto& each_usage : each_result->UsagesUnsorted()) {
            if (!each_usage->instruction) {
                continue;
            }
            // TODO(petermcneeley): Can we make this not recursive. Fuzzers may find ways of
            // producing stack overflows at some stage.
            auto usage_is_sink = UsageSinkAnalysis(each_usage->instruction, sink, memoize_cache);
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

    memoize_cache[inst] = assumed_type;
    return assumed_type;
}
}  // namespace

void ForLoopAnalysis::AttemptForLoopDeduction(const Loop* loop) {
    // Find the first 'if' which should be the condition for the for-loop.
    // Order here is critical since instructions in blocks are forward dependencies.
    const core::ir::If* if_to_remove = nullptr;
    for (auto* inst : *loop->Body()) {
        if (auto* if_ = inst->As<const core::ir::If>()) {
            if (if_->Results().IsEmpty() && if_->True()->Length() == 1 &&
                if_->False()->Length() == 1 && tint::Is<core::ir::ExitIf>(if_->True()->Front()) &&
                tint::Is<core::ir::ExitLoop>(if_->False()->Front())) {
                // Matched the loop condition as it was converted from the 'for' originally.
                if_to_remove = if_;
                break;
            } else {
                // Conservatively fail to avoid the possibility of reordering instructions (when
                // moving into conditional).
                return;
            }
        } else if (inst->Is<const Binary>() || inst->Is<const BuiltinCall>() ||
                   inst->Is<const Access>() || inst->Is<const Load>() ||
                   inst->Is<const Swizzle>() || inst->Is<const Unary>() ||
                   inst->Is<const LoadVectorElement>()) {
            // Allowed instructions since either load or operate on values (side effect free).
            continue;
        } else {
            // Conservatively fail for all other functions that could potentially store/mutate
            // memory.
            return;
        }
    }
    if (!if_to_remove) {
        return;
    }

    auto* local_for_condition = if_to_remove->Condition()->As<core::ir::InstructionResult>();
    if (!local_for_condition) {
        return;
    }

    if (local_for_condition->NumUsages() != 1u) {
        // The condition result should only be used by the 'if' instruction.
        return;
    }

    auto* for_condition_inst = local_for_condition->Instruction();
    if (!for_condition_inst) {
        return;
    }

    std::map<const Instruction*, SinkType> mem_cache;
    for (auto* inst : *loop->Body()) {
        if (for_condition_inst == inst) {
            body_removed_instructions.insert(inst);
            break;
        }

        auto sink_type = UsageSinkAnalysis(inst, for_condition_inst, mem_cache);
        if (sink_type != SinkType::Full) {
            // We only allow the situation where the instructions are used completely by the
            // condition. This simplifies hoisting and eliminates ordering concerns.
            return;
        }

        body_removed_instructions.insert(inst);
    }
    // Add the 'if' instruction to set to avoid adding it to the body.
    body_removed_instructions.insert(if_to_remove);
    // Valid condition found. Success criteria for condition hoisting.
    for_condition = local_for_condition;
}

ForLoopAnalysis::ForLoopAnalysis(const Loop& loop) {
    AttemptForLoopDeduction(&loop);
}
ForLoopAnalysis::~ForLoopAnalysis() = default;

}  // namespace tint::core::ir::analysis
