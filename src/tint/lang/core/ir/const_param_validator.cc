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
#include "src/tint/lang/core/fluent_types.h"
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
#include "src/tint/lang/core/number.h"
#include "src/tint/lang/core/type/abstract_float.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/bool.h"
#include "src/tint/lang/core/type/f16.h"
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
#include "src/tint/utils/internal_limits.h"
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
    explicit Validator(Module& mod);

    /// Destructor
    ~Validator();

    /// Runs the validator over the module provided during construction
    /// @returns success or failure
    Result<SuccessType> Run();

    void CheckSubgroupCall(const CoreBuiltinCall* call);
    void CheckBuiltinCall(const BuiltinCall* call);
    void CheckMemberBuiltinCall(const MemberBuiltinCall* call);

    void CheckCoreBinaryCall(const CoreBinary* inst);
    void CheckExtractBitsCall(const CoreBuiltinCall* call);
    void CheckInsertBitsCall(const CoreBuiltinCall* call);
    void CheckLdexpCall(const CoreBuiltinCall* call);
    void CheckQuantizeToF16(const CoreBuiltinCall* call);
    void CheckClampCall(const CoreBuiltinCall* call);
    void CheckPack2x16float(const CoreBuiltinCall* call);
    void CheckSmoothstepCall(const CoreBuiltinCall* call);
    void CheckBinaryDivModCall(const CoreBinary* call);
    void CheckBinaryShiftCall(const CoreBinary* call);

    Disassembler& Disassemble();

    diag::Diagnostic& AddError(const Instruction& inst);

  private:
    Module& mod_;
    core::constant::Eval const_eval_;
    std::optional<ir::Disassembler> disassembler_;  // Use Disassemble()
    diag::List diagnostics_;
    bool err_status_ = false;
};

Validator::Validator(Module& mod) : mod_(mod), const_eval_(mod.constant_values, diagnostics_) {}

Validator::~Validator() = default;

diag::Diagnostic& Validator::AddError(const Instruction& inst) {
    auto src = mod_.SourceOf(&inst);
    return diagnostics_.AddError(src);
}

const Constant* GetConstArg(const CoreBuiltinCall* call, uint32_t param_index) {
    if (call->Args().Length() <= param_index) {
        return nullptr;
    }
    if (call->Args()[param_index] == nullptr) {
        return nullptr;
    }
    return call->Args()[param_index]->As<ir::Constant>();
}

