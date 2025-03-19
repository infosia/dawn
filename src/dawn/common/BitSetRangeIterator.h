// Copyright 2025 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_COMMON_BITSETRANGEITERATOR_H_
#define SRC_DAWN_COMMON_BITSETRANGEITERATOR_H_

#include <bitset>
#include <limits>
#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/Math.h"
#include "dawn/common/UnderlyingType.h"

namespace dawn {

// Similar to BitSetIterator but returns ranges of consecutive bits as (offset, size) pairs
template <size_t N, typename T>
class BitSetRangeIterator final {
  public:
    explicit BitSetRangeIterator(const std::bitset<N>& bitset);
    BitSetRangeIterator(const BitSetRangeIterator& other);
    BitSetRangeIterator& operator=(const BitSetRangeIterator& other);

    class Iterator final {
      public:
        explicit Iterator(const std::bitset<N>& bits);
        Iterator& operator++();

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

        // Returns a pair of offset and size of the current range
        std::pair<T, T> operator*() const {
            using U = UnderlyingType<T>;
            DAWN_ASSERT(static_cast<U>(mOffset) <= std::numeric_limits<U>::max());
            DAWN_ASSERT(static_cast<U>(mSize) <= std::numeric_limits<U>::max());
            return std::make_pair(static_cast<T>(static_cast<U>(mOffset)),
                                  static_cast<T>(static_cast<U>(mSize)));
        }

      private:
        std::pair<uint32_t, uint32_t> getNextRange();

        static constexpr size_t kBitsPerWord = sizeof(uint32_t) * 8;
        std::bitset<N> mBits;
        uint32_t mOffset{0};
        uint32_t mSize{0};
        uint32_t mWordOffset{0};
    };

    Iterator begin() const { return Iterator(mBits); }
    Iterator end() const { return Iterator(std::bitset<N>(0)); }

  private:
    const std::bitset<N> mBits;
};

template <size_t N, typename T>
BitSetRangeIterator<N, T>::BitSetRangeIterator(const std::bitset<N>& bitset) : mBits(bitset) {}

template <size_t N, typename T>
BitSetRangeIterator<N, T>::BitSetRangeIterator(const BitSetRangeIterator& other)
    : mBits(other.mBits) {}

template <size_t N, typename T>
BitSetRangeIterator<N, T>& BitSetRangeIterator<N, T>::operator=(const BitSetRangeIterator& other) {
    mBits = other.mBits;
    return *this;
}

template <size_t N, typename T>
BitSetRangeIterator<N, T>::Iterator::Iterator(const std::bitset<N>& bits) : mBits(bits) {
    if (bits.any()) {
        auto range = getNextRange();
        mOffset = range.first;
        mSize = range.second;
    } else {
        mWordOffset = static_cast<uint32_t>(roundUp(N, kBitsPerWord));
    }
}

template <size_t N, typename T>
typename BitSetRangeIterator<N, T>::Iterator& BitSetRangeIterator<N, T>::Iterator::operator++() {
    DAWN_ASSERT(mBits.any());
    // Clear the current range
    for (uint32_t i = mOffset; i < mOffset + mSize; ++i) {
        mBits.set(i - mWordOffset, 0);
    }
    auto range = getNextRange();
    mOffset = range.first;
    mSize = range.second;
    return *this;
}

template <size_t N, typename T>
bool BitSetRangeIterator<N, T>::Iterator::operator==(const Iterator& other) const {
    return mWordOffset == other.mWordOffset && mBits == other.mBits;
}

template <size_t N, typename T>
bool BitSetRangeIterator<N, T>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

template <size_t N, typename T>
std::pair<uint32_t, uint32_t> BitSetRangeIterator<N, T>::Iterator::getNextRange() {
    static std::bitset<N> wordMask(std::numeric_limits<uint32_t>::max());

    while (mWordOffset < N) {
        uint32_t wordBits = static_cast<uint32_t>((mBits & wordMask).to_ulong());
        if (wordBits != 0ul) {
            uint32_t offset = ScanForward(wordBits) + mWordOffset;
            uint32_t size = 1;
            uint32_t bitPosition = offset - mWordOffset;

            // Count consecutive bits in current word
            uint32_t remainingBits = wordBits >> bitPosition;
            remainingBits >>= 1;  // Skip the first bit since we counted it
            while ((bitPosition + size) < kBitsPerWord && (remainingBits & 1)) {
                size++;
                remainingBits >>= 1;
            }

            // If range might continue into next word
            uint32_t wordCount = 1;
            while ((bitPosition + size) % kBitsPerWord == 0 &&
                   (mWordOffset + wordCount * kBitsPerWord) < N) {
                std::bitset<N> nextWordBits = mBits >> (wordCount * kBitsPerWord);
                uint32_t nextWordRemainBits =
                    static_cast<uint32_t>((nextWordBits & wordMask).to_ulong());

                // Count consecutive bits in next word
                uint32_t nextBitPosition = 0;
                while (nextBitPosition < kBitsPerWord &&
                       (mWordOffset + wordCount * kBitsPerWord + nextBitPosition) < N &&
                       (nextWordRemainBits & 1u)) {
                    size++;
                    nextBitPosition++;
                    nextWordRemainBits >>= 1;
                }
                wordCount++;
            }

            return std::make_pair(offset, size);
        }

        mBits >>= kBitsPerWord;
        mWordOffset += kBitsPerWord;
    }
    return std::make_pair(0, 0);
}

// Helper to avoid needing to specify the template parameter size
template <size_t N>
BitSetRangeIterator<N, uint32_t> IterateBitSetRanges(const std::bitset<N>& bitset) {
    return BitSetRangeIterator<N, uint32_t>(bitset);
}

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_BITSETRANGEITERATOR_H_
