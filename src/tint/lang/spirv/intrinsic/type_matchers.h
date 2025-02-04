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

#ifndef SRC_TINT_LANG_SPIRV_INTRINSIC_TYPE_MATCHERS_H_
#define SRC_TINT_LANG_SPIRV_INTRINSIC_TYPE_MATCHERS_H_

#include "src/tint/lang/core/intrinsic/table.h"
#include "src/tint/lang/core/type/array.h"
#include "src/tint/lang/core/type/i32.h"
#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/struct.h"
#include "src/tint/lang/core/type/u32.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/lang/spirv/builtin_type.h"
#include "src/tint/lang/spirv/type/builtin_structs.h"
#include "src/tint/lang/spirv/type/sampled_image.h"

namespace tint::spirv::intrinsic {

inline bool MatchStructWithRuntimeArray(core::intrinsic::MatchState&, const core::type::Type* ty) {
    if (auto* str = ty->As<core::type::Struct>()) {
        if (str->Members().IsEmpty()) {
            return false;
        }
        if (auto* ary = str->Members().Back()->Type()->As<core::type::Array>()) {
            if (ary->Count()->Is<core::type::RuntimeArrayCount>()) {
                return true;
            }
        }
    }
    return false;
}

inline const core::type::Type* BuildStructWithRuntimeArray(core::intrinsic::MatchState&,
                                                           const core::type::Type* ty) {
    return ty;
}

inline bool MatchSampledImage(core::intrinsic::MatchState&,
                              const core::type::Type* ty,
                              const core::type::Type*& T) {
    if (ty->Is<core::intrinsic::Any>()) {
        T = ty;
        return true;
    }
    if (auto* v = ty->As<spirv::type::SampledImage>()) {
        T = v->Image();
        return true;
    }
    return false;
}

inline const core::type::Type* BuildSampledImage(core::intrinsic::MatchState& state,
                                                 const core::type::Type*,
                                                 const core::type::Type* T) {
    return state.types.Get<type::SampledImage>(T);
}

inline bool MatchSpirvFrexpResultI32(core::intrinsic::MatchState&,
                                     const core::type::Type* ty,
                                     const core::type::Type*& T) {
    if (!ty->Is<core::intrinsic::Any>()) {
        return false;
    }
    T = ty;
    return true;
}

inline bool MatchSpirvFrexpResultU32(core::intrinsic::MatchState&,
                                     const core::type::Type* ty,
                                     const core::type::Type*& T) {
    if (!ty->Is<core::intrinsic::Any>()) {
        return false;
    }
    T = ty;
    return true;
}

inline bool MatchSpirvFrexpResultVecI32(core::intrinsic::MatchState&,
                                        const core::type::Type* ty,
                                        core::intrinsic::Number& N,
                                        const core::type::Type*& T) {
    if (!ty->Is<core::intrinsic::Any>()) {
        return false;
    }
    N = core::intrinsic::Number::any;
    T = ty;
    return true;
}

inline bool MatchSpirvFrexpResultVecU32(core::intrinsic::MatchState&,
                                        const core::type::Type* ty,
                                        core::intrinsic::Number& N,
                                        const core::type::Type*& T) {
    if (!ty->Is<core::intrinsic::Any>()) {
        return false;
    }
    N = core::intrinsic::Number::any;
    T = ty;
    return true;
}

inline const core::type::Struct* BuildSpirvFrexpResultI32(core::intrinsic::MatchState& state,
                                                          const core::type::Type*,
                                                          const core::type::Type* el) {
    return spirv::type::BuildFrexpStruct(state.types, state.symbols, el, state.types.i32());
}

inline const core::type::Struct* BuildSpirvFrexpResultU32(core::intrinsic::MatchState& state,
                                                          const core::type::Type*,
                                                          const core::type::Type* el) {
    return spirv::type::BuildFrexpStruct(state.types, state.symbols, el, state.types.u32());
}

inline const core::type::Struct* BuildSpirvFrexpResultVecI32(core::intrinsic::MatchState& state,
                                                             const core::type::Type*,
                                                             core::intrinsic::Number& n,
                                                             const core::type::Type* el) {
    auto* vec = state.types.vec(el, n.Value());
    return spirv::type::BuildFrexpStruct(state.types, state.symbols, vec,
                                         state.types.MatchWidth(state.types.i32(), vec));
}

inline const core::type::Struct* BuildSpirvFrexpResultVecU32(core::intrinsic::MatchState& state,
                                                             const core::type::Type*,
                                                             core::intrinsic::Number& n,
                                                             const core::type::Type* el) {
    auto* vec = state.types.vec(el, n.Value());
    return spirv::type::BuildFrexpStruct(state.types, state.symbols, vec,
                                         state.types.MatchWidth(state.types.u32(), vec));
}

}  // namespace tint::spirv::intrinsic

#endif  // SRC_TINT_LANG_SPIRV_INTRINSIC_TYPE_MATCHERS_H_
