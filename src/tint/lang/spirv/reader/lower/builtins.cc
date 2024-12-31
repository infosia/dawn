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

#include "src/tint/lang/spirv/reader/lower/builtins.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/spirv/ir/builtin_call.h"

namespace tint::spirv::reader::lower {

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
        Vector<spirv::ir::BuiltinCall*, 4> builtin_worklist;
        for (auto* inst : ir.Instructions()) {
            if (auto* builtin = inst->As<spirv::ir::BuiltinCall>()) {
                builtin_worklist.Push(builtin);
            }
        }

        // Replace the builtins that we found.
        for (auto* builtin : builtin_worklist) {
            switch (builtin->Func()) {
                case spirv::BuiltinFn::kSignI:
                case spirv::BuiltinFn::kSignU:
                case spirv::BuiltinFn::kSignIU:
                case spirv::BuiltinFn::kSignUI:
                    Sign(builtin);
                    break;
                default:
                    TINT_UNREACHABLE();
            }
        }
    }

    // The `spirv.sign` method takes a `u32` operand which is not accepted in WGSL. If the operand
    // is a `u32` or a `vec<N, u32>` then we need to bitcast the operand to `i32` before doing the
    // comparison.
    void Sign(spirv::ir::BuiltinCall* call) {
        auto* arg = call->Args()[0];

        b.InsertBefore(call, [&] {
            auto* result_ty = call->Result(0)->Type();
            if (arg->Type()->IsUnsignedIntegerScalarOrVector()) {
                arg = b.Bitcast(ty.MatchWidth(ty.i32(), result_ty), arg)->Result(0);
            }
            auto* new_call =
                b.Call(result_ty, core::BuiltinFn::kSign, Vector<core::ir::Value*, 1>{arg});

            core::ir::Value* replacement = new_call->Result(0);
            // If the call is a `u32` result type, we need to cast it to `i32`.
            if (result_ty->DeepestElement() == ty.u32()) {
                new_call->Result(0)->SetType(ty.MatchWidth(ty.i32(), result_ty));
                replacement = b.Bitcast(result_ty, replacement)->Result(0);
            }
            call->Result(0)->ReplaceAllUsesWith(replacement);
        });
        call->Destroy();
    }
};

}  // namespace

Result<SuccessType> Builtins(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "spirv.Builtins");
    if (result != Success) {
        return result.Failure();
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::spirv::reader::lower
