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

#include "src/tint/lang/core/ir/transform/remove_terminator_args.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
// For core::type::Pointer type checks
#include "src/tint/lang/core/type/pointer.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    Module& ir;

    /// The IR builder.
    Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// A list of terminators that need to have their arguments cleared.
    Vector<Terminator*, 8> terminators_to_clear{};

    /// Process the module.
    void Process() {
        // Strategy: Transform If / Switch first across the whole module, then Loops.
        // This ensures that nested If results are already materialized to function variables
        // before we rewrite Loop exits to stores. That avoids use-before-defs on Loop exit values
        // which may otherwise reference If results that are only loaded after the If terminator.

        // Pass 1: If and Switch
        for (auto* inst : ir.Instructions()) {
            if (auto* i = inst->As<If>()) {
                    RemoveExitArgs(i);
            } else if (auto* s = inst->As<Switch>()) {
                RemoveExitArgs(s);
            }

            // Clear queued terminator operands for this pass (except BreakIf)
            for (auto* terminator : terminators_to_clear) {
                if (!terminator->Is<BreakIf>()) {
                    terminator->ClearOperands();
                }
            }
            terminators_to_clear.Clear();
        }

        // Pass 2: Loops
        for (auto* inst : ir.Instructions()) {
            if (auto* l = inst->As<Loop>()) {
                // If the loop has pointer-typed results or body/continuing params, skip fully.
                bool skip_loop = HasPointerResult(l) || HasPointerParam(l->Body()) ||
                                 HasPointerParam(l->Continuing());
                if (!skip_loop) {
                    RemoveExitArgs(l);
                    RemoveBlockParams(l->Body(), l->Initializer()->Front());
                    RemoveBlockParams(l->Continuing(), l->Body()->Front());
                }
            }

            // Clear queued terminator operands for this pass (except BreakIf)
            for (auto* terminator : terminators_to_clear) {
                if (!terminator->Is<BreakIf>()) {
                    terminator->ClearOperands();
                }
            }
            terminators_to_clear.Clear();
        }

        // Post-pass: Ensure BreakIf next-iteration arity matches loop body params for pointer-
        // sensitive loops where we performed no transformation. This guards against any prior
        // clears and keeps the IR validator happy.
        for (auto* inst : ir.Instructions()) {
            if (auto* l = inst->As<Loop>()) {
                auto needed = l->Body()->Params().Length();
                if (needed == 0u) {
                    continue;
                }
                // Capture default next-iteration values from the initializer, if present.
                Vector<Value*, 8> init_vals;
                if (auto* ni = l->Initializer()->Terminator()->As<NextIteration>()) {
                    for (auto* v : ni->Args()) {
                        init_vals.Push(v);
                    }
                }
                for (auto* in : l->Continuing()->InboundSiblingBranches()) {
                    if (auto* bi = in->As<BreakIf>()) {
                        auto have = bi->NextIterValues().Length();
                        if (have < needed) {
                            // Append missing values from initializer defaults when available,
                            // otherwise append null placeholders which will trigger validation
                            // diagnostics in pre-existing invalid modules.
                            for (size_t i = have; i < needed; i++) {
                                Value* v = (i < init_vals.Length()) ? init_vals[i] : nullptr;
                                bi->PushOperand(v);
                            }
                            bi->SetNumNextIterValues(needed);
                        }
                    }
                }
            }
        }
    }

    /// @returns true if any result of the control instruction is a pointer type
    bool HasPointerResult(ControlInstruction* ci) {
        for (size_t i = 0; i < ci->Results().Length(); i++) {
            if (ci->Result(i)->Type()->Is<core::type::Pointer>()) {
                return true;
            }
        }
        return false;
    }

    /// Remove the arguments from all exit instructions inside a control instruction.
    /// Pointer-typed results are not supported by this transform, as materializing
    /// them via a function-scope variable would create nested pointer types which
    /// are invalid. In that case, this function leaves the control instruction
    /// and its terminator arguments untouched.
    /// @param ci the control instruction
    void RemoveExitArgs(ControlInstruction* ci) {
        if (HasPointerResult(ci)) {
            // Skip transforming this control instruction to avoid creating
            // nested pointer types (ptr<function, ptr<...>>), which are
            // rejected by the IR validator and WGSL.
            return;
        }
        if (ci->Results().Length() == 0u) {
            return;
        }
        // First pass: create a function-scope variable for each result.
        Vector<Var*, 8> result_vars;
        result_vars.Resize(ci->Results().Length());
        for (size_t i = 0; i < ci->Results().Length(); i++) {
            auto* result = ci->Result(i);
            auto* var = b.Var(ty.ptr<function>(result->Type()));
            var->InsertBefore(ci);
            result_vars[i] = var;
        }

        // Second pass: store exit operands into the corresponding variables, carefully handling
        // dependencies where an exit operand references another result of the same control.
        for (size_t i = 0; i < ci->Results().Length(); i++) {
            auto* var = result_vars[i];
            for (auto exit : ci->Exits()) {
                Value* value = nullptr;
                if (auto* breakif = exit.Value()->As<BreakIf>()) {
                    value = breakif->ExitValues()[i];
                } else {
                    value = exit.Value()->Args()[i];
                }
                if (!value) {
                    continue;
                }

                // If the exit operand is one of the control's results, then materialize that
                // operand from the corresponding function variable with a load before the exit.
                for (size_t k = 0; k < ci->Results().Length(); k++) {
                    if (value == ci->Result(k)) {
                        auto* dep_load = b.Load(result_vars[k]);
                        dep_load->InsertBefore(exit.Value());
                        value = dep_load->Result();
                        break;
                    }
                }

                // If the exit operand is a load instruction result that is defined after this
                // branch point (for example, a load inserted by transforming a nested control
                // result), then re-materialize the load at this location to preserve
                // dominance/ordering for consumers like the WGSL writer.
                if (auto* res = value->As<InstructionResult>()) {
                    if (auto* load_inst = res->Instruction()->As<Load>()) {
                        auto* local_load = b.Load(load_inst->From());
                        local_load->InsertBefore(exit.Value());
                        value = local_load->Result();
                    }
                }

                    auto* store = b.Store(var, value);
                    store->InsertBefore(exit.Value());
                }
            }

        // Third pass: replace the original results with loads from their variables.
        for (size_t i = 0; i < ci->Results().Length(); i++) {
            auto* result = ci->Result(i);
            auto* var = result_vars[i];
            auto* load = b.LoadWithResult(result, var);
            load->InsertAfter(ci);
        }

        // Remove the arguments from the exits and the results from the control instruction.
        for (auto exit : ci->Exits()) {
            terminators_to_clear.Push(exit);
        }
        ci->ClearResults();
    }

    /// @returns true if any parameter of the block is a pointer type
    bool HasPointerParam(MultiInBlock* block) {
        for (size_t i = 0; i < block->Params().Length(); i++) {
            if (block->Params()[i]->Type()->Is<core::type::Pointer>()) {
                return true;
            }
        }
        return false;
    }

    /// Remove block parameters and arguments from all branches to a block.
    /// Pointer-typed parameters are not supported by this transform for the
    /// same reason as control results (would create nested pointers). If any
    /// parameter is a pointer, the block is left unchanged.
    /// @param block the block
    /// @param var_insertion_point the insertion point for variables used to replace parameters
    void RemoveBlockParams(MultiInBlock* block, Instruction* var_insertion_point) {
        // Nothing to do if the block has no parameters. Avoid clearing branch arguments.
        if (block->Params().Length() == 0u) {
            return;
        }
        if (HasPointerParam(block)) {
            return;
        }
        for (size_t i = 0; i < block->Params().Length(); i++) {
            auto* param = block->Params()[i];

            // Create a variable to hold the parameter value, and insert it in the parent block.
            auto* var = b.Var(ty.ptr<function>(param->Type()));
            var->InsertBefore(var_insertion_point);

            // Store to the variable before each branch.
            for (auto* branch : block->InboundSiblingBranches()) {
                Value* value = nullptr;
                if (auto* breakif = branch->As<BreakIf>()) {
                    value = breakif->NextIterValues()[i];
                } else {
                    value = branch->Args()[i];
                }
                if (value) {
                    // If the source value is a Load produced outside of this branch point (e.g.,
                    // by a previous transform that replaced control results), re-materialize the
                    // load at this location to keep SSA/ordering valid.
                    if (auto* res = value->As<InstructionResult>()) {
                        if (auto* load_inst = res->Instruction()->As<Load>()) {
                            auto* local_load = b.Load(load_inst->From());
                            local_load->InsertBefore(branch);
                            value = local_load->Result();
                        }
                    }

                    auto* store = b.Store(var, value);
                    store->InsertBefore(branch);
                }
            }

            // Replace the original result with a load from the variable that we created above.
            auto* load = b.Load(var);
            load->InsertBefore(block->Front());
            param->ReplaceAllUsesWith(load->Result());
        }

        // Remove the arguments from the branches and the parameters from the block.
        for (auto exit : block->InboundSiblingBranches()) {
            terminators_to_clear.Push(exit);
        }
        block->SetParams({});
    }
};

}  // namespace

