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

#include "src/tint/lang/core/ir/const_param_validator.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "src/tint/lang/core/builtin_fn.h"
#include "src/tint/lang/core/intrinsic/table.h"
#include "src/tint/lang/core/ir/access.h"
#include "src/tint/lang/core/ir/binary.h"
#include "src/tint/lang/core/ir/bitcast.h"
#include "src/tint/lang/core/ir/block_param.h"
#include "src/tint/lang/core/ir/break_if.h"
#include "src/tint/lang/core/ir/constant.h"
#include "src/tint/lang/core/ir/construct.h"
#include "src/tint/lang/core/ir/continue.h"
#include "src/tint/lang/core/ir/control_instruction.h"
#include "src/tint/lang/core/ir/convert.h"
#include "src/tint/lang/core/ir/core_binary.h"
#include "src/tint/lang/core/ir/core_builtin_call.h"
#include "src/tint/lang/core/ir/disassembler.h"
#include "src/tint/lang/core/ir/discard.h"
#include "src/tint/lang/core/ir/exit_if.h"
#include "src/tint/lang/core/ir/exit_loop.h"
#include "src/tint/lang/core/ir/exit_switch.h"
#include "src/tint/lang/core/ir/function.h"
#include "src/tint/lang/core/ir/function_param.h"
#include "src/tint/lang/core/ir/if.h"
#include "src/tint/lang/core/ir/instruction.h"
#include "src/tint/lang/core/ir/instruction_result.h"
#include "src/tint/lang/core/ir/let.h"
#include "src/tint/lang/core/ir/load.h"
#include "src/tint/lang/core/ir/load_vector_element.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/member_builtin_call.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/lang/core/ir/next_iteration.h"
#include "src/tint/lang/core/ir/override.h"
#include "src/tint/lang/core/ir/referenced_module_vars.h"
#include "src/tint/lang/core/ir/return.h"
#include "src/tint/lang/core/ir/store.h"
#include "src/tint/lang/core/ir/store_vector_element.h"
#include "src/tint/lang/core/ir/switch.h"
#include "src/tint/lang/core/ir/swizzle.h"
#include "src/tint/lang/core/ir/terminate_invocation.h"
#include "src/tint/lang/core/ir/type/array_count.h"
#include "src/tint/lang/core/ir/unary.h"
#include "src/tint/lang/core/ir/unreachable.h"
#include "src/tint/lang/core/ir/unused.h"
#include "src/tint/lang/core/ir/user_call.h"
#include "src/tint/lang/core/ir/value.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/f32.h"
#include "src/tint/lang/core/type/function.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/i8.h"
#include "src/tint/lang/core/type/matrix.h"
#include "src/tint/lang/core/type/memory_view.h"
#include "src/tint/lang/core/type/pointer.h"
#include "src/tint/lang/core/type/reference.h"
#include "src/tint/lang/core/type/storage_texture.h"
#include "src/tint/lang/core/type/type.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/u8.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/core/type/void.h"
#include "src/tint/utils/containers/hashset.h"
#include "src/tint/utils/containers/predicates.h"
#include "src/tint/utils/containers/reverse.h"
#include "src/tint/utils/containers/transform.h"
#include "src/tint/utils/diagnostic/diagnostic.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/macros/defer.h"
#include "src/tint/utils/result/result.h"
#include "src/tint/utils/rtti/castable.h"
#include "src/tint/utils/rtti/switch.h"
#include "src/tint/utils/text/styled_text.h"
#include "src/tint/utils/text/text_style.h"

/// If set to 1 then the Tint will dump the IR when validating.
#define TINT_DUMP_IR_WHEN_VALIDATING 0
#if TINT_DUMP_IR_WHEN_VALIDATING
#include <iostream>
#include "src/tint/utils/text/styled_text_printer.h"
#endif

using namespace tint::core::fluent_types;  // NOLINT

namespace tint::core::ir {

struct ValidatedType {
    const core::type::Type* ty;
};

namespace {

/// The core IR validator.
class Validator {
  public:
    /// Create a core validator
    /// @param mod the module to be validated
    /// @param capabilities the optional capabilities that are allowed
    explicit Validator(const Module& mod);

    /// Destructor
    ~Validator();

    /// Runs the validator over the module provided during construction
    /// @returns success or failure
    Result<SuccessType> Run();

    void CheckSubgroupCall(const CoreBuiltinCall* call);
    void CheckBuiltinCall(const BuiltinCall* call);
    void CheckMemberBuiltinCall(const MemberBuiltinCall* call);

    void CheckCoreBinaryCall(const CoreBinary* inst);
    void CheckExtractInsertBitsCall(const CoreBuiltinCall* call);
    void CheckLdexpCall(const CoreBuiltinCall* call);
    void CheckQuantizeToF16(const CoreBuiltinCall* call);
    void CheckClampCall(const CoreBuiltinCall* call);

    Disassembler& Disassemble();

    diag::Diagnostic& AddError(const Instruction& inst);

