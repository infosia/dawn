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

#ifndef SRC_TINT_UTILS_MEMORY_COPY_H_
#define SRC_TINT_UTILS_MEMORY_COPY_H_

#include <cstring>
#include <span>

#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/macros/compiler.h"

// These wrappers around memcpy exist to localize the UNSAFE_BUFFER_USAGE warning intrinsic to the
// operation to one location. These warnings could be eliminated by using iteration to copy, but
// that would be less performant. std::ranges::copy does not guarantee bounds checking, so the
// warning still fires on some platforms. Chromium's //base has a more sophisticated implementation
// of this in the form of ::copy_from, but that is not available in standalone Dawn builds.
//
// It is intentional that the raw pointer -> std::span copy is not implemented, since that would
// involve crossing the boundary between unsafe -> safe memory operations. It is preferable for the
// call site where this is occurring be explicitly marked as unsafe, instead of trying to hide the
// warning. The idiomatic way to do this is to construct a std::span using the two-part constructor,
// which will need a warning suppression. And then call the std::span -> std::span version of Copy.

namespace tint {

/// Copy copies all elements from @p src to @p dst.
/// @param dst the destination span
/// @param src the source span
/// @tparam T the type of destination elements
/// @tparam N the size of the destination span
/// @tparam U the type of source elements
/// @tparam M the size of the source span
// The usage of std::memcpy will always trigger this warning
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
template <typename T, size_t N, typename U, size_t M>
inline void Copy(std::span<T, N> dst, std::span<U, M> src)
    requires(N >= M)
{
    static_assert(sizeof(T) == sizeof(U));
    TINT_ASSERT(src.data());
    TINT_ASSERT(dst.data());
    if (!src.empty()) {
        std::memcpy(dst.data(), src.data(), src.size_bytes());
    }
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

/// Copy copies all elements from @p src to @p dst.
/// @param dst the destination pointer
/// @param dst_count the maximum number of elements that can be written to @p dst
/// @param src the source span
/// @tparam T the type of destination elements
/// @tparam U the type of source elements
/// @tparam M the size of the source span
// The usage of std::memcpy will always trigger this warning
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
template <typename T, typename U, size_t M>
inline void Copy(T* dst, size_t dst_count, std::span<U, M> src) {
    static_assert(sizeof(T) == sizeof(U));
    TINT_ASSERT(src.data());
    TINT_ASSERT(dst);
    TINT_ASSERT(dst_count >= src.size());
    if (!src.empty()) {
        std::memcpy(dst, src.data(), src.size_bytes());
    }
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

// template <typename T, typename U, size_t M>
// inline void Copy(std::span<T, M> dest, T* src, size_t src_count)
// is intentionally omitted, see above

}  // namespace tint

#endif  // SRC_TINT_UTILS_MEMORY_COPY_H_
