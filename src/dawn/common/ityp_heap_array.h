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
#include "dawn/common/Numeric.h"
#include "dawn/common/UnderlyingType.h"
#include "dawn/common/ityp_span.h"

namespace dawn::ityp {

// "Lite", `ityp` version of base::HeapArray from Chromium. It's like a safer, sized
// std::unique_ptr<T[]>, and it has a nicer (std::span-like / std::ranges-compatible) interface.
// To convert to an actual span, use `std::span{heapArray}`, which constructs it via std::ranges.
//
// Note that just like new[], it's invalid to reinterpret to another type UNLESS the element type is
// char or std::byte AND the target type's alignment is <= alignof(std::max_align_t).
template <typename Index, typename Value>
class heap_array :
    // Could be copyable, if needed, but we haven't needed it.
    public NonCopyable {
    using I = UnderlyingType<Index>;
    using Span = ityp::span<Index, Value>;

    // Some code below assumes the Index type is unsigned. It could be generalized.
    static_assert(UnsignedUnderlyingType<Index>, "Index type must be unsigned");

  public:
    // Constructs an empty heap_array. (Note it cannot be resized.)
    constexpr heap_array() = default;

    constexpr heap_array(heap_array<Index, Value>&& other) { *this = std::move(other); }
    constexpr heap_array<Index, Value>& operator=(heap_array<Index, Value>&& other) {
        mOwnedData = other.mOwnedData;
        other.mOwnedData = {};
        return *this;
    }

    constexpr ~heap_array() {
        if (mOwnedData.data()) {
            delete mOwnedData.data();
            mOwnedData = {};
        }
    }

    // Constructs a zero-initialized heap_array with count `count`.
    constexpr explicit heap_array(Index count) : heap_array{Alloc(count, true), count} {
        // Even if count is 0, the new[] shouldn't have returned nullptr.
        DAWN_ASSERT(mOwnedData.data() != nullptr);
    }
    // Constructs a zero-initialized heap_array with count `count`.
    // If allocation fails, returns a falsy object, with .data() == nullptr and .size() == 0.
    constexpr heap_array(Index count, std::nothrow_t)
        : heap_array{AllocNoThrow(count, true), count} {
        if (mOwnedData.data() == nullptr) {
            DAWN_ASSERT(mOwnedData.size() == Index{});
        }
    }

    // Constructs a uninitialized heap_array with count `count`.
    // NOTE: Usages of this method should include a SAFETY comment about why it's OK to use.
    [[nodiscard]] static constexpr heap_array<Index, Value> Uninit(Index count)
        // This can only be used with POD types, others are always initialized.
        requires(std::is_trivially_constructible_v<Value> &&
                 std::is_trivially_destructible_v<Value>)
    {
        return heap_array<Index, Value>{Alloc(count, false), count};
    }
    // Constructs a uninitialized heap_array with count `count`, or count 0 if allocation fails.
    // NOTE: Usages of this method should include a SAFETY comment about why it's OK to use.
    [[nodiscard]] static constexpr heap_array<Index, Value> Uninit(Index count, std::nothrow_t)
        // This can only be used with POD types, others are always initialized.
        requires(std::is_trivially_constructible_v<Value> &&
                 std::is_trivially_destructible_v<Value>)
    {
        return heap_array<Index, Value>{AllocNoThrow(count, false), count};
    }

    // Acquire the contents as a raw pointer and size (kind of like what you get from new[] - it's
    // valid to delete using `delete[]`).
    // Useful with std::tie when returning a size+ptr array (e.g. via the webgpu.h API).
    constexpr std::pair<size_t, Value*> AcquireRaw() {
        auto data = mOwnedData;
        mOwnedData = {};
        return {checked_cast<size_t>(data.size()), data.data()};
    }

    // Returns true if the allocation succeded. This can be used like `if (myHeapArray) {}` to check
    // if nothrow allocation succeded. Note, even if the size is 0, this may return true.
    constexpr explicit operator bool() { return mOwnedData.data() != nullptr; }

    // span-like interface
    constexpr auto data() { return mOwnedData.data(); }
    constexpr auto data() const { return mOwnedData.data(); }
    constexpr auto size() const { return mOwnedData.size(); }
    constexpr auto begin() { return mOwnedData.begin(); }
    constexpr auto begin() const { return mOwnedData.begin(); }
    constexpr auto end() { return mOwnedData.end(); }
    constexpr auto end() const { return mOwnedData.end(); }
    constexpr auto subspan(Index offset) { return mOwnedData.subspan(offset); }
    constexpr auto subspan(Index offset) const { return mOwnedData.subspan(offset); }
    constexpr auto subspan(Index offset, Index count) { return mOwnedData.subspan(offset, count); }
    constexpr auto subspan(Index offset, Index count) const {
        return mOwnedData.subspan(offset, count);
    }

    constexpr Span::reference front() { return mOwnedData.front(); }
    constexpr Span::const_reference front() const { return mOwnedData.front(); }
    constexpr Span::reference back() { return mOwnedData.back(); }
    constexpr Span::const_reference back() const { return mOwnedData.back(); }
    constexpr Span::reference operator[](Index i) { return mOwnedData[i]; }
    constexpr Span::const_reference operator[](Index i) const { return mOwnedData[i]; }

  private:
    // Constructs a heap_array from an existing allocation. If the allocation is null (AllocNoThrow
    // failed), the count is replaced with 0.
    constexpr heap_array(Value* data, Index count)
        : mOwnedData{data ? Span{data, count} : Span{}} {}

    static constexpr Value* Alloc(Index count, bool defaultInitialize) {
        return defaultInitialize ? new Value[checked_cast<size_t>(count)]{}
                                 : new Value[checked_cast<size_t>(count)];
    }

    static constexpr Value* AllocNoThrow(Index count, bool defaultInitialize) {
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
        // std::nothrow isn't implemented in sanitizers and they often have a 2GB allocation
        // limit. Catch large allocations and error out so fuzzers make progress.
        constexpr size_t kLargestAllowedAllocationAttemptBytes = 0x70000000;
#else
        constexpr size_t kLargestAllowedAllocationAttemptBytes =
            std::numeric_limits<size_t>::max() - 4095;
#endif

        // Early-fail to cover two cases:
        // - The checked_cast is going to fail.
        // - The total allocation size is going to be too close to the whole address space.
        //   PartitionAlloc in particular crashes instead of failing if (size >= SIZE_MAX - 23).
        if (I{count} > kLargestAllowedAllocationAttemptBytes / sizeof(Value)) {
            return nullptr;
        }

        return defaultInitialize ? new (std::nothrow) Value[checked_cast<size_t>(count)]{}
                                 : new (std::nothrow) Value[checked_cast<size_t>(count)];
    }

    // We store this as a span, but we own its allocation.
    // {nullptr, 0} = failed to allocate. (operator bool() returns false)
    // {non-null, size} = succeeded in allocating, even if size==0. (operator bool() returns true)
    Span mOwnedData;
};

// Construct a heap_array by copying its data from a std::ranges::range type.
template <typename Index>
static constexpr auto HeapArrayFrom(const std::ranges::sized_range auto& src) {
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
