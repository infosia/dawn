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

#include "src/tint/lang/spirv/type/builtin_structs.h"

#include "src/tint/lang/core/type/vector.h"

namespace tint::spirv::type {

core::type::Struct* BuildFrexpStruct(core::type::Manager& type,
                                     tint::SymbolTable& symbols,
                                     const core::type::Type* fract_ty,
                                     const core::type::Type* exp_ty) {
    std::stringstream name;
    name << ToString(spirv::BuiltinType::kSpirvFrexpResult);
    name << "_";
    if (auto* vec = fract_ty->As<core::type::Vector>()) {
        name << "vec" << vec->Width() << "_";
    }
    name << fract_ty->DeepestElement()->FriendlyName() << "_";

    if (auto* vec = exp_ty->As<core::type::Vector>()) {
        name << "vec" << vec->Width() << "_";
    }
    name << exp_ty->DeepestElement()->FriendlyName();

    auto symbol = symbols.Register(name.str());
    if (auto* existing = type.Find<core::type::Struct>(symbol)) {
        return existing;
    }
    return type.Struct(symbol,
                       {{symbols.Register("fract"), fract_ty}, {symbols.Register("exp"), exp_ty}});
}

}  // namespace tint::spirv::type