Result<SuccessType> RemoveTerminatorArgs(Module& ir) {
    auto result =
        ValidateAndDumpIfNeeded(ir, "core.RemoveTerminatorArgs", kRemoveTerminatorArgsCapabilities);
    if (result != Success) {
        return result;
    }

    // Pre-scan: if the module only contains loops whose body/continuing have pointer-typed
    // parameters or pointer-typed results, then there is nothing safe to rewrite. Exit early
    // to avoid accidentally modifying loop-carried next-iteration values.
    bool only_pointer_sensitive_loops = true;
    for (auto* inst : ir.Instructions()) {
        if (auto* i = inst->As<If>()) {
            only_pointer_sensitive_loops = false;
            break;
        }
        if (auto* s = inst->As<Switch>()) {
            only_pointer_sensitive_loops = false;
            break;
        }
        if (auto* l = inst->As<Loop>()) {
            bool loop_sensitive = false;
            // Pointer results or pointer params on body/continuing make this loop sensitive.
            for (size_t r = 0; r < l->Results().Length(); r++) {
                if (l->Result(r)->Type()->Is<core::type::Pointer>()) {
                    loop_sensitive = true;
                    break;
                }
            }
            if (!loop_sensitive) {
                for (size_t p = 0; p < l->Body()->Params().Length(); p++) {
                    if (l->Body()->Params()[p]->Type()->Is<core::type::Pointer>()) {
                        loop_sensitive = true;
                        break;
                    }
                }
            }
            if (!loop_sensitive) {
                for (size_t p = 0; p < l->Continuing()->Params().Length(); p++) {
                    if (l->Continuing()->Params()[p]->Type()->Is<core::type::Pointer>()) {
                        loop_sensitive = true;
                        break;
                    }
                }
            }
            if (!loop_sensitive) {
                only_pointer_sensitive_loops = false;
                break;
            }
        }
    }
    if (only_pointer_sensitive_loops) {
        return Success;
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
