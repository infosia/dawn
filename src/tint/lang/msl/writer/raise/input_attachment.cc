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

// tint-tbdr: begin input_attachment-msl
//
// Fork-only file: TBDR input-attachment lowering for the MSL backend. See TILED.md.

#include "src/tint/lang/msl/writer/raise/input_attachment.h"

#include <string>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/input_attachment.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/utils/containers/hashset.h"

namespace tint::msl::writer::raise {

namespace {

using namespace tint::core::fluent_types;  // NOLINT

/// PIMPL state for the transform.
struct State {
    /// The IR module.
    core::ir::Module& ir;

    /// The transform configuration.
    const InputAttachmentConfig& config;

    /// The IR builder.
    core::ir::Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The color slots already used by input attachments in this transform run.
    Hashset<uint32_t, 4> used_color_slots{};

    /// Process the module.
    Result<SuccessType> Process() {
        // Gather all module-scope input_attachment variables. Even unused declarations must be
        // consumed here so they do not reach later MSL lowering passes as unsupported handle types.
        Vector<core::ir::Var*, 4> input_attachments;
        for (auto* inst : *ir.root_block) {
            auto* var = inst->As<core::ir::Var>();
            if (!var) {
                continue;
            }
            if (var->Result()->Type()->UnwrapPtr()->Is<core::type::InputAttachment>()) {
                input_attachments.Push(var);
            }
        }
        if (input_attachments.IsEmpty()) {
            return Success;
        }

        core::ir::Function* entry_point = nullptr;
        for (auto* func : ir.functions) {
            if (func->IsEntryPoint()) {
                entry_point = func;
                break;
            }
        }

        // Pre-seed the used slots with any existing `@color` entry-point parameters (framebuffer
        // fetch), so an input attachment mapped to the same slot fails instead of emitting a
        // duplicate `[[color(N)]]` parameter.
        if (entry_point) {
            for (auto* param : entry_point->Params()) {
                if (auto color = param->Color()) {
                    used_color_slots.Add(color.value());
                }
            }
        }

        for (auto* var : input_attachments) {
            auto res = ProcessVar(var, entry_point);
            if (res != Success) {
                return res;
            }
        }

        return Success;
    }

    /// Lower a single input_attachment variable.
    /// @param var the input_attachment variable
    /// @param entry_point the fragment entry point
    Result<SuccessType> ProcessVar(core::ir::Var* var, core::ir::Function* entry_point) {
        auto* ia = var->Result()->Type()->UnwrapPtr()->As<core::type::InputAttachment>();
        auto* color_ty = ty.vec4(ia->Type());

        // Collect the loads of the attachment handle. The only supported use of an input attachment
        // is `inputAttachmentLoad()` on the value produced by a direct load of the variable.
        Vector<core::ir::Load*, 4> loads;
        bool unsupported_use = false;
        var->Result()->ForEachUseUnsorted([&](core::ir::Usage use) {
            if (auto* load = use.instruction->As<core::ir::Load>()) {
                loads.Push(load);
            } else {
                unsupported_use = true;
            }
        });
        if (unsupported_use) {
            return Failure("unsupported use of input_attachment in the MSL backend");
        }

        // If the attachment is never loaded, drop the variable entirely so it does not reach the
        // printer as an unsupported type.
        if (loads.IsEmpty()) {
            var->Destroy();
            return Success;
        }

        // Loaded input attachments are only valid in fragment shaders, as the entry point must
        // receive the resolved value as a `@color` parameter.
        if (!entry_point || !entry_point->IsFragment()) {
            return Failure("input_attachment requires a fragment entry point");
        }

        auto bp = var->BindingPoint();
        if (!bp) {
            return Failure("input_attachment variable has no binding point");
        }
        auto iter = config.binding_to_color.find(*bp);
        if (iter == config.binding_to_color.end()) {
            return Failure(
                "input_attachment used without a corresponding entry in "
                "Options::input_attachment_to_color_index");
        }
        uint32_t color_slot = iter->second;
        if (!used_color_slots.Add(color_slot)) {
            return Failure("input_attachment color index " + std::to_string(color_slot) +
                           " collides with another color input");
        }

        // Create a private variable to carry the resolved color value. ModuleScopeVars later threads
        // this variable to any helper function that reads the attachment, so the load can live in a
        // function other than the entry point.
        std::string name = "tint_input_attachment_" + std::to_string(color_slot);
        core::ir::Var* color_var = nullptr;
        b.Append(ir.root_block, [&] {
            color_var = b.Var(name, ty.ptr(core::AddressSpace::kPrivate, color_ty));
            color_var->SetInitializer(b.Zero(color_ty));
        });

        // Add a `@color(N)` fragment input parameter and store it into the private variable at the
        // top of the entry point. ShaderIO turns this parameter into a `[[color(N)]]` input.
        auto* param = b.FunctionParam(name, color_ty);
        param->SetColor(color_slot);
        entry_point->AppendParam(param);
        b.InsertBefore(entry_point->Block()->Front(), [&] { b.Store(color_var, param); });

        // Replace each `inputAttachmentLoad()` with a load of the private color variable. Both
        // produce a `vec4<T>`, so this is an identity substitution.
        for (auto* load : loads) {
            Vector<core::ir::CoreBuiltinCall*, 4> calls;
            bool unsupported = false;
            load->Result()->ForEachUseUnsorted([&](core::ir::Usage use) {
                auto* call = use.instruction->As<core::ir::CoreBuiltinCall>();
                if (call && call->Func() == core::BuiltinFn::kInputAttachmentLoad) {
                    calls.Push(call);
                } else {
                    unsupported = true;
                }
            });
            if (unsupported) {
                return Failure("unsupported use of input_attachment in the MSL backend");
            }

            for (auto* call : calls) {
                // Metal resolves the current sample of a subpass input implicitly (via
                // [[sample_id]] execution), so the MSAA overload taking an explicit sample index
                // cannot be lowered here. Reject it cleanly.
                if (call->Args().size() != 1) {
                    return Failure(
                        "inputAttachmentLoad with an explicit sample_index is not supported by "
                        "the MSL backend");
                }
                b.InsertBefore(call, [&] { b.LoadWithResult(call->DetachResult(), color_var); });
                call->Destroy();
            }
            load->Destroy();
        }
        var->Destroy();

        return Success;
    }
};

}  // namespace

Result<SuccessType> InputAttachment(core::ir::Module& ir, const InputAttachmentConfig& config) {
    core::ir::AssertValid(ir, "before msl.InputAttachment");

    return State{ir, config}.Process();
}

}  // namespace tint::msl::writer::raise

// tint-tbdr: end input_attachment-msl
