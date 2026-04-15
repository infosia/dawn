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

#ifndef SRC_DAWN_COMMON_ITYP_HEAP_ARRAY_H_
#define SRC_DAWN_COMMON_ITYP_HEAP_ARRAY_H_

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ranges>
#include <utility>

#include "dawn/common/NonCopyable.h"
#include "dawn/common/TypedInteger.h"
#include "dawn/common/ityp_span.h"

namespace dawn::ityp {

// "Lite", `ityp` version of base::HeapArray from Chromium. It's like a safer, sized
// std::unique_ptr<T[]>, and it has a nicer (std::span-like / std::ranges-compatible) interface.
// To convert to an actual span, use std::span{xs} which constructs it via std::ranges.
// The allocation will always be aligned to at least std::max_align_t.
template <typename Index, typename Value>
class heap_array :
    // Could be copyable, if needed, but we haven't needed it.
    public NonCopyable {
    using I = UnderlyingType<Index>;

    static_assert(std::numeric_limits<I>::min() == 0, "Index type must be unsigned");

  public:
    // Constructs an empty heap_array. (Note it cannot be resized.)
    heap_array() = default;

    heap_array(heap_array<Index, Value>&& other) { *this = std::move(other); }
    heap_array<Index, Value>& operator=(heap_array<Index, Value>&& other) {
        mOwnedData = other.mOwnedData;
        other.mOwnedData = {};
        return *this;
    }

    ~heap_array() {
        if (mOwnedData.data()) {
            delete mOwnedData.data();
            mOwnedData = {};
        }
    }

    // Constructs a zero-initialized heap_array with count `count`.
    explicit heap_array(Index count) : heap_array{Alloc(count, true), count} {
        // Even if count is 0, the allocation shouldn't be nullptr.
        DAWN_ASSERT(mOwnedData.data() != nullptr);
        std::fill(mOwnedData.begin(), mOwnedData.end(), Value{});
    }
    // Constructs a zero-initialized heap_array with count `count`.
    // If allocation fails, returns a falsy object, with .data() == nullptr and .size() == 0.
    heap_array(Index count, std::nothrow_t) : heap_array{AllocNoThrow(count, true), count} {
        if (mOwnedData.data() == nullptr) {
            DAWN_ASSERT(mOwnedData.size() == Index{});
        }
        std::fill(mOwnedData.begin(), mOwnedData.end(), Value{});
    }

    // Constructs a uninitialized heap_array with count `count`.
    // NOTE: Usages of this method should include a SAFETY comment about why it's OK to use.
    [[nodiscard]] static heap_array<Index, Value> Uninit(Index count)
        // This can only be used with POD types, others are always initialized.
        requires(std::is_trivially_constructible_v<Value> &&
                 std::is_trivially_destructible_v<Value>)
    {
        return heap_array<Index, Value>{Alloc(count, false), count};
    }
    // Constructs a uninitialized heap_array with count `count`, or count 0 if allocation fails.
    // NOTE: Usages of this method should include a SAFETY comment about why it's OK to use.
    [[nodiscard]] static heap_array<Index, Value> Uninit(Index count, std::nothrow_t)
        // This can only be used with POD types, others are always initialized.
        requires(std::is_trivially_constructible_v<Value> &&
                 std::is_trivially_destructible_v<Value>)
    {
        return heap_array<Index, Value>{AllocNoThrow(count, false), count};
    }

    // Acquire the contents as a raw pointer and size (kind of like what you get from new[] - it's
    // valid to delete using `delete[]`).
    // Useful with std::tie when returning a size+ptr array (e.g. via the webgpu.h API).
    std::pair<size_t, Value*> AcquireRaw() {
        auto data = mOwnedData;
        mOwnedData = {};
        return {CheckedSize(data.size()), data.data()};
    }

    // Explicit cast to bool to use with `if (heap_array)` to check if nothrow allocation succeeded.
    // Note, even if the size is 0, this may be true.
    explicit operator bool() { return mOwnedData.data() != nullptr; }

    // span-like interface
    auto data() { return mOwnedData.data(); }
    auto data() const { return mOwnedData.data(); }
    auto size() const { return mOwnedData.size(); }
    auto begin() { return mOwnedData.begin(); }
    auto begin() const { return mOwnedData.begin(); }
    auto end() { return mOwnedData.end(); }
    auto end() const { return mOwnedData.end(); }
    auto subspan(Index offset) { return mOwnedData.subspan(offset); }
    auto subspan(Index offset) const { return mOwnedData.subspan(offset); }
    auto subspan(Index offset, Index count) { return mOwnedData.subspan(offset, count); }
    auto subspan(Index offset, Index count) const { return mOwnedData.subspan(offset, count); }

    Value& front() { return mOwnedData.front(); }
    const Value& front() const { return mOwnedData.front(); }
    Value& back() { return mOwnedData.back(); }
    const Value& back() const { return mOwnedData.back(); }
    Value& operator[](Index i) { return mOwnedData[i]; }
    const Value& operator[](Index i) const { return mOwnedData[i]; }

  private:
    // Constructs a heap_array from an existing allocation. If the allocation is null (AllocNoThrow
    // failed), the count is replaced with 0.
    heap_array(Value* data, Index count)
        : mOwnedData{data ? ityp::span{data, count} : ityp::span<Index, Value>{}} {}

    static constexpr std::align_val_t kAlignment{
        std::max(alignof(Value), alignof(std::max_align_t))};

    static Value* Alloc(Index count, bool defaultInitialize) {
        return defaultInitialize ? new (kAlignment) Value[CheckedSize(count)]{}
                                 : new (kAlignment) Value[CheckedSize(count)];
    }

    static Value* AllocNoThrow(Index count, bool defaultInitialize) {
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
        if (size_t{count} >= 0x70000000 / sizeof(Value)) {
            // std::nothrow isn't implemented in sanitizers and they often have a 2GB allocation
            // limit. Catch large allocations and error out so fuzzers make progress.
            return nullptr;
        }
#endif
        return defaultInitialize ? new (kAlignment, std::nothrow) Value[CheckedSize(count)]{}
                                 : new (kAlignment, std::nothrow) Value[CheckedSize(count)];
    }

    static constexpr inline size_t CheckedSize(Index size) {
        return checked_cast<size_t>(static_cast<I>(size));
    }

    // We store this as a span, but we own its allocation.
    // {nullptr, 0} = failed to allocate. (operator bool() returns false)
    // {non-null, size} = succeeded in allocating, even if size==0. (operator bool() returns true)
    ityp::span<Index, Value> mOwnedData;
};

// Construct a heap_array by copying its data from a std::ranges::range type.
template <typename Index>
static auto HeapArrayFrom(const std::ranges::sized_range auto& src) {
    using T = std::ranges::range_value_t<decltype(src)>;
    size_t size = std::ranges::size(src);
    DAWN_CHECK(uint64_t{size} <= uint64_t{std::numeric_limits<Index>::max()});
    // SAFETY: Initialized by the copy.
    auto result = heap_array<Index, T>::Uninit(
        Index{static_cast<UnderlyingType<Index>>(std::ranges::size(src))});
    std::ranges::copy(src, result.begin());
    return result;
}

}  // namespace dawn::ityp

#endif  // SRC_DAWN_COMMON_ITYP_HEAP_ARRAY_H_
