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

#include "src/tint/lang/core/ir/analysis/integer_range_analysis.h"

#include <limits>

#include "src/tint/lang/core/constant/scalar.h"
#include "src/tint/lang/core/ir/loop.h"
#include "src/tint/lang/core/ir/traverse.h"
#include "src/tint/lang/core/ir/var.h"
#include "src/tint/lang/core/type/pointer.h"

namespace tint::core::ir::analysis {

IntegerRangeInfo::IntegerRangeInfo(int64_t min_bound, int64_t max_bound) {
    TINT_ASSERT(min_bound <= max_bound);
    range = SignedIntegerRange{min_bound, max_bound};
}

IntegerRangeInfo::IntegerRangeInfo(uint64_t min_bound, uint64_t max_bound) {
    TINT_ASSERT(min_bound <= max_bound);
    range = UnsignedIntegerRange{min_bound, max_bound};
}

struct IntegerRangeAnalysisImpl {
    explicit IntegerRangeAnalysisImpl(Function* func) : function_(func) {
        // Analyze all of the loops in the function.
        Traverse(func->Block(), [&](Loop* l) { AnalyzeLoop(l); });
    }

    const IntegerRangeInfo* GetInfo(const FunctionParam* param, uint32_t index) {
        if (!param->Type()->IsIntegerScalarOrVector()) {
            return nullptr;
        }

        const auto& info = integer_function_param_range_info_map_.GetOrAdd(
            param, [&]() -> Vector<IntegerRangeInfo, 3> {
                if (param->Builtin() == core::BuiltinValue::kLocalInvocationIndex) {
                    // We shouldn't be trying to use range analysis on a module that has
                    // non-constant workgroup sizes, since we will always have replaced pipeline
                    // overrides with constant values early in the pipeline.
                    TINT_ASSERT(function_->WorkgroupSizeAsConst().has_value());
                    std::array<uint32_t, 3> workgroup_size =
                        function_->WorkgroupSizeAsConst().value();
                    uint64_t max_bound =
                        workgroup_size[0] * workgroup_size[1] * workgroup_size[2] - 1u;
                    constexpr uint64_t kMinBound = 0;

                    return {IntegerRangeInfo(kMinBound, max_bound)};
                }

                if (param->Builtin() == core::BuiltinValue::kLocalInvocationId) {
                    TINT_ASSERT(function_->WorkgroupSizeAsConst().has_value());
                    std::array<uint32_t, 3> workgroup_size =
                        function_->WorkgroupSizeAsConst().value();

                    constexpr uint64_t kMinBound = 0;
                    Vector<IntegerRangeInfo, 3> integerRanges;
                    for (uint32_t size_x_y_z : workgroup_size) {
                        integerRanges.Push({kMinBound, size_x_y_z - 1u});
                    }
                    return integerRanges;
                }

                if (param->Type()->IsUnsignedIntegerScalar()) {
                    return {IntegerRangeInfo(0, std::numeric_limits<uint64_t>::max())};
                } else {
                    TINT_ASSERT(param->Type()->IsSignedIntegerScalar());
                    return {IntegerRangeInfo(std::numeric_limits<int64_t>::min(),
                                             std::numeric_limits<int64_t>::max())};
                }
            });

        TINT_ASSERT(info.Length() > index);
        return &info[index];
    }

    const IntegerRangeInfo* GetInfo(const Var* var) {
        return integer_var_range_info_map_.Get(var).value;
    }

    /// Analyze a loop to compute the range of the loop control variable if possible.
    void AnalyzeLoop(Loop* loop) {
        if (auto* init_block = loop->Initializer()) {
            // Look for variables that could be used as iteration indices.
            for (auto* inst : *init_block) {
                auto* var = inst->As<Var>();
                if (!var) {
                    continue;
                }

                const auto* pty = var->Result(0)->Type()->As<core::type::Pointer>();
                if (!pty->StoreType()->IsIntegerScalar()) {
                    break;
                }

                std::variant<std::monostate, int64_t, uint64_t> bound0 =
                    GetBoundFromInitializer(var);
                if (std::holds_alternative<std::monostate>(bound0)) {
                    break;
                }

                // This is incorrect because bound0 may not always be the lower bound. We can only
                // get the correct range after getting another bound from the loop exit condition.
                // TODO(348701956): Compute another bound from the loop exit condition.
                if (std::holds_alternative<uint64_t>(bound0)) {
                    uint64_t uintBound0 = std::get<uint64_t>(bound0);
                    uint64_t uintBound1 = std::numeric_limits<uint64_t>::max();
                    integer_var_range_info_map_.Add(var, IntegerRangeInfo(uintBound0, uintBound1));
                } else {
                    int64_t intBound0 = std::get<int64_t>(bound0);
                    int64_t intBound1 = std::numeric_limits<int64_t>::max();
                    integer_var_range_info_map_.Add(var, IntegerRangeInfo(intBound0, intBound1));
                }
                return;
            }
        }
    }

    std::variant<std::monostate, int64_t, uint64_t> GetBoundFromInitializer(Var* var) {
        const auto* initializer = var->Initializer();
        if (!initializer) {
            return {};
        }

        const auto* constInitializer = initializer->As<Constant>();
        if (!constInitializer) {
            return {};
        }

        return GetValueFromConstantVal(constInitializer);
    }

    std::variant<std::monostate, int64_t, uint64_t> GetValueFromConstantVal(const Constant* value) {
        if (value->Type()->IsSignedIntegerScalar()) {
            const auto* i32Initializer = value->Value()->As<core::constant::Scalar<i32>>();
            TINT_ASSERT(i32Initializer);
            return static_cast<int64_t>(i32Initializer->ValueAs<int32_t>());
        } else {
            const auto* u32Initializer = value->Value()->As<core::constant::Scalar<u32>>();
            TINT_ASSERT(u32Initializer);
            return static_cast<uint64_t>(u32Initializer->ValueAs<uint32_t>());
        }
    }

  private:
    Function* function_;
    Hashmap<const FunctionParam*, Vector<IntegerRangeInfo, 3>, 4>
        integer_function_param_range_info_map_;
    Hashmap<const Var*, IntegerRangeInfo, 8> integer_var_range_info_map_;
};

IntegerRangeAnalysis::IntegerRangeAnalysis(Function* func)
    : impl_(new IntegerRangeAnalysisImpl(func)) {}
IntegerRangeAnalysis::~IntegerRangeAnalysis() = default;

const IntegerRangeInfo* IntegerRangeAnalysis::GetInfo(const FunctionParam* param, uint32_t index) {
    return impl_->GetInfo(param, index);
}

const IntegerRangeInfo* IntegerRangeAnalysis::GetInfo(const Var* var) {
    return impl_->GetInfo(var);
}

}  // namespace tint::core::ir::analysis
