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

#include <set>
#include <utility>
#include <vector>

#include "dawn/common/BitSetRangeIterator.h"
#include "gtest/gtest.h"

namespace dawn {
namespace {

class BitSetRangeIteratorTest : public testing::Test {
  protected:
    std::bitset<40> mStateBits;
};

// Test basic range iteration with single bits (each range has size 1)
TEST_F(BitSetRangeIteratorTest, SingleBitRanges) {
    std::vector<std::pair<uint32_t, uint32_t>> expectedRanges;
    expectedRanges.push_back({2, 1});  // offset 2, size 1
    // expectedRanges.push_back({6, 1});  // offset 6, size 1
    // expectedRanges.push_back({8, 1});  // offset 8, size 1
    // expectedRanges.push_back({35, 1}); // offset 35, size 1

    for (const auto& range : expectedRanges) {
        mStateBits.set(range.first);
    }

    std::vector<std::pair<uint32_t, uint32_t>> foundRanges;
    for (auto range : IterateBitSetRanges(mStateBits)) {
        foundRanges.push_back(range);
    }

    EXPECT_EQ(expectedRanges.size(), foundRanges.size());
    for (size_t i = 0; i < expectedRanges.size(); ++i) {
        EXPECT_EQ(expectedRanges[i].first, foundRanges[i].first);
        EXPECT_EQ(expectedRanges[i].second, foundRanges[i].second);
    }
}

// Test ranges with consecutive bits
TEST_F(BitSetRangeIteratorTest, ConsecutiveBitRanges) {
    std::vector<std::pair<uint32_t, uint32_t>> expectedRanges;
    expectedRanges.push_back({2, 3});   // bits 2,3,4
    expectedRanges.push_back({8, 2});   // bits 8,9
    expectedRanges.push_back({35, 4});  // bits 35,36,37,38

    for (const auto& range : expectedRanges) {
        for (uint32_t i = 0; i < range.second; ++i) {
            mStateBits.set(range.first + i);
        }
    }

    std::vector<std::pair<uint32_t, uint32_t>> foundRanges;
    for (auto range : IterateBitSetRanges(mStateBits)) {
        foundRanges.push_back(range);
    }

    EXPECT_EQ(expectedRanges.size(), foundRanges.size());
    for (size_t i = 0; i < expectedRanges.size(); ++i) {
        EXPECT_EQ(expectedRanges[i].first, foundRanges[i].first);
        EXPECT_EQ(expectedRanges[i].second, foundRanges[i].second);
    }
}

// Test an empty iterator
TEST_F(BitSetRangeIteratorTest, EmptySet) {
    bool sawRange = false;
    for ([[maybe_unused]] auto range : IterateBitSetRanges(mStateBits)) {
        sawRange = true;
    }
    EXPECT_FALSE(sawRange);
}

// Test iterating a result of combining two bitsets
TEST_F(BitSetRangeIteratorTest, NonLValueBitset) {
    std::bitset<40> otherBits;

    // Set up first bitset with consecutive ranges
    mStateBits.set(1);
    mStateBits.set(2);
    mStateBits.set(3);
    mStateBits.set(4);

    // Set up second bitset with overlapping and non-overlapping bits
    otherBits.set(0);
    otherBits.set(1);
    otherBits.set(2);
    otherBits.set(4);
    otherBits.set(5);

    std::vector<std::pair<uint32_t, uint32_t>> expectedRanges;
    expectedRanges.push_back({1, 2});  // bits 1,2
    expectedRanges.push_back({4, 1});  // bit 4

    std::vector<std::pair<uint32_t, uint32_t>> foundRanges;
    for (auto range : IterateBitSetRanges(mStateBits & otherBits)) {
        foundRanges.push_back(range);

        // Verify all bits in the range are set in both bitsets
        for (uint32_t i = 0; i < range.second; ++i) {
            EXPECT_TRUE(mStateBits[range.first + i]);
            EXPECT_TRUE(otherBits[range.first + i]);
        }
    }

    EXPECT_EQ(expectedRanges.size(), foundRanges.size());
    for (size_t i = 0; i < expectedRanges.size(); ++i) {
        EXPECT_EQ(expectedRanges[i].first, foundRanges[i].first);
        EXPECT_EQ(expectedRanges[i].second, foundRanges[i].second);
    }
}

// Test ranges that cross word boundaries
TEST_F(BitSetRangeIteratorTest, CrossWordBoundaryRanges) {
    // Set a range that crosses 32-bit word boundary (bits 30-33)
    for (uint32_t i = 30; i <= 33; ++i) {
        mStateBits.set(i);
    }

    std::vector<std::pair<uint32_t, uint32_t>> foundRanges;
    for (auto range : IterateBitSetRanges(mStateBits)) {
        foundRanges.push_back(range);
    }

    EXPECT_EQ(1u, foundRanges.size());
    EXPECT_EQ(30u, foundRanges[0].first);
    EXPECT_EQ(4u, foundRanges[0].second);
}

}  // anonymous namespace
}  // namespace dawn
