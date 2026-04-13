// Copyright 2020 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_COMMON_ALLOC_H_
#define SRC_DAWN_COMMON_ALLOC_H_

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <utility>

#include "dawn/common/NonCopyable.h"

namespace dawn {

// Lite version of base::HeapArray from Chromium
template <typename T>
class HeapArray : public NonCopyable {
  public:
    HeapArray() = default;

    explicit HeapArray(size_t count) : HeapArray{new T[count], count} {
        std::fill(mOwnedData.begin(), mOwnedData.end(), 0);
    }
    HeapArray(size_t count, std::nothrow_t) : HeapArray{AllocNoThrow(count), count} {
        std::fill(mOwnedData.begin(), mOwnedData.end(), 0);
    }

    static HeapArray<T> Uninit(size_t count) {  //
        return HeapArray<T>{new T[count], count};
    }
    static HeapArray<T> Uninit(size_t count, std::nothrow_t) {
        return HeapArray<T>{AllocNoThrow(count), count};
    }
    static HeapArray<T> CopiedFrom(std::span<const T> src) {
        auto result = HeapArray<T>::Uninit(src.size());
        std::copy(src.begin(), src.end(), result.data());
        return result;
    }

    HeapArray(HeapArray<T>&& other) { *this = std::move(other); }
    HeapArray<T>& operator=(HeapArray<T>&& other) {
        mOwnedData = other.mOwnedData;
        other.mOwnedData = {};
        return *this;
    }

    ~HeapArray() {
        if (mOwnedData.data()) {
            delete mOwnedData.data();
            mOwnedData = {};
        }
    }

    operator std::span<T>() { return mOwnedData; }
    explicit operator bool() { return !mOwnedData.empty(); }

    std::span<T> as_span() { return mOwnedData; }
    std::span<const T> as_span() const { return mOwnedData; }

    // span-like interface
    T* data() { return mOwnedData.data(); }
    const T* data() const { return mOwnedData.data(); }
    T* end() { return mOwnedData.data() + mOwnedData.size(); }
    const T* end() const { return mOwnedData.data() + mOwnedData.size(); }
    std::span<T> subspan(size_t offset) { return mOwnedData.subspan(offset); }
    std::span<const T> subspan(size_t offset) const { return mOwnedData.subspan(offset); }
    std::span<T> subspan(size_t offset, size_t count) { return mOwnedData.subspan(offset, count); }
    std::span<const T> subspan(size_t offset, size_t count) const {
        return mOwnedData.subspan(offset, count);
    }
    T& operator[](size_t i) { return mOwnedData[i]; }
    const T& operator[](size_t i) const { return mOwnedData[i]; }

  private:
    HeapArray(T* data, size_t count) : mOwnedData{data ? std::span{data, count} : std::span<T>{}} {}

    static T* AllocNoThrow(size_t count) {
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
        if (count * sizeof(T) >= 0x70000000) {
            // std::nothrow isn't implemented in sanitizers and they often have a 2GB allocation
            // limit. Catch large allocations and error out so fuzzers make progress.
            return nullptr;
        }
#endif
        return new (std::nothrow) T[count];
    }

    std::span<T> mOwnedData;
};

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_ALLOC_H_
