// Copyright 2023 The Dawn & Tint Authors
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

#include "src/tint/lang/msl/writer/raise/module_constant.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/utils/containers/hashmap.h"

using namespace tint::core::number_suffixes;  // NOLINT
using namespace tint::core::fluent_types;     // NOLINT

namespace tint::msl::writer::raise {

namespace {

/// PIMPL state for the transform.
struct State {
    core::ir::Module& ir;
    core::ir::Builder b{ir};
    core::type::Manager& ty{ir.Types()};

    /// Traversal action for WalkAccessChain.
    enum class Action { kStop, kContinue };

    void Process() {
        Hashmap<core::ir::Value*, core::ir::Value*, 16> object_to_var;
        for (auto* inst : ir.Instructions()) {
            auto* access = inst->As<core::ir::Access>();
            if (!access) {
                continue;
            }

            auto* source_object = access->Object();
            auto* curr_const = source_object->As<core::ir::Constant>();
            if (!curr_const) {
                continue;
            }

            // only handle arrays for now. Probably also structs
            if (!curr_const->Type()->Is<core::type::Array>()) {
                continue;
            }
            // Declare a variable and copy the source object to it.
            auto* var = object_to_var.GetOrAdd(source_object, [&] {
                // If the source object is a constant we use a module-scope variable
                core::ir::Let* decl = b.Let(curr_const);
                ir.root_block->Append(decl);
                return decl->Result();
            });
            access->SetOperand(core::ir::Access::kObjectOperandOffset, var);
        }
    }
};

}  // namespace

Result<SuccessType> ModuleConstant(core::ir::Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "msl.ModuleConstant", kModuleConstant);
    if (result != Success) {
        return result;
    }

    State{ir}.Process();

    return Success;
}

}  // namespace tint::msl::writer::raise
