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

#include <concepts>
#include <memory>

#include "src/utils/numeric.h"
#include "src/utils/underlying_type.h"

namespace dawn {

template <typename Index>
inline constexpr Index DynamicExtent = std::numeric_limits<Index>::max();

namespace detail {

// XXX add documentation for every method.

template <typename From, typename To>
concept LegalDataConversion = std::is_convertible_v<From (*)[], To (*)[]>;

template <typename T, typename It>
concept CompatibleIter = std::contiguous_iterator<It> &&
                         LegalDataConversion<std::remove_reference_t<std::iter_reference_t<It>>, T>;

template <typename T, typename Index, typename R>
concept CompatibleRange = requires(R r) {
    { r.data() } -> std::convertible_to<T*>;
    { r.size() } -> std::same_as<Index>;
};

// DIFF: only dynamic_extent is supported at the moment because Dawn might not need spans with
// static extents.
// DIFF: size_t in function signatures is replaced by Index to support typed integers as the index
// type. The only exception is size_bytes().
template <typename T, typename Index, typename PtrType>
class SpanBase {
  private:
    using Self = SpanBase<T, Index, PtrType>;

  public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using pointer = T*;
    // DIFF: size_type / difference_type missing because it's not clear what they should be, Index
    // or size_t?
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    // DIFF: [const_]reverse_iterator missing to avoid including <iterator> until it is needed.

    static inline constexpr Index extent = DynamicExtent<Index>;

    // Constructors
    constexpr SpanBase() noexcept = default;

    template <typename It>
        requires CompatibleIter<element_type, It>
    DAWN_UNSAFE_BUFFER_USAGE constexpr SpanBase(It first, Index count)
        : mData(std::to_address(first)), mSize(checked_cast<size_t>(count)) {
        DAWN_CHECK(mSize != DynamicExtent<Index>);
    }

    template <typename It, typename End>
        requires(CompatibleIter<element_type, It> && std::sized_sentinel_for<End, It> &&
                 !std::is_convertible_v<End, size_t>)
    DAWN_UNSAFE_BUFFER_USAGE constexpr SpanBase(It first, End last)
        : mData(std::to_address(first)), mSize(last - first) {
        DAWN_CHECK(first <= last);
        DAWN_CHECK(mSize <= DynamicExtent<Index>);
    }

    // Constructor from a "range-like" that provides `T* data()` and `Index size()`. Note that this
    // is quite different from Chromium's base::span constructor from ranges because adapting that
    // constructor to support TypedInteger for Index would be extremely challenging.
    template <typename R>
        requires CompatibleRange<T, Index, R>
    constexpr SpanBase(R& range) : mSize(range.size()), mData(range.data()) {}
    template <typename R>
        requires CompatibleRange<const T, Index, R>
    constexpr SpanBase(const R& range) : mSize(range.size()), mData(range.data()) {}

    // Copy and move.
    constexpr SpanBase(const SpanBase& other) noexcept = default;
    constexpr SpanBase(SpanBase&& other) noexcept = default;

    constexpr SpanBase& operator=(const SpanBase& other) noexcept = default;
    constexpr SpanBase& operator=(SpanBase&& other) noexcept = default;

    // Iterators
    constexpr iterator begin() const noexcept { return static_cast<T*>(mData); }
    constexpr iterator end() const noexcept { return DAWN_UNSAFE_BUFFERS(begin() + mSize); }

    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr const_iterator cend() const noexcept { return end(); }

    // DIFF: r[c]begin/end missing to avoid including <iterator> until it is needed.

    // Element access
    constexpr reference front() const noexcept { return at(0); }

    constexpr reference back() const noexcept { return at(size() - Index{1u}); }

    constexpr reference at(Index typedIndex) const {
        size_t index = checked_cast<size_t>(typedIndex);
        DAWN_CHECK(index < size());
        // SAFETY: data() points at at least size() elements, so the DAWN_CHECK ensures that the
        // resulting reference is in the range.
        return DAWN_UNSAFE_BUFFERS(*(data() + index));
    }

    constexpr T& operator[](Index index) const noexcept { return at(index); }

    constexpr pointer data() const noexcept { return mData; }

    // Observers
    constexpr Index size() const noexcept {
        return Index(static_cast<UnderlyingType<Index>>(mSize));
    }

    constexpr size_t size_bytes() const noexcept { return mSize * sizeof(element_type); }

    [[nodiscard]] constexpr bool empty() const noexcept { return mSize == 0; }

    // Subviews
    // DIFF: subviews with template Offset / Count missing.

    constexpr Self first(Index count) const {
        DAWN_CHECK(count <= size());
        // SAFETY: data() points at at least size() elements, so the DAWN_CHECK ensure that the
        // result span is a subset of this.
        return DAWN_UNSAFE_BUFFERS(Self(data(), count));
    }

    constexpr Self last(Index count) const {
        DAWN_CHECK(count <= size());
        // SAFETY: data() points at at least size() elements, so the DAWN_CHECK ensure that the
        // result span is a subset of this.
        return DAWN_UNSAFE_BUFFERS(Self(data() + (size() - count), count));
    }

    // DIFF: dynamic_extent is not supported in the 2-arg version of subspan(), the single argument
    // overload must be used instead.
    constexpr Self subspan(Index offset) const {
        DAWN_CHECK(offset <= size());
        Index remainingSize = size() - offset;
        // SAFETY: data() points at at least size() elements, so the DAWN_CHECK ensure that the
        // result span is a subset of this.
        return DAWN_UNSAFE_BUFFERS(Self(data() + offset, remainingSize));
    }
    constexpr Self subspan(Index offset, Index count) const {
        DAWN_CHECK(offset <= size());
        DAWN_CHECK(count <= size() - offset);
        // SAFETY: data() points at at least size() elements, so the DAWN_CHECKs ensure that the
        // result span is a subset of this.
        return DAWN_UNSAFE_BUFFERS(Self(data() + offset, count));
    }

    // Additions not part of std::span.

    // None for now, but consider adding the following like in Chromium's base::span:
    //
    //  - constructor from (&T)[N]
    //  - operator ==, operator <=>
    //  - copy_from, copy_prefix_from
    //  - split_at, take_first, take_first_elem
    //  - get_at
    //  - to_fixed_extent

  private:
    size_t mSize = 0;
    PtrType mData = {};
};

}  // namespace detail

}  // namespace dawn

#endif  // SRC_UTILS_SPAN_H_