  private:
    const Module& mod_;
    std::optional<ir::Disassembler> disassembler_;  // Use Disassemble()
    diag::List diagnostics_;
};

Validator::Validator(const Module& mod) : mod_(mod) {}

Validator::~Validator() = default;

Disassembler& Validator::Disassemble() {
    if (!disassembler_) {
        disassembler_.emplace(ir::Disassembler(mod_));
    }
    return *disassembler_;
}

diag::Diagnostic& Validator::AddError(const Instruction& inst) {
    auto src = Disassemble().InstructionSource(&inst);
    return diagnostics_.AddError(src);
}

bool IfAll(const core::constant::Value* value, auto test_fun) {
    bool ret = true;
    if (value == nullptr) {
        return ret;
    }
    printf("here  %d \n", __LINE__);
    if (value->Index(0) == nullptr) {
        printf("here  %d \n", __LINE__);
        // single element
        if (value->Type()->IsFloatScalar()) {
            ret &= test_fun(value->ValueAs<f32>());
        }

    } else {
        printf("here  %d \n", __LINE__);
        for (size_t i = 0; i < value->NumElements(); i++) {
            ret &= IfAll(value->Index(i), test_fun);
        }
    }
    return ret;
}

void Validator::CheckExtractInsertBitsCall(const CoreBuiltinCall* call) {
    //
}

void Validator::CheckLdexpCall(const CoreBuiltinCall* call) {}

void Validator::CheckQuantizeToF16(const CoreBuiltinCall* call) {
    printf("here  %d \n", __LINE__);
    //  constant values
    auto param0 = call->Args()[0];
    if (auto const_val = param0->As<ir::Constant>()) {
        printf("here  %d \n", __LINE__);
        auto as_aint = const_val->Value();
        // as_aint->AllZero()
        printf("here  %d \n", __LINE__);
        if (!IfAll(as_aint, [](auto val) {
                printf(" value %f \n", float(val));
                return CheckedConvert<f16>(val) == Success;
            })) {
            AddError(*call) << call->FriendlyName() << " outside domain. ";
            printf("here  %d \n", __LINE__);
            printf("test \n");
        }
    }
}

void Validator::CheckSubgroupCall(const CoreBuiltinCall* call) {
    auto param0 = call->Args()[1];
    if (auto const_val = param0->As<ir::Constant>()) {
        auto as_aint = const_val->Value()->ValueAs<AInt>();

        // User friendly param name.
        std::string paramName = "sourceLaneIndex";
        switch (call->Func()) {
            case core::BuiltinFn::kSubgroupShuffleXor:
                paramName = "mask";
                break;
            case core::BuiltinFn::kSubgroupShuffleUp:
            case core::BuiltinFn::kSubgroupShuffleDown:
                paramName = "delta";
                break;
            default:
                break;
        }

        if (as_aint > 127) {
            AddError(*call) << call->FriendlyName() << " "
                            << paramName;  // << "  " <<as_aint.value;
        } else if (as_aint < 0) {
            AddError(*call) << call->FriendlyName() << " "
                            << paramName;  // << "  " <<as_aint.value;
        }
    }
}

void Validator::CheckClampCall(const CoreBuiltinCall* call) {
    // low must be not be greater than high
}

void Validator::CheckBuiltinCall(const BuiltinCall* inst) {
    if (auto* call = inst->As<core::ir::CoreBuiltinCall>()) {
        switch (call->Func()) {
            case core::BuiltinFn::kSubgroupShuffle:
            case core::BuiltinFn::kSubgroupShuffleXor:
            case core::BuiltinFn::kSubgroupShuffleUp:
            case core::BuiltinFn::kSubgroupShuffleDown:
                CheckSubgroupCall(call);
                break;
            case core::BuiltinFn::kExtractBits:
            case core::BuiltinFn::kInsertBits:
                CheckExtractInsertBitsCall(call);
                break;
            case core::BuiltinFn::kLdexp:
                CheckLdexpCall(call);
                break;
            case core::BuiltinFn::kClamp:
                CheckClampCall(call);
                break;
            case core::BuiltinFn::kQuantizeToF16:
                CheckQuantizeToF16(call);
                break;
            default:
                break;
        }
    }
}

void Validator::CheckCoreBinaryCall(const CoreBinary* inst) {
    switch (inst->Op()) {
        case core::BinaryOp::kDivide:
        case core::BinaryOp::kModulo:

            // AddError(node->source) << "integer division by zero is invalid";
            // CheckSubgroupCall(call);
            break;
        default:
            break;
    }
}

void Validator::CheckMemberBuiltinCall(const MemberBuiltinCall* call) {}

Result<SuccessType> Validator::Run() {
    auto instructions = this->mod_.Instructions();

    for (auto inst : instructions) {
        tint::Switch(
            inst,                                                            //
            [&](const BuiltinCall* c) { CheckBuiltinCall(c); },              //
            [&](const MemberBuiltinCall* c) { CheckMemberBuiltinCall(c); },  //
            [&](const CoreBinary* c) { CheckCoreBinaryCall(c); },            //
            [&](Default) {
                // Validation of custom IR instructions
                printf("default name %s \n", inst->FriendlyName().c_str());
            });
    }

    if (diagnostics_.ContainsErrors()) {
        return Failure{std::move(diagnostics_)};
    }

    return Success;
}

}  // namespace

Result<SuccessType> ValidateConstParam(const Module& mod) {
    Validator v(mod);
    auto ret = v.Run();
    return ret;
}

}  // namespace tint::core::ir
