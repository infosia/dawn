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

#include "src/tint/lang/hlsl/writer/raise/reconstruct_for_loops.h"
#include <map>
#include <set>

#include "src/tint/lang/core/ir/block.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/call.h"
#include "src/tint/lang/core/ir/continue.h"
#include "src/tint/lang/core/ir/exit.h"
#include "src/tint/lang/core/ir/exit_if.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/instruction_result.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load_vector_element.h"
#include "src/tint/lang/core/ir/loop.h"

#include "src/tint/lang/core/ir/clone_context.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/lang/core/ir/next_iteration.h"
#include "src/tint/lang/core/ir/switch.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/hlsl/ir/for_loop.h"
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
        Vector<core::ir::Loop*, 4> loop_worklist;
        for (auto* inst : ir.Instructions()) {
            // for loop reconstruction attempt
            if (auto* loop = inst->As<core::ir::Loop>()) {
                loop_worklist.Push(loop);
                // loop over body statements. Take the first one
            }
        }

        // Do the for loop mitigation
        for (auto* loop : loop_worklist) {
            AttemptForLoopDeduction(loop);
        }
    }
    enum class SinkType {
        Full,
        Partial,
        None,
        Unknown,
    };

    // Determines if 'sink' is a usage sink for 'inst'.
    // A 'Full' sink means this instruction
    SinkType UsageAnalysis(core::ir::Instruction* inst,
                           core::ir::Instruction* sink,
                           std::map<core::ir::Instruction*, SinkType>& memorize_cache) {
        if (inst == sink) {
            return SinkType::Full;
        }
        if (memorize_cache.find(sink) != memorize_cache.end()) {
            return memorize_cache[inst];
        }

        if (!inst->Result(0)) {
            return SinkType::Unknown;  // no result;
        }

        auto assumed_type = SinkType::Unknown;
        for (auto& each_usage : inst->Result()->UsagesSorted()) {
            if (!each_usage.instruction) {
                continue;
            }
            auto usage_is_sink = UsageAnalysis(each_usage.instruction, sink, memorize_cache);
            if (assumed_type == SinkType::Unknown) {
                assumed_type = usage_is_sink;
            } else if (assumed_type == SinkType::Full && usage_is_sink != SinkType::Full) {
                assumed_type = SinkType::Partial;
            } else if (assumed_type == SinkType::None && usage_is_sink != SinkType::None) {
                assumed_type = SinkType::Partial;
            }
        }

        if (assumed_type == SinkType::Unknown) {
            assumed_type = SinkType::None;
        }

        memorize_cache[inst] = assumed_type;
        return assumed_type;
    }

    void AttemptForLoopDeduction(core::ir::Loop* loop) {
        // Find the first if which should be the condition for the for-loop.
        core::ir::Instruction* for_condition = nullptr;
        core::ir::If* if_replace = nullptr;
        for (auto* inst : *loop->Body()) {
            if (auto* if_ = inst->As<core::ir::If>()) {
                if (if_->Results().IsEmpty() && if_->True()->Length() == 1 &&
                    if_->False()->Length() == 1 &&
                    tint::Is<core::ir::ExitIf>(if_->True()->Front()) &&
                    tint::Is<core::ir::ExitLoop>(if_->False()->Front())) {
                    // Matched the loop condition as it was converted from the 'for' originally.
                    if (auto* inst_res = if_->Condition()->As<core::ir::InstructionResult>()) {
                        // TODO check for single use of this condition?
                        for_condition = inst_res->Instruction();
                        if_replace = if_;
                    }
                    break;
                }
            } else if (inst->Is<core::ir::Binary>() || inst->Is<core::ir::Call>() ||
                       inst->Is<core::ir::Access>() || inst->Is<core::ir::Load>() ||
                       inst->Is<core::ir::Swizzle>() || inst->Is<core::ir::Unary>() ||
                       inst->Is<core::ir::LoadVectorElement>()) {
                // Allowed instructions since either load or operate on values.
                continue;
            } else {
                // Conservatively fail for all other functions that could potentially store/mutate
                // memory.
                return;
            }
        }
        if (!for_condition) {
            printf(" \n failed ot find if !! \n");
            return;
        }

        std::map<core::ir::Instruction*, SinkType> mem_cache;
        std::set<core::ir::Instruction*> cond_instructions;
        for (auto* inst : *loop->Body()) {
            if (for_condition == inst) {
                cond_instructions.insert(inst);
                break;
            }

            auto sink_type = UsageAnalysis(inst, for_condition, mem_cache);
            if (sink_type == SinkType::Partial) {
                printf("sink is partial");
                return;
            }
            if (sink_type == SinkType::Full) {
                cond_instructions.insert(inst);
            }
        }

        auto* for_loop = b.Append(ir.CreateInstruction<ir::ForLoop>(
            b.Block(), b.MultiInBlock(), b.MultiInBlock(), b.MultiInBlock()));

        auto* new_init = b.Block();
        auto* new_continuing = b.MultiInBlock();

        auto* new_body = b.MultiInBlock();
        auto* condition_block = b.MultiInBlock();

        auto ctx = tint::core::ir::CloneContext{ir};

        auto replace_loop_reference = [&](core::ir::Instruction* inst, ir::ForLoop* for_loop) {
            if (auto* next_iter = inst->As<core::ir::NextIteration>()) {
                next_iter->SetLoop(for_loop);
            }
            if (auto* cont = inst->As<core::ir::Continue>()) {
                cont->SetLoop(for_loop);
            }

            if (auto* exit_loop = inst->As<core::ir::ExitLoop>()) {
                exit_loop->SetLoop(for_loop);
            }

            if (auto* if_inst = inst->As<core::ir::If>()) {
                for (auto each : *if_inst->True()) {
                    if (auto* exit_loop = each->As<core::ir::ExitLoop>()) {
                        exit_loop->SetLoop(for_loop);
                    }
                }

                for (auto each : *if_inst->False()) {
                    if (auto* exit_loop = each->As<core::ir::ExitLoop>()) {
                        exit_loop->SetLoop(for_loop);
                    }
                }
            }
        };

        for (auto inst = *loop->Continuing()->begin(); inst;) {
            // Remember next instruction as we're about to remove the current one from its block
            auto* next = inst->next.Get();
            TINT_DEFER(inst = next);
            inst->Remove();
            replace_loop_reference(inst, for_loop);
            new_continuing->Append(inst);
        }

        for (auto inst = *loop->Initializer()->begin(); inst;) {
            // Remember next instruction as we're about to remove the current one from its block
            auto* next = inst->next.Get();
            TINT_DEFER(inst = next);
            inst->Remove();
            replace_loop_reference(inst, for_loop);
            new_init->Append(inst);
        }

// body is added manually
#if 1
        for (auto inst = *loop->Body()->begin(); inst;) {
            // Remember next instruction as we're about to remove the current one from its block
            auto* next = inst->next.Get();
            TINT_DEFER(inst = next);
            inst->Remove();

            replace_loop_reference(inst, for_loop);

            if (if_replace == inst->As<core::ir::If>()) {
                for_loop->SetConditionValue(if_replace->Condition());
                if_replace->Destroy();
                continue;
            }
            if (cond_instructions.contains(inst)) {
                printf(" Appending to condition \n");
                condition_block->Append(inst);
            } else {
                printf(" Appending to new body \n");
                new_body->Append(inst);
            }
        }
#endif

        for_loop->SetInitializer(new_init);
        for_loop->SetContinuing(new_continuing);
        for_loop->SetBody(new_body);
        for_loop->SetCondBlock(condition_block);
        // loop->
        for_loop->InsertBefore(loop);

        loop->Destroy();
        printf(" \n matched !! \n");
    }
};

}  // namespace

Result<SuccessType> ReconstuctForLoops(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(
        ir, "hlsl.ReconstuctForLoops",
        core::ir::Capabilities{core::ir::Capability::kAllowDuplicateBindings});
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    auto result2 = ValidateAndDumpIfNeeded(
        ir, "AFTER.hlsl.ReconstuctForLoops2",
        core::ir::Capabilities{core::ir::Capability::kAllowDuplicateBindings});

    auto result4 = ValidateAndDumpIfNeeded(
        ir, "AFTER.hlsl.ReconstuctForLoops3",
        core::ir::Capabilities{core::ir::Capability::kAllowDuplicateBindings});

    return Success;
}

}  // namespace tint::hlsl::writer::raise
