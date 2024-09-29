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

#include "src/tint/lang/core/ir/transform/remove_pointer_lets.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/validator.h"

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
        Vector<ir::Let*, 4> worklist;

        // Loop over every instruction looking for control instructions.
        for (auto* inst : ir.Instructions()) {
            if (auto* l = inst->As<ir::Let>()) {
                if (l->Result()->Type()->Is<core::type::Pointer>()) {
                    worklist.Push(l);
                }
            }
        }

        for (auto* l : worklist) {
            ReplaceLetWithValue(l);
        }
    }

    void ReplaceLetWithValue(core::ir::Let* l) {
        l->Result()->ReplaceAllUsesWith(l->Value());
        l->Destroy();
    }
};

}  // namespace

Result<SuccessType> RemovePointerLets(Module& ir) {
    auto result = ValidateAndDumpIfNeeded(ir, "RemovePointerLets transform",
                                          core::ir::Capabilities{
                                              core::ir::Capability::kAllow8BitIntegers,
                                              core::ir::Capability::kAllowPointersInStructures,
                                              core::ir::Capability::kAllowVectorElementPointer,
                                          });
    if (result != Success) {
        return result;
    }

    State{ir}.Process();
    return Success;
}

}  // namespace tint::core::ir::transform
