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

#include "src/tint/lang/wgsl/reader/lower/lower.h"

#include <utility>

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/wgsl/builtin_fn_convert.h"
#include "src/tint/lang/wgsl/ir/builtin_call.h"
#include "src/tint/utils/ice/ice.h"

namespace tint::wgsl::reader {

Result<SuccessType> Lower(core::ir::Module& mod) {
    auto res =
        core::ir::ValidateAndDumpIfNeeded(mod, "wgsl.Lower",
                                          core::ir::Capabilities{
                                              core::ir::Capability::kAllowMultipleEntryPoints,
                                              core::ir::Capability::kAllowOverrides,
                                              core::ir::Capability::kAllowResourceBinding,
                                              core::ir::Capability::kAllow8BitIntegers,
                                          });
    if (res != Success) {
        return res.Failure();
    }

    core::ir::Builder b{mod};
    core::type::Manager& ty{mod.Types()};
    for (auto* inst : mod.Instructions()) {
        if (auto* call = inst->As<wgsl::ir::BuiltinCall>()) {
            switch (call->Func()) {
                case BuiltinFn::kWorkgroupUniformLoad: {
                    auto* param0 = call->Args()[0];
                    TINT_ASSERT(param0->Type()->Is<core::type::Pointer>());
                    auto* storeType = param0->Type()->As<core::type::Pointer>()->StoreType();
                    // Replace:
                    //    %value = call workgroupUniformLoad %ptr
                    // With:
                    //    call workgroupBarrier
                    //    %value = {load || atomicLoad} &ptr
                    //    call workgroupBarrier
                    b.InsertBefore(call, [&] {
                        b.Call(ty.void_(), core::BuiltinFn::kWorkgroupBarrier);
                        if (storeType->Is<core::type::Atomic>()) {
                            b.CallWithResult(call->DetachResult(), core::BuiltinFn::kAtomicLoad,
                                             param0);
                        } else {
                            b.LoadWithResult(call->DetachResult(), param0);
                        }
                        b.Call(ty.void_(), core::BuiltinFn::kWorkgroupBarrier);
                    });
                    break;
                }
                default: {
                    Vector<core::ir::Value*, 8> args(call->Args());
                    auto* replacement = b.CallWithResult(
                        call->DetachResult(), ConvertToCore(call->Func()), std::move(args));
                    if (!call->ExplicitTemplateParams().IsEmpty()) {
                        replacement->SetExplicitTemplateParams(call->ExplicitTemplateParams());
                    }
                    call->ReplaceWith(replacement);
                    break;
                }
            }
            call->Destroy();
        }
    }
    return Success;
}

}  // namespace tint::wgsl::reader
