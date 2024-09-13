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

#include "src/tint/lang/hlsl/writer/raise/num_workgroups_from_uniform.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::hlsl::writer::raise {

namespace {

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
        for (auto& func : ir.functions) {
            // num_workgroups only applies to compute shaders
            if (func->Stage() != core::ir::Function::PipelineStage::kCompute) {
                continue;
            }

            for (auto* param : func->Params()) {
                auto* st = param->Type()->As<core::type::Struct>();
                if (!st) {
                    continue;
                }
                for (auto* m : st->Members()) {
                    if (m->Attributes().builtin != core::BuiltinValue::kNumWorkgroups) {
                        continue;
                    }

                    // Create UBO that will receive the number of workgroups
                    // core::ir::Value* ubo = nullptr;  // TODO
                    // core::ir::Value* ubo_num_workgroups_member = nullptr;
                    // {
                        // Create new struct
                        auto* ubo_struct_ty = ty.Struct(ir.symbols.New(), {
                            {ir.symbols.New(), ty.vec3<u32>()}
                        });
                        // Create module uniform variable of new struct type
                        auto* ubo_var = b.Var(ty.ptr(uniform, ubo_struct_ty));
                        ubo_var->SetBindingPoint(0_u, 0_u); // TODO: Use config
                        ir.root_block.Get()->Append(ubo_var);

                    // }

                    // Replace all member accessor uses of the param with the UBO
                    auto usages = param->UsagesUnsorted();
                    for (const core::ir::Usage& usage : usages) {
                        if (auto* access = usage.instruction->As<core::ir::Access>()) {
                            b.InsertBefore(access, [&] {
                                // Create access to num_wgs member of ubo
                                auto* new_access = b.Access(ty.ptr(uniform, ty.vec3<u32>()), ubo_var, 0_u);
                                auto* new_load = b.Load(new_access);

                                // Replace old access with new one
                                access->Result(0)->ReplaceAllUsesWith(new_load->Result(0));
                            });
                            access->Destroy();
                        }
                    }
                }
            }
        }
    }

    // std::tuple<core::ir::Value*, core::ir::Value*> CreateUbo() {

    // }
};
}  // namespace

Result<SuccessType> NumWorkgroupsFromUniform(core::ir::Module& ir,
                                             const NumWorkgroupsFromUniformConfig& config) {
    auto result = ValidateAndDumpIfNeeded(ir, "NumWorkgroupsFromUniform transform");
    if (result != Success) {
        return result;
    }

    State{ir}.Process();
    return Success;
}

}  // namespace tint::hlsl::writer::raise
