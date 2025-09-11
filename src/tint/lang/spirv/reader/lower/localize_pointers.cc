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

#include "src/tint/lang/spirv/reader/lower/localize_pointers.h"

#include "src/tint/lang/core/ir/access.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/instruction.h"
#include "src/tint/lang/core/ir/value.h"

namespace tint::spirv::reader::lower {
namespace {

// Returns true if the defining block `def_blk` dominates (or is) the `use_blk`.
static bool Dominates(core::ir::Block* def_blk, core::ir::Block* use_blk) {
    while (use_blk) {
        if (use_blk == def_blk) {
            return true;
        }
        if (!use_blk->Parent()) {
            break;
        }
        use_blk = use_blk->Parent()->Block();
    }
    return false;
}

}  // namespace

Result<SuccessType> LocalizePointers(core::ir::Module& mod) {
    core::ir::Builder b(mod);
    for (auto* inst : mod.Instructions()) {
        auto* use_blk = inst->Block();
        for (size_t i = 0; i < inst->Operands().Length(); i++) {
            auto* v = inst->Operands()[i];
            if (!v || !v->Type()->Is<core::type::Pointer>()) {
                continue;
            }
            if (auto* res = v->As<core::ir::InstructionResult>()) {
                auto* def_inst = res->Instruction();
                auto* def_blk = def_inst->Block();
                if (def_blk && use_blk && !Dominates(def_blk, use_blk)) {
                    if (auto* acc = def_inst->As<core::ir::Access>()) {
                        Vector<core::ir::Value*, 8> idxs;
                        for (auto* idx : acc->Indices()) {
                            idxs.Push(idx);
                        }
                        auto* localized = b.Access(v->Type(), acc->Object(), std::move(idxs));
                        localized->InsertBefore(inst);
                        inst->SetOperand(i, localized->Result());
                    }
                }
            }
        }
    }
    return Success;
}

}  // namespace tint::spirv::reader::lower

