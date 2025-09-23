// Copyright 2022 The Dawn & Tint Authors
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

#include "src/tint/lang/core/constant/scalar.h"

TINT_INSTANTIATE_TYPEINFO(tint::core::constant::ScalarBase, "const::ScalarBase");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::AInt>, "const::Scalar<AInt>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::AFloat>,
                          "const::Scalar<AFloat>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::i32>, "const::Scalar<I32>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::i8>, "const::Scalar<I8>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::u32>, "const::Scalar<U32>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::u64>, "const::Scalar<U64>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::u8>, "const::Scalar<U8>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::f16>, "const::Scalar<F16>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<tint::core::f32>, "const::Scalar<F32>");
TINT_INSTANTIATE_TYPEINFO(tint::core::constant::Scalar<bool>, "const::Scalar<Bool>");

namespace tint::core::constant {

ScalarBase::~ScalarBase() = default;

}