void Validator::CheckExtractBitsCall(const CoreBuiltinCall* call) {
    // This can be u32/i32 or vector of those types.
    auto* param0 = call->Args()[0];
    auto* const_val_offset = GetConstArg(call, 1);
    auto* const_val_count = GetConstArg(call, 2);
    if (const_val_count && const_val_offset) {
        auto* zero = const_eval_.Zero(param0->Type(), {}, Source{}).Get();
        auto fakeArgs = Vector{zero, const_val_offset->Value(), const_val_count->Value()};
        auto res = const_eval_.extractBits(param0->Type(), fakeArgs, mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckInsertBitsCall(const CoreBuiltinCall* call) {
    // This can be u32/i32 or vector of those types.
    auto* param0 = call->Args()[0];
    auto* const_val_offset = GetConstArg(call, 2);
    auto* const_val_count = GetConstArg(call, 3);
    if (const_val_count && const_val_offset) {
        auto* zero = const_eval_.Zero(param0->Type(), {}, Source{}).Get();
        auto fakeArgs = Vector{zero, zero, const_val_offset->Value(), const_val_count->Value()};
        auto res = const_eval_.insertBits(param0->Type(), fakeArgs, mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckLdexpCall(const CoreBuiltinCall* call) {
    auto* param0 = call->Args()[0];
    auto const_val = GetConstArg(call, 1);
    if (const_val) {
        auto* zero = const_eval_.Zero(param0->Type(), {}, Source{}).Get();
        auto fakeArgs = Vector{zero, const_val->Value()};
        auto res = const_eval_.ldexp(param0->Type(), fakeArgs, mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckQuantizeToF16(const CoreBuiltinCall* call) {
    //  constant values
    if (auto const_val = GetConstArg(call, 0)) {
        auto res = const_eval_.quantizeToF16(call->Result(0)->Type(), Vector{const_val->Value()},
                                             mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckPack2x16float(const CoreBuiltinCall* call) {
    //  constant values
    if (auto const_val = GetConstArg(call, 0)) {
        auto res = const_eval_.pack2x16float(call->Result(0)->Type(), Vector{const_val->Value()},
                                             mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckSubgroupCall(const CoreBuiltinCall* call) {
    if (auto const_val = GetConstArg(call, 1)) {
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
            AddError(*call) << "The " << paramName << " argument of " << call->FriendlyName()
                            << " must be greater than or equal to zero";
        } else if (as_aint < 0) {
            AddError(*call) << "The " << paramName << " argument of " << call->FriendlyName()
                            << " must be less than " << tint::internal_limits::kMaxSubgroupSize;
        }
    }
}

void Validator::CheckClampCall(const CoreBuiltinCall* call) {
    auto* const_val_low = GetConstArg(call, 1);
    auto* const_val_high = GetConstArg(call, 2);
    if (const_val_low && const_val_high) {
        auto fakeArgs =
            Vector{const_val_low->Value(), const_val_low->Value(), const_val_high->Value()};
        auto res = const_eval_.clamp(call->Result(0)->Type(), fakeArgs, mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
}

void Validator::CheckSmoothstepCall(const CoreBuiltinCall* call) {
    auto* const_val_low = GetConstArg(call, 1);
    auto* const_val_high = GetConstArg(call, 2);
    if (const_val_low && const_val_high) {
        auto fakeArgs =
            Vector{const_val_low->Value(), const_val_high->Value(), const_val_high->Value()};
        auto res = const_eval_.smoothstep(call->Result(0)->Type(), fakeArgs, mod_.SourceOf(call));
        err_status_ &= res != Success;
    }
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
                CheckExtractBitsCall(call);
                break;
            case core::BuiltinFn::kInsertBits:
                CheckInsertBitsCall(call);
                break;
            case core::BuiltinFn::kLdexp:
                CheckLdexpCall(call);
                break;
            case core::BuiltinFn::kClamp:
                CheckClampCall(call);
                break;
            case core::BuiltinFn::kSmoothstep:
                CheckSmoothstepCall(call);
                break;
            case core::BuiltinFn::kQuantizeToF16:
                CheckQuantizeToF16(call);
                break;
            case core::BuiltinFn::kPack2X16Float:
                CheckPack2x16float(call);
                break;
            default:
                break;
        }
    }
}

void Validator::CheckBinaryDivModCall(const CoreBinary* call) {
    // Integer division by zero should be checked for the partial evaluation case (only rhs
    // is const). FP division by zero is only invalid when the whole expression is
    // constant-evaluated.
    if (call->RHS()->Type()->IsIntegerScalarOrVector()) {
        auto rhs_constant = call->RHS()->As<ir::Constant>();
        if (rhs_constant->Value()->AnyZero()) {
            AddError(*call) << "integer division by zero is invalid";
        }
    }
}

void Validator::CheckBinaryShiftCall(const CoreBinary* call) {
    // If lhs value is a concrete type, and rhs is a const-expression greater than or equal
    // to the bit width of lhs, then it is a shader-creation error.
    const auto* elem_type = call->LHS()->Type()->UnwrapRef()->DeepestElement();
    const uint32_t bit_width = elem_type->Size() * 8;
    if (auto* rhs_val_as_const = call->RHS()->As<ir::Constant>()) {
        auto* rhs_as_value = rhs_val_as_const->Value();
        for (size_t i = 0, n = rhs_as_value->NumElements(); i < n; i++) {
            auto* shift_val = n == 1 ? rhs_as_value : rhs_as_value->Index(i);
            if (shift_val->ValueAs<u32>() >= bit_width) {
                AddError(*call) << "shift "
                                << (call->Op() == core::BinaryOp::kShiftLeft ? "left" : "right")
                                << " value must be less than the bit width of the lhs, which is "
                                << bit_width;
                break;
            }
        }
    }
}

void Validator::CheckCoreBinaryCall(const CoreBinary* call) {
    switch (call->Op()) {
        case core::BinaryOp::kDivide:
        case core::BinaryOp::kModulo:
            CheckBinaryDivModCall(call);
            break;
        case core::BinaryOp::kShiftLeft:
        case core::BinaryOp::kShiftRight:
            CheckBinaryShiftCall(call);
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
            inst,                                                  //
            [&](const BuiltinCall* c) { CheckBuiltinCall(c); },    //
            [&](const CoreBinary* c) { CheckCoreBinaryCall(c); },  //
            [&](Default) {});
    }

    if (diagnostics_.ContainsErrors()) {
        return Failure{std::move(diagnostics_)};
    }

    return Success;
}

}  // namespace

Result<SuccessType> ValidateConstParam(Module& mod) {
    Validator v(mod);
    auto ret = v.Run();
    return ret;
}

}  // namespace tint::core::ir
