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

#include "src/tint/lang/core/ir/transform/single_entry_point.h"

#include <utility>

#include "src/tint/lang/core/ir/block.h"
#include "src/tint/lang/core/ir/constexpr_if.h"
#include "src/tint/lang/core/ir/instruction_result.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/referenced_functions.h"
#include "src/tint/lang/core/ir/referenced_module_decls.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::core::ir::transform {

namespace {

Result<SuccessType> Run(ir::Module& ir, std::string_view entry_point_name) {
    // Find the entry point.
    ir::Function* entry_point = nullptr;
    for (auto& func : ir.functions) {
        if (!func->IsEntryPoint()) {
            continue;
        }
        if (ir.NameOf(func).NameView() == entry_point_name) {
            if (entry_point) {
                TINT_ICE() << "multiple entry points named '" << entry_point_name << "' were found";
            }
            entry_point = func;
        }
    }
    if (!entry_point) {
        TINT_ICE() << "entry point '" << entry_point_name << "' not found";
    }

    // Remove unused functions.
    ReferencedFunctions<Module> referenced_function_cache(ir);
    auto& referenced_functions = referenced_function_cache.TransitiveReferences(entry_point);
    for (uint32_t i = 0; i < ir.functions.Length();) {
        auto func = ir.functions[i];
        if (func == entry_point || referenced_functions.Contains(func)) {
            i++;
            continue;
        }

        func->Destroy();
        ir.functions.Erase(i);
    }

    // Remove unused module-scope variables.
    ReferencedModuleDecls<Module> referenced_var_cache(ir);
    auto& referenced_vars = referenced_var_cache.TransitiveReferences(entry_point);

    Vector<Value*, 16> values_to_propagate;
    for (auto inst : referenced_vars) {
        // These operands are assumed to simply be the initalizers for the vars.
        for (auto* op : inst->Operands()) {
            values_to_propagate.Push(op);
        }
    }

    // Transitively add all instruction references for all operands
    while (!values_to_propagate.IsEmpty()) {
        auto* value = values_to_propagate.Pop();
        if (auto* inst_res = value->As<InstructionResult>()) {
            if (auto* inst = inst_res->Instruction()) {
                referenced_vars.Add(inst);
                for (auto* op : inst->Operands()) {
                    values_to_propagate.Push(op);
                }
                // ConstExprIf instruction is special because it use to be a short circuiting binary
                // operation that was converted to a specialized if instruction.
                if (auto* const_expr_if = inst->As<ConstExprIf>()) {
                    auto add_block_instructions = [&values_to_propagate](Block* block) {
                        if (block) {
                            for (auto* block_inst : *block) {
                                for (auto* each_res : block_inst->Results()) {
                                    values_to_propagate.Push(each_res);
                                }
                            }
                        }
                    };

                    add_block_instructions(const_expr_if->True());
                    add_block_instructions(const_expr_if->False());
                }
            }
        }
    }

    auto* inst = ir.root_block->Back();

    // The instructions are removed in reverse order. This is in order to handle overrides which
    // have an initializer. If the initializer is made of multiple instructions we need to delete
    // the later one first to remove the usage from the earlier instruction.
    while (inst) {
        auto prev = inst->prev;
        if (!referenced_vars.Contains(inst)) {
            // There shouldn't be any remaining references to the variable.
            if (!(inst->Result(0)->NumUsages() == 0)) {
                printf(" inst %s \n", inst->FriendlyName().c_str());

                TINT_ICE() << " bad things";
            }
            inst->Destroy();
        }
        inst = prev;
    }

    return Success;
}

}  // namespace

Result<SuccessType> SingleEntryPoint(Module& ir, std::string_view entry_point_name) {
    auto result = ValidateAndDumpIfNeeded(
        ir, "core.SingleEntryPoint", core::ir::Capabilities{core::ir::Capability::kAllowOverrides});
    if (result != Success) {
        return result.Failure();
    }

    return Run(ir, entry_point_name);
}

}  // namespace tint::core::ir::transform
