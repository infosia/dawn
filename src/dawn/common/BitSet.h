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

#ifndef SRC_DAWN_COMMON_BITSET_H_
#define SRC_DAWN_COMMON_BITSET_H_

#include <stdint.h>

#include <algorithm>
#include <array>
#include <limits>

#include "dawn/common/Assert.h"
#include "dawn/common/Math.h"
#include "dawn/common/Platform.h"
#include "dawn/common/UnderlyingType.h"

namespace dawn {

// This is ANGLE's bitset_utils.h with whatever fixes were necessary to integrate with Dawn.
// Last updated to match ANGLE's latest changes in March 2025.
// TODO(crbug.com/42241557): to track ongoing optimization efforts.

// Given x, create 1 << x.
template <typename BitsT, typename ParamT>
constexpr BitsT Bit(ParamT x) {
    // It's undefined behavior if the shift size is equal to or larger than the width of the type.
    DAWN_ASSERT(static_cast<size_t>(x) < sizeof(BitsT) * 8);

    return (static_cast<BitsT>(1) << static_cast<size_t>(x));
}

// Given x, create (1 << x) - 1, i.e. a mask with x bits set.
template <typename BitsT, typename ParamT>
constexpr BitsT BitMask(ParamT x) {
    if (static_cast<size_t>(x) == 0) {
        return 0;
    }
    return ((Bit<BitsT>(static_cast<ParamT>(static_cast<size_t>(x) - 1)) - 1) << 1) | 1;
}

template <size_t N, typename BitsT, typename ParamT>
class BitSetT final {
  public:
    class Reference final {
      public:
        ~Reference() {}
        Reference& operator=(bool x) {
            mParent->set(mBit, x);
            return *this;
        }
        explicit operator bool() const { return mParent->test(mBit); }

      private:
        friend class BitSetT;

        Reference(BitSetT* parent, ParamT bit) : mParent(parent), mBit(bit) {}

        BitSetT* mParent;
        ParamT mBit;
    };

    class Iterator final {
      public:
        explicit Iterator(const BitSetT& bits);
        Iterator& operator++();

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        ParamT operator*() const;

        // These helper functions allow mutating an iterator in-flight.
        // They only operate on later bits to ensure we don't iterate the same bit twice.
        void resetLaterBit(std::size_t index) {
            DAWN_ASSERT(index > mCurrentBit);
            mBitsCopy.reset(index);
        }

        // bits could contain bit that earlier than mCurrentBit. Since mBitCopy can't have bits
        // earlier than mCurrentBit, the & operation will mask out earlier bits anyway.
        void resetLaterBits(const BitSetT& bits) {
            BitSetT maskedBits = ~Mask(mCurrentBit + 1);
            maskedBits &= bits;
            mBitsCopy &= ~maskedBits;
        }

        void setLaterBit(std::size_t index) {
            DAWN_ASSERT(index > mCurrentBit);
            mBitsCopy.set(index);
        }

        void setLaterBits(const BitSetT& bits) {
            DAWN_ASSERT((BitSetT(bits) &= Mask(mCurrentBit + 1)).none());
            mBitsCopy |= bits;
        }

      private:
        std::size_t getNextBit();

        BitSetT mBitsCopy;
        std::size_t mCurrentBit{0};
    };

    using value_type = BitsT;
    using param_type = ParamT;

    constexpr BitSetT();
    constexpr explicit BitSetT(BitsT value);
    constexpr explicit BitSetT(std::initializer_list<ParamT> init);

    constexpr bool operator==(const BitSetT& other) const;
    constexpr bool operator!=(const BitSetT& other) const;

    constexpr bool operator[](ParamT pos) const;
    Reference operator[](ParamT pos) { return Reference(this, pos); }

    constexpr bool test(ParamT pos) const;

    constexpr bool all() const;
    constexpr bool any() const;
    constexpr bool none() const;
    constexpr std::size_t count() const;

