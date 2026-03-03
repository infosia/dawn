// Copyright 2026 The Dawn & Tint Authors
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

#ifndef SRC_TINT_UTILS_CONTAINERS_REINTERPRET_H_
#define SRC_TINT_UTILS_CONTAINERS_REINTERPRET_H_

#include <cstdint>
#include <span>
#include <type_traits>

#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/memory/bitcast.h"
#include "src/tint/utils/rtti/castable.h"
#include "src/tint/utils/rtti/traits.h"

namespace tint {

/// Mode enumerator for Reinterpret
enum class ReinterpretMode {
    /// Only upcasts of pointers are permitted
    kSafe,
    /// Potentially unsafe downcasts of pointers are also permitted
    kUnsafe,
};

namespace detail {

template <typename TO, typename FROM>
static constexpr bool ConstRemoved = std::is_const_v<FROM> && !std::is_const_v<TO>;

/// Private implementation of tint::CanReinterpret.
/// Specialized for the case of TO equal to FROM, which is the common case, and avoids inspection of
/// the base classes, which can be troublesome if the view is of an incomplete type.
template <ReinterpretMode MODE, typename TO, typename FROM>
struct CanReinterpret {
  private:
    using TO_EL = std::remove_pointer_t<std::decay_t<TO>>;
    using FROM_EL = std::remove_pointer_t<std::decay_t<FROM>>;

  public:
    /// @see tint::CanReinterpret
    static constexpr bool value =
        // const can only be applied, not removed
        !ConstRemoved<TO, FROM> &&

        // Both TO and FROM are the same type (ignoring const)
        (std::is_same_v<std::remove_const_t<TO>, std::remove_const_t<FROM>> ||

         // Both TO and FROM are pointers...
         ((std::is_pointer_v<std::remove_const_t<TO>> &&
           std::is_pointer_v<std::remove_const_t<FROM>>) &&

          // const can only be applied to element type, not removed
          !ConstRemoved<std::remove_pointer_t<std::remove_const_t<TO>>,
                        std::remove_pointer_t<std::remove_const_t<FROM>>> &&

          // Either:
          // * Both the pointer elements are of the same type (ignoring const)
          // * Both the pointer elements are both Castable, and MODE is kUnsafe, or FROM is of,
          // or
          //   derives from TO
          (std::is_same_v<std::remove_const_t<FROM_EL>, std::remove_const_t<TO_EL>> ||
           (IsCastable<FROM_EL, TO_EL> &&
            (MODE == ReinterpretMode::kUnsafe || tint::traits::IsTypeOrDerived<FROM_EL, TO_EL>)))));
};

/// Specialization of 'CanReinterpret' for when TO and FROM are equal types.
template <typename T, ReinterpretMode MODE>
struct CanReinterpret<MODE, T, T> {
    /// Always `true` as TO and FROM are the same type.
    static constexpr bool value = true;
};

}  // namespace detail

/// Evaluates whether a view of `FROM` and be reinterpreted as a view of `TO`.
/// Views can be reinterpreted if:
///  * TO has the same or more 'constness' than FROM.
///  * And either:
///  * `FROM` and `TO` are pointers to the same type
///  * `FROM` and `TO` are pointers to CastableBase (or derived), and the pointee type of `TO` is of
///     the same type as, or is an ancestor of the pointee type of `FROM`.
template <ReinterpretMode MODE, typename TO, typename FROM>
static constexpr bool CanReinterpret = tint::detail::CanReinterpret<MODE, TO, FROM>::value;

/// Reinterprets a std::span<FROM> as a std::span<TO>.
/// @param span the span to reinterpret
/// @returns the reinterpreted span
template <typename TO, ReinterpretMode MODE = ReinterpretMode::kSafe, typename FROM>
std::span<TO> Reinterpret(std::span<FROM> span) {
    static_assert(CanReinterpret<MODE, TO, FROM>);
    return {Bitcast<TO*>(span.data()), span.size()};
}

}  // namespace tint

#endif  // SRC_TINT_UTILS_CONTAINERS_REINTERPRET_H_
