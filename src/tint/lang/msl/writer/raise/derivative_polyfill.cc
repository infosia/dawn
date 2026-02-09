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

#include "src/tint/lang/msl/writer/raise/derivative_polyfill.h"

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/instruction.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/referenced_module_vars.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/containers/vector.h"
#include "src/tint/utils/result.h"

namespace tint::msl::writer::raise {
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

    /// The thread_index_in_quadgroup module-scope variable.
    core::ir::Var* thread_index_in_quadgroup = nullptr;

    /// Process the module, polyfilling `dpdxFine` and `dpdyFine` builtins.
    ///
    /// The builtins can be implemented in terms of quad operations, where the neighboring pixel's
    /// value is fetched with a quad swap, and the difference is calculated. For dpdx, the
    /// difference should be calculated as `right - left`, and for dpdy, `bottom - top`. To
    /// determine whether the current pixel is the subtraction's lhs (right/bottom) or rhs
    /// (top/left), we use the thread_index_in_quadgroup.
    ///
    /// Therefore, given the following thread ids in a quad:
    ///
    ///         Left      Right
    ///     +--------+--------+
    /// Top |   00   |   01   |
    ///     |   (0)  |   (1)  |
    ///     +--------+--------+
    /// Bot |   10   |   11   |
    ///     |   (2)  |   (3)  |
    ///     +--------+--------+
    ///
    /// dpdxFine(v) = is_right_pixel  ? v - quadSwapX(v) : quadSwapX(v) - v
    ///   where is_right_pixel = (thread_id & 0b01) != 0
    ///
    /// dpdyFine(v) = is_bottom_pixel ? v - quadSwapY(v) : quadSwapX(v) - v
    ///   where is_bottom_pixel = (thread_id & 0b10) != 0
    void Process() {
        // Declare the thread_index_in_quadgroup variable in which the builtin value will be stored.
        b.Append(ir.root_block, [&] {
            thread_index_in_quadgroup = b.Var<private_, u32>("tint_thread_index_in_quadgroup");
        });

        // Collect all derivative calls.
        tint::Vector<core::ir::CoreBuiltinCall*, 4> derivative_calls;
        for (auto* inst : ir.Instructions()) {
            if (auto* builtin = inst->As<core::ir::CoreBuiltinCall>()) {
                if (builtin->Func() == core::BuiltinFn::kDpdxFine ||
                    builtin->Func() == core::BuiltinFn::kDpdyFine) {
                    derivative_calls.Push(builtin);
                }
            }
        }

        // Polyfill the derivative calls.
        for (auto* builtin : derivative_calls) {
            b.InsertBefore(builtin, [&] {
                bool is_dpdx = builtin->Func() == core::BuiltinFn::kDpdxFine;
                core::ir::Value* current = builtin->Args()[0];

                // Get the value from the neighboring pixel in the quad.
                core::BuiltinFn quad_swap_fn =
                    is_dpdx ? core::BuiltinFn::kQuadSwapX : core::BuiltinFn::kQuadSwapY;
                auto* neighbor = b.Call(current->Type(), quad_swap_fn, current);

                // Determine whether to use this pixel's value as the lhs of the subtraction in the
                // derivative calculation.
                auto* thread_id = b.Load(thread_index_in_quadgroup);
                auto* bitmask = b.Constant(u32(is_dpdx ? 0b01 : 0b10));
                auto* cond = b.NotEqual(b.And(thread_id, bitmask), b.Constant(u32(0)));

                // Calculate both potential differences.
                auto* current_minus_neighbor = b.Subtract(current, neighbor);
                auto* neighbor_minus_current = b.Subtract(neighbor, current);

                // Select the correct difference.
                auto* result = b.Call(current->Type(), core::BuiltinFn::kSelect,
                                      neighbor_minus_current,  // false value
                                      current_minus_neighbor,  // true value
                                      cond);

                // Make a silly derivative call, zero it, and add it to the result, just so there's
                // another derivative call later in execution to keep helper threads alive.
                auto* dummy = b.Call(current->Type(), core::BuiltinFn::kDpdx, current);
                auto* zero = b.Multiply(dummy, b.Constant(f32(0)));
                auto new_result = b.Add(result, zero);

                builtin->Result()->ReplaceAllUsesWith(new_result->Result());
            });
            builtin->Destroy();
        }

        // Set the thread_index_in_quadgroup variable from all entry points that use it.
        core::ir::ReferencedModuleVars<core::ir::Module> refs(ir,
                                                              [&](const core::ir::Var* var) {  //
                                                                  return var ==
                                                                         thread_index_in_quadgroup;
                                                              });
        for (auto func : ir.functions) {
            if (func->IsEntryPoint()) {
                if (refs.TransitiveReferences(func).Contains(thread_index_in_quadgroup)) {
                    SetThreadIndexInQuadgroupForEntryPoint(func);
                }
            }
        }
    }

    /// Sets the module-scope variable from the entry point's `[[thread_index_in_quadgroup]]`
    /// parameter.
    /// @param ep the entry point
    void SetThreadIndexInQuadgroupForEntryPoint(core::ir::Function* ep) {
        auto* param = b.FunctionParam("tint_thread_index_in_quadgroup", ty.u32());
        param->SetBuiltin(core::BuiltinValue::kThreadIndexInQuadgroup);
        ep->AppendParam(param);

        // Store the value of param into the module-scope variable.
        b.InsertBefore(ep->Block()->Front(), [&] { b.Store(thread_index_in_quadgroup, param); });
    }
};

}  // namespace

Result<SuccessType> DerivativePolyfill(core::ir::Module& ir) {
    TINT_CHECK_RESULT(ValidateAndDumpIfNeeded(ir, "msl.DerivativePolyfill",
                                              tint::core::ir::Capabilities{
                                                  core::ir::Capability::kAllow8BitIntegers,
                                                  core::ir::Capability::kAllowPointSizeBuiltin,
                                                  core::ir::Capability::kAllowDuplicateBindings,
                                                  core::ir::Capability::kAllowNonCoreTypes,
                                              }));

    State{ir}.Process();

    return Success;
}

}  // namespace tint::msl::writer::raise