    // Returns true iff there are unset bits prior
    // to the most significant bit set. For example:
    // 0b0000 - false
    // 0b0001 - false
    // 0b0011 - false
    // 0b0010 - true
    // 0b0101 - true
    constexpr bool hasGaps() const;

    constexpr static std::size_t size() { return N; }

    constexpr BitSetT& operator&=(const BitSetT& other);
    constexpr BitSetT& operator|=(const BitSetT& other);
    constexpr BitSetT& operator^=(const BitSetT& other);
    constexpr BitSetT operator~() const;

    constexpr BitSetT& operator&=(BitsT value);
    constexpr BitSetT& operator|=(BitsT value);
    constexpr BitSetT& operator^=(BitsT value);

    constexpr BitSetT operator<<(std::size_t pos) const;
    constexpr BitSetT& operator<<=(std::size_t pos);
    constexpr BitSetT operator>>(std::size_t pos) const;
    constexpr BitSetT& operator>>=(std::size_t pos);

    constexpr BitSetT& set();
    constexpr BitSetT& set(ParamT pos, bool value = true);

    constexpr BitSetT& reset();
    constexpr BitSetT& reset(ParamT pos);

    constexpr BitSetT& flip();
    constexpr BitSetT& flip(ParamT pos);

    // NOLINTNEXTLINE(runtime/int)
    constexpr unsigned long to_ulong() const { return static_cast<unsigned long>(mBits); }
    constexpr BitsT bits() const { return mBits; }

    Iterator begin() const { return Iterator(*this); }
    Iterator end() const { return Iterator(BitSetT()); }

    constexpr static BitSetT Zero() { return BitSetT(); }

    constexpr ParamT first() const;
    constexpr ParamT last() const;

    // Produces a mask of ones up to the "x"th bit.
    constexpr static BitSetT Mask(std::size_t x) {
        BitSetT result;
        result.mBits = BitMask<BitsT>(static_cast<ParamT>(x));
        return result;
    }

