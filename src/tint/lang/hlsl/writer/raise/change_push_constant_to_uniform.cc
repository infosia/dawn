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

#include "src/tint/lang/hlsl/writer/raise/change_push_constant_to_uniform.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"

namespace tint::hlsl::writer::raise {
namespace {

/// PIMPL state for the transform.
///
/// Each uniform buffer variable is rewritten so its store type is an array of vec4u
/// elements.  Accesses to original store type contents are rewritten to pick out the
/// right set of vec4u elements and then use bitcasts as needed to construct the originally
/// loaded value.
struct State {
    /// The transform config.
    const ChangePushConstantToUniformConfig& config;
    /// The IR module.
    core::ir::Module& ir;
    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// Process the module.
    void Process() {
        core::ir::Var* push_constants = nullptr;
        for (auto* inst : *ir.root_block) {
            // Allow this to run before or after PromoteInitializers by handling non-var root_block
            // entries
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }

            // DecomposeStorageAccess maybe have converte the var pointers into ByteAddressBuffer
            // objects. Since they've been changed, then they're Storage buffers and we don't care
            // about them here.
            auto* var_ty = var->Result()->Type()->As<core::type::Pointer>();
            if (!var_ty) {
                continue;
            }

            // Only care about uniform address space variables.
            if (var_ty->AddressSpace() != core::AddressSpace::kPushConstant) {
                continue;
            }

            if (push_constants) {
                TINT_ICE() << "multiple push constant variables";
            }

            push_constants = var;
        }

        if (!push_constants) {
            return;
        }

        BindingPoint bp = {};
        if (config.push_constant_binding_point.has_value()) {
            bp = *config.push_constant_binding_point;
        } else {
            // Otherwise, use the binding 0 of the largest used group plus 1, or group 0 if no
            // resources are bound.
            uint32_t group = 0;
            for (auto* inst : *ir.root_block.Get()) {
                if (auto* var_visited = inst->As<core::ir::Var>()) {
                    if (const auto& bp_visited = var_visited->BindingPoint()) {
                        group = std::max(group, bp_visited->group + 1);
                    }
                }
            }
            bp = {group, 0};
        }
        push_constants->SetBindingPoint(bp.group, bp.binding);

        ReplacePushConstantAddressSpace(push_constants->Result());
    }

    /// Replace an output pointer address space to make it `private`.
    /// @param value the output variable
    void ReplacePushConstantAddressSpace(core::ir::InstructionResult* value) {
        // Change the address space to `private`.
        auto* old_ptr_type = value->Type();
        auto* new_ptr_type = ty.ptr(core::AddressSpace::kUniform, old_ptr_type->UnwrapPtr());
        value->SetType(new_ptr_type);

        // Update all uses of the module-scope variable.
        value->ForEachUseUnsorted([&](core::ir::Usage use) {
            if (auto* access = use.instruction->As<core::ir::Access>()) {
                ReplacePushConstantAddressSpace(access->Result());
            }

            if (auto* let = use.instruction->As<core::ir::Let>()) {
                ReplacePushConstantAddressSpace(let->Result());
            }
        });
    }
};

}  // namespace

Result<SuccessType> ChangePushConstantToUniform(core::ir::Module& ir,
                                                const ChangePushConstantToUniformConfig& config) {
    auto result = ValidateAndDumpIfNeeded(ir, "hlsl.ChangePushConstantToUniform");
    if (result != Success) {
        return result.Failure();
    }

    State{config, ir}.Process();

    return Success;
}

}  // namespace tint::hlsl::writer::raise
