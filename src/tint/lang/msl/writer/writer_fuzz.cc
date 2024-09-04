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

#include "src/tint/cmd/fuzz/ir/fuzz.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/input_attachment.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/msl/writer/helpers/generate_bindings.h"
#include "src/tint/lang/msl/writer/writer.h"

namespace tint::msl::writer {
namespace {

bool CanRun(const core::ir::Module& module) {
    // Check for push constants and input attachments, which the MSL writer does not support.
    for (auto* inst : *module.root_block) {
        auto* var = inst->As<core::ir::Var>();
        auto* ptr = var->Result(0)->Type()->As<core::type::Pointer>();
        if (ptr->AddressSpace() == core::AddressSpace::kPushConstant) {
            return false;
        }
        if (ptr->StoreType()->Is<core::type::InputAttachment>()) {
            return false;
        }
    }
    return true;
}

void IRFuzzer(core::ir::Module& module, Options options) {
    if (!CanRun(module)) {
        return;
    }

    options.bindings = GenerateBindings(module);
    [[maybe_unused]] auto output = Generate(module, options);
}

}  // namespace
}  // namespace tint::msl::writer

TINT_IR_MODULE_FUZZER(tint::msl::writer::IRFuzzer);