  private:
    BitsT mBits;
};

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT() : mBits(0) {
    static_assert(N > 0, "Bitset type cannot support zero bits.");
    static_assert(N <= sizeof(BitsT) * 8, "Bitset type cannot support a size this large.");
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT(BitsT value) : mBits(value & Mask(N).bits()) {}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT(std::initializer_list<ParamT> init) : mBits(0) {
    for (ParamT element : init) {
        mBits |= Bit<BitsT>(element);
    }
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator==(const BitSetT& other) const {
    return mBits == other.mBits;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator!=(const BitSetT& other) const {
    return mBits != other.mBits;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator[](ParamT pos) const {
    return test(pos);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::test(ParamT pos) const {
    return (mBits & Bit<BitsT>(pos)) != 0;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::all() const {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return mBits == Mask(N).bits();
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::any() const {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return (mBits != 0);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::none() const {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return (mBits == 0);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr std::size_t BitSetT<N, BitsT, ParamT>::count() const {
    return BitCount(mBits);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::hasGaps() const {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return (mBits != Mask(N).bits()) && ((mBits & (mBits + 1)) != 0);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator&=(const BitSetT& other) {
    mBits &= other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator|=(const BitSetT& other) {
    mBits |= other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator^=(const BitSetT& other) {
    mBits = mBits ^ other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator~() const {
    return BitSetT<N, BitsT, ParamT>(~mBits & Mask(N).bits());
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator&=(BitsT value) {
    mBits &= value;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator|=(BitsT value) {
    mBits |= value;
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator^=(BitsT value) {
    mBits ^= value;
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator<<(std::size_t pos) const {
    return BitSetT<N, BitsT, ParamT>((mBits << pos) & Mask(N).bits());
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator<<=(std::size_t pos) {
    mBits = mBits << pos & Mask(N).bits();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator>>(std::size_t pos) const {
    return BitSetT<N, BitsT, ParamT>(mBits >> pos);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::operator>>=(std::size_t pos) {
    mBits = (mBits >> pos) & Mask(N).bits();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::set() {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    mBits = Mask(N).bits();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::set(ParamT pos, bool value) {
    DAWN_ASSERT(static_cast<size_t>(pos) < N);
    if (value) {
        mBits |= Bit<BitsT>(pos);
    } else {
        reset(pos);
    }
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::reset() {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    mBits = 0;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::reset(ParamT pos) {
    DAWN_ASSERT(static_cast<size_t>(pos) < N);
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    mBits &= ~Bit<BitsT>(pos);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::flip() {
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    mBits ^= Mask(N).bits();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>& BitSetT<N, BitsT, ParamT>::flip(ParamT pos) {
    DAWN_ASSERT(static_cast<size_t>(pos) < N);
    mBits ^= Bit<BitsT>(pos);
    DAWN_ASSERT(mBits == (mBits & Mask(N).bits()));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr ParamT BitSetT<N, BitsT, ParamT>::first() const {
    DAWN_ASSERT(!none());
    return static_cast<ParamT>(ScanForward(mBits));
}

template <size_t N, typename BitsT, typename ParamT>
constexpr ParamT BitSetT<N, BitsT, ParamT>::last() const {
    DAWN_ASSERT(!none());
    return static_cast<ParamT>(ScanReverse(mBits));
}

template <size_t N, typename BitsT, typename ParamT>
BitSetT<N, BitsT, ParamT>::Iterator::Iterator(const BitSetT& bits)
    : mBitsCopy(bits), mCurrentBit(0) {
    if (bits.any()) {
        mCurrentBit = getNextBit();
    }
}

template <size_t N, typename BitsT, typename ParamT>
inline typename BitSetT<N, BitsT, ParamT>::Iterator&
BitSetT<N, BitsT, ParamT>::Iterator::operator++() {
    DAWN_ASSERT(mBitsCopy.any());
    mBitsCopy.reset(static_cast<ParamT>(mCurrentBit));
    mCurrentBit = getNextBit();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
bool BitSetT<N, BitsT, ParamT>::Iterator::operator==(const Iterator& other) const {
    return mBitsCopy == other.mBitsCopy;
}

template <size_t N, typename BitsT, typename ParamT>
bool BitSetT<N, BitsT, ParamT>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

template <size_t N, typename BitsT, typename ParamT>
ParamT BitSetT<N, BitsT, ParamT>::Iterator::operator*() const {
    return static_cast<ParamT>(mCurrentBit);
}

template <size_t N, typename BitsT, typename ParamT>
std::size_t BitSetT<N, BitsT, ParamT>::Iterator::getNextBit() {
    if (mBitsCopy.none()) {
        return 0;
    }

    return ScanForward(mBitsCopy.mBits);
}

template <size_t N, typename ParamT = std::size_t>
using BitSet8 = BitSetT<N, uint8_t, ParamT>;

template <size_t N, typename ParamT = std::size_t>
using BitSet16 = BitSetT<N, uint16_t, ParamT>;

template <size_t N, typename ParamT = std::size_t>
using BitSet32 = BitSetT<N, uint32_t, ParamT>;
static_assert(std::is_trivially_copyable<BitSet32<32>>(), "must be memcpy-able");

template <size_t N, typename ParamT = std::size_t>
using BitSet64 = BitSetT<N, uint64_t, ParamT>;

template <std::size_t N, typename ParamT>
class BitSetArray;

namespace priv {

template <size_t N, typename T>
using EnableIfBitsFit = std::enable_if_t<N <= sizeof(T) * 8>;

template <size_t N, typename ParamT, typename Enable = void>
struct GetBitSet {
    using Type = BitSetArray<N, ParamT>;
};

// Prefer 64-bit bitsets on 64-bit CPUs. They seem faster than 32-bit.
#if DAWN_PLATFORM_IS(64_BIT)
template <size_t N, typename ParamT>
struct GetBitSet<N, ParamT, EnableIfBitsFit<N, uint64_t>> {
    using Type = BitSet64<N, ParamT>;
};
constexpr std::size_t kDefaultBitSetSize = 64;
using BaseBitSetType = BitSet64<kDefaultBitSetSize>;
#else
template <size_t N, typename ParamT>
struct GetBitSet<N, ParamT, EnableIfBitsFit<N, uint32_t>> {
    using Type = BitSet32<N, ParamT>;
};
constexpr std::size_t kDefaultBitSetSize = 32;
using BaseBitSetType = BitSet32<kDefaultBitSetSize>;
#endif  // DAWN_PLATFORM_IS(64_BIT)

}  // namespace priv

template <size_t N, typename ParamT = std::size_t>
using BitSet = typename priv::GetBitSet<N, ParamT>::Type;

template <std::size_t N, typename ParamT = std::size_t>
class BitSetArray final {
  public:
    using BaseBitSet = priv::BaseBitSetType;
    using value_type = BaseBitSet::value_type;
    using param_type = BaseBitSet::param_type;

    constexpr BitSetArray();
    constexpr explicit BitSetArray(uint64_t value);
    constexpr explicit BitSetArray(std::initializer_list<param_type> init);

    class Reference final {
      public:
        ~Reference() {}
        Reference& operator=(bool x) {
            mParent.set(mPosition, x);
            return *this;
        }
        explicit operator bool() const { return mParent.test(mPosition); }

      private:
        friend class BitSetArray;

        Reference(BitSetArray& parent, std::size_t pos) : mParent(parent), mPosition(pos) {}

        BitSetArray& mParent;
        std::size_t mPosition;
    };
    class Iterator final {
      public:
        Iterator(const BitSetArray<N, ParamT>& bitSetArray, std::size_t index);
        Iterator& operator++();
        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
        ParamT operator*() const;

        // These helper functions allow mutating an iterator in-flight.
        // They only operate on later bits to ensure we don't iterate the same bit twice.
        void resetLaterBit(std::size_t pos) {
            DAWN_ASSERT(pos > (mIndex * priv::kDefaultBitSetSize) + *mCurrentIterator);
            prepareCopy();
            mParentCopy.reset(pos);
            updateIteratorBit(pos, false);
        }

        void setLaterBit(std::size_t pos) {
            DAWN_ASSERT(pos > (mIndex * priv::kDefaultBitSetSize) + *mCurrentIterator);
            prepareCopy();
            mParentCopy.set(pos);
            updateIteratorBit(pos, true);
        }

        void setLaterBits(const BitSetArray& bits) {
            prepareCopy();
            mParentCopy |= bits;
            updateIteratorBits(bits);
        }

      private:
        void prepareCopy() {
            DAWN_ASSERT(mParent.mBaseBitSetArray[mIndex].end() ==
                        mParentCopy.mBaseBitSetArray[mIndex].end());
            if (mParentCopy.none()) {
                mParentCopy = mParent;
                mCurrentParent = &mParentCopy;
            }
        }

        void updateIteratorBit(std::size_t pos, bool setBit) {
            // Get the index and offset, update current iterator if within range
            size_t index = pos >> kShiftForDivision;
            size_t offset = pos & kDefaultBitSetSizeMinusOne;
            if (index == mIndex) {
                if (setBit) {
                    mCurrentIterator.setLaterBit(offset);
                } else {
                    mCurrentIterator.resetLaterBit(offset);
                }
            }
        }

        void updateIteratorBits(const BitSetArray& bits) {
            mCurrentIterator.setLaterBits(bits.mBaseBitSetArray[mIndex]);
        }

        // Problem -
        // We want to provide the fastest path possible for usecases that iterate though the bitset.
        //
        // Options -
        // 1) For non-mutating iterations the const ref <mParent> is set as mCurrentParent and only
        //    for usecases that need to mutate the bitset while iterating we perform a copy of
        //    <mParent> into <mParentCopy> and modify its bits accordingly.
        // 2) The alternate approach was to perform a copy all the time in the constructor
        //    irrespective of whether it was a mutating usecase or not.
        //
        // Experiment -
        // BitSetIteratorPerfTest was run on a Windows machine with Intel CPU and these were the
        // results -
        // 1) Copy only when necessary -
        //      RESULT BitSetIteratorPerf.wall_time:    run = 116.1067374961 ns
        //      RESULT BitSetIteratorPerf.trial_steps : run = 8416124 count
        //      RESULT BitSetIteratorPerf.total_steps : run = 16832251 count
        // 2) Copy always -
        //      RESULT BitSetIteratorPerf.wall_time:    run = 242.7446459439 ns
        //      RESULT BitSetIteratorPerf.trial_steps : run = 4171416 count
        //      RESULT BitSetIteratorPerf.total_steps : run = 8342834 count
        //
        // Resolution -
        // We settled on the copy only when necessary path.
        size_t mIndex;
        const BitSetArray& mParent;
        BitSetArray mParentCopy;
        const BitSetArray* mCurrentParent;
        typename BaseBitSet::Iterator mCurrentIterator;
    };

    constexpr static std::size_t size() { return N; }
    Iterator begin() const { return Iterator(*this, 0); }
    Iterator end() const { return Iterator(*this, kArraySize); }

    // NOLINTNEXTLINE(runtime/int)
    constexpr unsigned long to_ulong() const {
        // TODO(anglebug.com/42264163): Handle serializing more than kDefaultBitSetSize
        for (std::size_t index = 1; index < kArraySize; index++) {
            DAWN_ASSERT(mBaseBitSetArray[index].none());
        }
        // NOLINTNEXTLINE(runtime/int)
        return static_cast<unsigned long>(mBaseBitSetArray[0].to_ulong());
    }

    // Assignment operators
    constexpr BitSetArray& operator&=(const BitSetArray& other);
    constexpr BitSetArray& operator|=(const BitSetArray& other);
    constexpr BitSetArray& operator^=(const BitSetArray& other);

    // Bitwise operators
    constexpr BitSetArray<N, ParamT> operator&(const dawn::BitSetArray<N, ParamT>& other) const;
    constexpr BitSetArray<N, ParamT> operator|(const dawn::BitSetArray<N, ParamT>& other) const;
    constexpr BitSetArray<N, ParamT> operator^(const dawn::BitSetArray<N, ParamT>& other) const;

    // Relational operators
    constexpr bool operator==(const dawn::BitSetArray<N, ParamT>& other) const;
    constexpr bool operator!=(const dawn::BitSetArray<N, ParamT>& other) const;

    // Unary operators
    constexpr BitSetArray operator~() const;
    constexpr bool operator[](std::size_t pos) const;
    constexpr Reference operator[](std::size_t pos) {
        DAWN_ASSERT(pos < size());
        return Reference(*this, pos);
    }

    // Setter, getters and other helper methods
    constexpr BitSetArray& set();
    constexpr BitSetArray& set(std::size_t pos, bool value = true);
    constexpr BitSetArray& reset();
    constexpr BitSetArray& reset(std::size_t pos);
    constexpr bool test(std::size_t pos) const;
    constexpr bool all() const;
    constexpr bool any() const;
    constexpr bool none() const;
    constexpr std::size_t count() const;
    constexpr bool intersects(const BitSetArray& other) const;
    constexpr BitSetArray<N, ParamT>& flip();
    constexpr param_type first() const;
    constexpr param_type last() const;

    constexpr value_type bits(size_t index) const;

    // Produces a mask of ones up to the "x"th bit.
    constexpr static BitSetArray Mask(std::size_t x);

  private:
    static constexpr std::size_t kDefaultBitSetSizeMinusOne = priv::kDefaultBitSetSize - 1;
    static constexpr std::size_t kShiftForDivision = static_cast<std::size_t>(
        ConstexprLog2(static_cast<unsigned int>(priv::kDefaultBitSetSize)));
    static constexpr std::size_t kArraySize =
        ((N + kDefaultBitSetSizeMinusOne) >> kShiftForDivision);
    constexpr static std::size_t kLastElementCount = (N & kDefaultBitSetSizeMinusOne);
    constexpr static std::size_t kLastElementMask =
        priv::BaseBitSetType::Mask(kLastElementCount == 0 ? priv::kDefaultBitSetSize
                                                          : kLastElementCount)
            .bits();

    std::array<BaseBitSet, kArraySize> mBaseBitSetArray;
};
static_assert(std::is_trivially_copyable<BitSetArray<32>>(), "must be memcpy-able");

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>::BitSetArray() {
    static_assert(N > priv::kDefaultBitSetSize, "BitSetArray type can't support requested size.");
    reset();
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>::BitSetArray(uint64_t value) {
    reset();

    if (priv::kDefaultBitSetSize < 64) {
        size_t i = 0;
        for (; i < kArraySize - 1; ++i) {
            value_type elemValue =
                value & priv::BaseBitSetType::Mask(priv::kDefaultBitSetSize).bits();
            mBaseBitSetArray[i] = priv::BaseBitSetType(elemValue);
            value >>= priv::kDefaultBitSetSize;
        }
        value_type elemValue = value & kLastElementMask;
        mBaseBitSetArray[i] = priv::BaseBitSetType(elemValue);
    } else {
        value_type elemValue = value & priv::BaseBitSetType::Mask(priv::kDefaultBitSetSize).bits();
        mBaseBitSetArray[0] = priv::BaseBitSetType(elemValue);
    }
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>::BitSetArray(std::initializer_list<param_type> init) {
    reset();

    for (param_type element : init) {
        size_t index = element >> kShiftForDivision;
        size_t offset = element & kDefaultBitSetSizeMinusOne;
        mBaseBitSetArray[index].set(offset, true);
    }
}

template <size_t N, typename ParamT>
BitSetArray<N, ParamT>::Iterator::Iterator(const BitSetArray<N, ParamT>& bitSetArray,
                                           std::size_t index)
    : mIndex(index),
      mParent(bitSetArray),
      mCurrentParent(&mParent),
      mCurrentIterator(mParent.mBaseBitSetArray[0].begin()) {
    while (mIndex < mCurrentParent->kArraySize) {
        if (mCurrentParent->mBaseBitSetArray[mIndex].any()) {
            break;
        }
        mIndex++;
    }

    if (mIndex < mCurrentParent->kArraySize) {
        mCurrentIterator = mCurrentParent->mBaseBitSetArray[mIndex].begin();
    } else {
        mCurrentIterator = mCurrentParent->mBaseBitSetArray[mCurrentParent->kArraySize - 1].end();
    }
}

template <std::size_t N, typename ParamT>
typename BitSetArray<N, ParamT>::Iterator& BitSetArray<N, ParamT>::Iterator::operator++() {
    ++mCurrentIterator;
    while (mCurrentIterator == mCurrentParent->mBaseBitSetArray[mIndex].end()) {
        mIndex++;
        if (mIndex >= mCurrentParent->kArraySize) {
            break;
        }
        mCurrentIterator = mCurrentParent->mBaseBitSetArray[mIndex].begin();
    }
    return *this;
}

template <std::size_t N, typename ParamT>
bool BitSetArray<N, ParamT>::Iterator::operator==(
    const BitSetArray<N, ParamT>::Iterator& other) const {
    return mCurrentIterator == other.mCurrentIterator;
}

template <std::size_t N, typename ParamT>
bool BitSetArray<N, ParamT>::Iterator::operator!=(
    const BitSetArray<N, ParamT>::Iterator& other) const {
    return mCurrentIterator != other.mCurrentIterator;
}

template <std::size_t N, typename ParamT>
ParamT BitSetArray<N, ParamT>::Iterator::operator*() const {
    return (mIndex * priv::kDefaultBitSetSize) + *mCurrentIterator;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::operator&=(
    const BitSetArray<N, ParamT>& other) {
    for (std::size_t index = 0; index < kArraySize; index++) {
        mBaseBitSetArray[index] &= other.mBaseBitSetArray[index];
    }
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::operator|=(
    const BitSetArray<N, ParamT>& other) {
    for (std::size_t index = 0; index < kArraySize; index++) {
        mBaseBitSetArray[index] |= other.mBaseBitSetArray[index];
    }
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::operator^=(
    const BitSetArray<N, ParamT>& other) {
    for (std::size_t index = 0; index < kArraySize; index++) {
        mBaseBitSetArray[index] ^= other.mBaseBitSetArray[index];
    }
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT> BitSetArray<N, ParamT>::operator&(
    const dawn::BitSetArray<N, ParamT>& other) const {
    dawn::BitSetArray<N, ParamT> result(other);
    result &= *this;
    return result;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT> BitSetArray<N, ParamT>::operator|(
    const dawn::BitSetArray<N, ParamT>& other) const {
    dawn::BitSetArray<N, ParamT> result(other);
    result |= *this;
    return result;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT> BitSetArray<N, ParamT>::operator^(
    const dawn::BitSetArray<N, ParamT>& other) const {
    dawn::BitSetArray<N, ParamT> result(other);
    result ^= *this;
    return result;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::operator==(const dawn::BitSetArray<N, ParamT>& other) const {
    for (std::size_t index = 0; index < kArraySize; index++) {
        if (mBaseBitSetArray[index] != other.mBaseBitSetArray[index]) {
            return false;
        }
    }
    return true;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::operator!=(const dawn::BitSetArray<N, ParamT>& other) const {
    return !(*this == other);
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT> BitSetArray<N, ParamT>::operator~() const {
    dawn::BitSetArray<N, ParamT> result;
    for (std::size_t index = 0; index < kArraySize; index++) {
        result.mBaseBitSetArray[index] |= ~mBaseBitSetArray[index];
    }
    // The last element needs to keep extra bits set to 0
    result.mBaseBitSetArray[kArraySize - 1] &= kLastElementMask;

    return result;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::operator[](std::size_t pos) const {
    DAWN_ASSERT(pos < size());
    return test(pos);
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::set() {
    for (BaseBitSet& baseBitSet : mBaseBitSetArray) {
        baseBitSet.set();
    }
    // The last element in mBaseBitSetArray may need special handling
    mBaseBitSetArray[kArraySize - 1] &= kLastElementMask;

    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::set(std::size_t pos, bool value) {
    DAWN_ASSERT(pos < size());
    // Get the index and offset, then set the bit
    size_t index = pos >> kShiftForDivision;
    size_t offset = pos & kDefaultBitSetSizeMinusOne;
    mBaseBitSetArray[index].set(offset, value);
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::reset() {
    for (BaseBitSet& baseBitSet : mBaseBitSetArray) {
        baseBitSet.reset();
    }
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::reset(std::size_t pos) {
    DAWN_ASSERT(pos < size());
    return set(pos, false);
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::test(std::size_t pos) const {
    DAWN_ASSERT(pos < size());
    // Get the index and offset, then test the bit
    size_t index = pos >> kShiftForDivision;
    size_t offset = pos & kDefaultBitSetSizeMinusOne;
    return mBaseBitSetArray[index].test(offset);
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::all() const {
    constexpr priv::BaseBitSetType kLastElementBitSet = priv::BaseBitSetType(kLastElementMask);

    for (std::size_t index = 0; index < kArraySize - 1; index++) {
        if (!mBaseBitSetArray[index].all()) {
            return false;
        }
    }

    // The last element in mBaseBitSetArray needs to keep extra bits set to 0
    return mBaseBitSetArray[kArraySize - 1] == kLastElementBitSet;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::any() const {
    for (const BaseBitSet& baseBitSet : mBaseBitSetArray) {
        if (baseBitSet.any()) {
            return true;
        }
    }
    return false;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::none() const {
    for (const BaseBitSet& baseBitSet : mBaseBitSetArray) {
        if (!baseBitSet.none()) {
            return false;
        }
    }
    return true;
}

template <std::size_t N, typename ParamT>
constexpr std::size_t BitSetArray<N, ParamT>::count() const {
    size_t count = 0;
    for (const BaseBitSet& baseBitSet : mBaseBitSetArray) {
        count += baseBitSet.count();
    }
    return count;
}

template <std::size_t N, typename ParamT>
constexpr bool BitSetArray<N, ParamT>::intersects(const BitSetArray<N, ParamT>& other) const {
    for (std::size_t index = 0; index < kArraySize; index++) {
        if ((mBaseBitSetArray[index].bits() & other.mBaseBitSetArray[index].bits()) != 0) {
            return true;
        }
    }
    return false;
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT>& BitSetArray<N, ParamT>::flip() {
    for (BaseBitSet& baseBitSet : mBaseBitSetArray) {
        baseBitSet.flip();
    }

    // The last element in mBaseBitSetArray may need special handling
    mBaseBitSetArray[kArraySize - 1] &= kLastElementMask;
    return *this;
}

template <std::size_t N, typename ParamT>
constexpr typename BitSetArray<N, ParamT>::param_type BitSetArray<N, ParamT>::first() const {
    DAWN_ASSERT(any());
    for (size_t arrayIndex = 0; arrayIndex < kArraySize; ++arrayIndex) {
        const BaseBitSet& baseBitSet = mBaseBitSetArray[arrayIndex];
        if (baseBitSet.any()) {
            return baseBitSet.first() + (arrayIndex * priv::kDefaultBitSetSize);
        }
    }
    DAWN_UNREACHABLE();
    return 0;
}

template <std::size_t N, typename ParamT>
constexpr typename BitSetArray<N, ParamT>::param_type BitSetArray<N, ParamT>::last() const {
    DAWN_ASSERT(any());
    for (size_t arrayIndex = kArraySize; arrayIndex > 0; --arrayIndex) {
        const BaseBitSet& baseBitSet = mBaseBitSetArray[arrayIndex - 1];
        if (baseBitSet.any()) {
            return baseBitSet.last() + ((arrayIndex - 1) * priv::kDefaultBitSetSize);
        }
    }
    DAWN_UNREACHABLE();
    return 0;
}

template <std::size_t N, typename ParamT>
constexpr typename BitSetArray<N, ParamT>::value_type BitSetArray<N, ParamT>::bits(
    size_t index) const {
    return mBaseBitSetArray[index].bits();
}

template <std::size_t N, typename ParamT>
constexpr BitSetArray<N, ParamT> BitSetArray<N, ParamT>::Mask(std::size_t x) {
    BitSetArray result;

    for (size_t arrayIndex = 0; arrayIndex < kArraySize; ++arrayIndex) {
        const size_t bitOffset = arrayIndex * priv::kDefaultBitSetSize;
        if (x <= bitOffset) {
            break;
        }
        const size_t bitsInThisIndex = std::min(x - bitOffset, priv::kDefaultBitSetSize);
        result.mBaseBitSetArray[arrayIndex] = BaseBitSet::Mask(bitsInThisIndex);
    }

    return result;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> operator&(const BitSetT<N, BitsT, ParamT>& lhs,
                                              const BitSetT<N, BitsT, ParamT>& rhs) {
    BitSetT<N, BitsT, ParamT> result(lhs);
    result &= rhs.bits();
    return result;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> operator|(const BitSetT<N, BitsT, ParamT>& lhs,
                                              const BitSetT<N, BitsT, ParamT>& rhs) {
    BitSetT<N, BitsT, ParamT> result(lhs);
    result |= rhs.bits();
    return result;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> operator^(const BitSetT<N, BitsT, ParamT>& lhs,
                                              const BitSetT<N, BitsT, ParamT>& rhs) {
    BitSetT<N, BitsT, ParamT> result(lhs);
    result ^= rhs.bits();
    return result;
}

template <size_t N, typename BitsT, typename ParamT>
inline bool operator==(BitSetT<N, BitsT, ParamT>& lhs, BitSetT<N, BitsT, ParamT>& rhs) {
    return lhs.bits() == rhs.bits();
}

template <size_t N, typename BitsT, typename ParamT>
inline bool operator!=(BitSetT<N, BitsT, ParamT>& lhs, BitSetT<N, BitsT, ParamT>& rhs) {
    return !(lhs == rhs);
}

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_BITSET_H_
