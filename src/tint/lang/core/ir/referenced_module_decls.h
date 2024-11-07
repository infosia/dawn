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

#ifndef SRC_TINT_LANG_CORE_IR_REFERENCED_MODULE_DECLS_H_
#define SRC_TINT_LANG_CORE_IR_REFERENCED_MODULE_DECLS_H_

#include <functional>

#include "src/tint/lang/core/ir/control_instruction.h"
#include "src/tint/lang/core/ir/instruction.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/override.h"
#include "src/tint/lang/core/ir/user_call.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/utils/containers/hashmap.h"
#include "src/tint/utils/containers/unique_vector.h"
#include "src/tint/utils/rtti/switch.h"

// Forward declarations.
namespace tint::core::ir {
class Block;
class Function;
}  // namespace tint::core::ir

/// Utility that helps guarantee the same const-ness is applied to both types.
template <class Src, class Dst>
using TranscribeConst = std::conditional_t<std::is_const<Src>{}, std::add_const_t<Dst>, Dst>;

namespace tint::core::ir {

/// ReferencedModuleDecls is a helper to determine the set of module-scope declarations that are
/// transitively referenced by functions in a module.
///
/// References are determined lazily and cached for future requests.
///
/// Note:
///      The template param M is used to ensure that inputs and outputs of this class have the same
///      const-ness. If 'Module' is supplied then the internal operations and output will not be
///      const, which is needed for transforms. Whereas if the param is 'const Module' the internals
///      and outputs will be const, which is needed for the IR validator.
/// Note:
///      Changes to the module can invalidate the cached data. This is intended to be created by
///      operations that need this information, and discarded when they complete. Tracking this
///      information inside the IR module would add overhead any time an instruction is added or
///      removed from the module. Since only a few operations need this information, it is expected
///      to be more efficient to generate it on demand.
template <typename M>
class ReferencedModuleDecls {
    // Replace this with concepts when C++20 is available
    static_assert(std::is_same<std::remove_cv_t<M>, Module>());

  public:
    /// Short form aliases for types that have the same constant-ness as M.
    /// (The single use types are not aliased)
    using BlockT = TranscribeConst<M, Block>;
    using DeclT = TranscribeConst<M, Instruction>;
    using FunctionT = TranscribeConst<M, Function>;

    /// A set of a declarations referenced by a function (in declaration order).
    using DeclSet = UniqueVector<DeclT*, 16>;

    /// Constructor.
    /// @param ir the module
    /// @param pred an predicate function for filtering declarations
    /// Note: @p pred is not stored by the class, so can be a lambda that captures by reference.
    template <typename Predicate>
    ReferencedModuleDecls(M& ir, Predicate&& pred) {
        // Loop over module-scope declarations, recording the blocks that they are referenced from.
        BlockT* root_block = ir.root_block;
        for (auto* inst : *root_block) {
            if (!inst || !pred(inst) || !inst->Result(0)) {
                continue;
            }

            inst->Result(0)->ForEachUseUnsorted([&](const Usage& use) {
                block_to_direct_decls_.GetOrAddZero(use.instruction->Block()).Add(inst);
            });

            // For each var<workgroup> with array of ValueArrayCount, walk up chain and add to all
            // blocks that reference the var
        }
    }

    /// Constructor.
    /// Provided default predicate that accepts all declarations.
    explicit ReferencedModuleDecls(M& ir)
        : ReferencedModuleDecls(ir, [](DeclT*) { return true; }) {}

    /// Get the set of transitively referenced module-scope declarations for a function, filtered by
    /// the predicate function if provided.
    /// @param func the function
    /// @returns the set of (possibly filtered) transitively reference module-scope declarations
    DeclSet& TransitiveReferences(FunctionT* func) {
        return transitive_references_.GetOrAdd(func, [&] {
            DeclSet decls;
            GetTransitiveReferences(func ? func->Block() : nullptr, decls);

            // For a compute entry point, we need to check if any of the workgroup sizes are built
            // on overrides.
            if (func->Stage() == core::ir::Function::PipelineStage::kCompute) {
                TINT_ASSERT(func->WorkgroupSize()->has_value());

                for (auto size : *func->WorkgroupSize()) {
                    if (size->template Is<core::ir::Constant>()) {
                        continue;
                    }

                    // Size is based on instructions, walk up the chain adding those instructions to
                    // the `decls` list.
                    auto* inst = size->template As<core::ir::InstructionResult>();
                    TINT_ASSERT(inst);
                }
            }
            return decls;
        });
    }

  private:
    /// A map from blocks to their directly referenced declarations.
    Hashmap<BlockT*, DeclT, 64> block_to_direct_decls_{};

    /// A map from functions to their transitively referenced declarations.
    Hashmap<FunctionT*, DeclSet, 8> transitive_references_;

    /// Get the set of transitively referenced module-scope declarations for a block.
    /// @param block the block
    /// @param decls the set of transitively reference module-scope declarations to populate
    void GetTransitiveReferences(BlockT* block, DeclSet& decls) {
        if (!block) {
            return;
        }

        // Add directly referenced declarations.
        if (auto itr = block_to_direct_decls_.Get(block)) {
            for (auto& decl : *itr) {
                decls.Add(decl);
            }
        }

        // Loop over instructions in the block to find indirectly referenced vars.
        for (auto* inst : *block) {
            tint::Switch(
                inst,
                [&](TranscribeConst<M, UserCall>* call) {
                    // Get declarations referenced by a function called from this block.
                    const auto& callee_decls = TransitiveReferences(call->Target());
                    for (auto* decl : callee_decls) {
                        decls.Add(decl);
                    }
                },
                [&](TranscribeConst<M, ControlInstruction>* ctrl) {
                    // Recurse into control instructions and gather their referenced declarations.
                    ctrl->ForeachBlock([&](BlockT* blk) { GetTransitiveReferences(blk, decls); });
                });
        }
    }
};

}  // namespace tint::core::ir

#endif  // SRC_TINT_LANG_CORE_IR_REFERENCED_MODULE_DECLS_H_
