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

#include "src/tint/lang/core/ir/transform/builtin_scalarize.h"
#include <cstdint>
#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/sampled_texture.h"
#include "src/tint/lang/core/type/texture.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::core::ir::transform {

namespace {

/// PIMPL state for the transform.
struct State {
    /// The polyfill config.
    const BuiltinScalarizeConfig& config;

    /// The IR module.
    Module& ir;

    /// The IR builder.
    Builder b{ir};

    /// The type manager.
    core::type::Manager& ty{ir.Types()};

    /// The symbol table.
    SymbolTable& sym{ir.symbols};

    // We must have a pre-approved scalarization list as the semantics for allowable scalarization
    // is not simply the function signature. An example here is 'cross' which is a vec3 for all
    // input and return parameters but cannot be scalarized.
    bool ShouldAttemptScalarize(ir::CoreBuiltinCall* builtin) {
        auto builtin_enum = builtin->Func();
        switch (builtin_enum) {
            case core::BuiltinFn::kClamp:
                return config.scalarize_clamp;
            case core::BuiltinFn::kMax:
                return config.scalarize_max;
            case core::BuiltinFn::kMin:
                return config.scalarize_min;
            default:
                return false;
        }
    }

    void Process() {
        Vector<ir::CoreBuiltinCall*, 4> worklist;
        for (auto* inst : ir.Instructions()) {
            if (auto* builtin = inst->As<ir::CoreBuiltinCall>()) {
                if (ShouldAttemptScalarize(builtin)) {
                    worklist.Push(builtin);
                }
            }
        }

        for (auto* builtin : worklist) {
            ScalarizeBuiltin(builtin);
        }
    }

    void ScalarizeBuiltin(ir::CoreBuiltinCall* builtin) {
        uint32_t common_width = 0;
        const core::type::Type* vec_type = nullptr;
        for (auto& e : builtin->Args()) {
            if (!e->Type()->IsScalarVector()) {
                // Only scalars or vectors are supported.
                return;
            }

            if (auto* vec = e->Type()->As<core::type::Vector>()) {
                if (common_width == 0) {
                    common_width = vec->Width();
                    vec_type = vec->DeepestElement();
                }
                // It would be an error to scalarize over different sized vectors.
                TINT_ASSERT(common_width == vec->Width());
            }
        }
        if (common_width == 0) {
            // No vector found. Already scalar.
            return;
        }

        b.InsertBefore(builtin, [&] {
            Vector<core::ir::Value*, 4> args;
            for (uint32_t i = 0; i < common_width; i++) {
                Vector<core::ir::Value*, 4> scalar_args;

                for (auto& e : builtin->Args()) {
                    if (auto* vec = e->Type()->As<core::type::Vector>()) {
                        auto* access_arg = b.Access(vec->DeepestElement(), e, u32(i));
                        scalar_args.Push(access_arg->Result());
                    } else {
                        scalar_args.Push(e);
                    }
                }
                auto* scalar_call = b.Call(vec_type, builtin->Func(), std::move(scalar_args));
                args.Push(scalar_call->Result());
            }
            // Places result back into a vector.
            b.ConstructWithResult(builtin->DetachResult(), std::move(args));
        });
        builtin->Destroy();
    }
};

}  // namespace

Result<SuccessType> BuiltinScalarize(Module& ir, const BuiltinScalarizeConfig& config) {
    auto result =
        ValidateAndDumpIfNeeded(ir, "core.BuiltinScalarize", kBuiltinScalarizeCapabilities);
    if (result != Success) {
        return result;
    }

    State{config, ir}.Process();

    return Success;
}

}  // namespace tint::core::ir::transform
