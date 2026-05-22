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

#ifndef SRC_UTILS_SPAN_H_
#define SRC_UTILS_SPAN_H_

#include "utils/numeric.h"
#include "utils/underlying_type.h"

namespace dawn {

namespace detail {

// XXX maybe only dynamic extent to start?

template <typename T, typename Extent, typename Index, typename PtrType>
class SpanBase {
  public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using pointer = T*;
    // size_type / difference_type missing
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    // [const_]reverse_iterator missing

    constexpr SpanBase() = default;
    DAWN_UNSAFE_BUFFER_USAGE constexpr SpanBase(PtrType* data, Index size)
        : mData(std::move(data)), mSize(checked_cast<size_t>(size)) {}

    // XXX safe constructor from something that's a random iterator?
    //  something that has a data() as a element_type*, a size() as Index, pointers can convert.
    //  Also something about borrowed range?

    // Copy and move.
    constexpr SpanBase(const SpanBase& other) noexcept = default;
    constexpr SpanBase(SpanBase&& other) noexcept = default;

    constexpr SpanBase& operator=(const SpanBase& other) noexcept = default;
    constexpr SpanBase& operator=(SpanBase&& other) noexcept = default;
    // We could add a conversion from compatible spans

    // Observers
    constexpr Index size() const noexcept {
        return Index(static_cast<UnderlyingType<Index>>(mSize));
    }

    constexpr size_t size_bytes() const noexcept { return mSize * sizeof(element_type); }

    [[nodiscard]] constexpr bool empty() const noexcept { return mSize == 0; }

    // Element access
    constexpr T& operator[](Index index) const noexcept
    // requires extent > 0
    {
        return at(index);
    }

    constexpr reference at(Index typedIndex) const
    //    requires(extent > 0)
    {
        size_t index = checked_cast<size_t>(typedIndex);
        // DAWN_CHECK(index < mSize);
        return DAWN_UNSAFE_BUFFERS(*(begin() + index));
    }

    constexpr reference front() const noexcept
    // requires Extent > 0
    {
        return at(0);
    }

    constexpr reference back() const noexcept
    // requires extent > 0
    {
        return at(size() - 1);
    }

    constexpr pointer data() const noexcept { return mData; }

    // Subviews
    // first / last / subspan missing

    // Iterators
    constexpr iterator begin() const noexcept { return static_cast<T*>(mData); }
    constexpr iterator end() const noexcept { return DAWN_UNSAFE_BUFFERS(begin() + mSize); }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }

    // r[c]begin/end missing.

  private:
    size_t mSize = 0;
    PtrType mData = {};
}

}  // namespace detail

}  // namespace dawn

#endif  // SRC_UTILS_SPAN_H_
