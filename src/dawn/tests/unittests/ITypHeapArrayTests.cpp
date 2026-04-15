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

#include <limits>

#include "dawn/common/TypedInteger.h"
#include "dawn/common/ityp_heap_array.h"
#include "gtest/gtest.h"

namespace dawn {
namespace {

class ITypHeapArrayTest : public testing::Test {
  protected:
    using Key = TypedInteger<struct KeyT, size_t>;
    using Val = TypedInteger<struct ValT, uint32_t>;
    using HeapArray = ityp::heap_array<Key, Val>;
    using PODHeapArray = ityp::heap_array<Key, int>;
};

TEST_F(ITypHeapArrayTest, KeyTypes) {
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, uint8_t>, int> a;
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, uint32_t>, int> b;
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, size_t>, int> c;
}

// Test that values can be set at an index and retrieved from the same index.
TEST_F(ITypHeapArrayTest, Indexing) {
    HeapArray arr{Key{10}};
    {
        arr[Key(2)] = Val(5);
        arr[Key(1)] = Val(9);
        arr[Key(9)] = Val(2);

        ASSERT_EQ(arr[Key(2)], Val(5));
        ASSERT_EQ(arr[Key(1)], Val(9));
        ASSERT_EQ(arr[Key(9)], Val(2));
    }
}

// Test that the heap_array can be iterated in order with a range-based for loop
TEST_F(ITypHeapArrayTest, RangeBasedIteration) {
    HeapArray arr{Key{10}};
    ASSERT_TRUE(bool{arr});

    // Assign in a non-const range-based for loop
    for (uint32_t i = 0; Val& val : arr) {
        val = Val(i);
    }

    // Check values in a const range-based for loop
    for (uint32_t i = 0; Val val : static_cast<const HeapArray&>(arr)) {
        ASSERT_EQ(val, arr[Key(i++)]);
    }
}

// Test that begin/end/front/back/data return pointers/references to the correct elements.
TEST_F(ITypHeapArrayTest, BeginEndFrontBackData) {
    HeapArray arr{Key{10}};

    // non-const versions
    ASSERT_EQ(&*arr.begin(), &arr[Key(0)]);
    ASSERT_EQ(&*arr.end(), &arr[Key(0)] + static_cast<size_t>(arr.size()));
    ASSERT_EQ(&arr.front(), &arr[Key(0)]);
    ASSERT_EQ(&arr.back(), &arr[Key(9)]);
    ASSERT_EQ(arr.data(), &arr[Key(0)]);

    // const versions
    const HeapArray& constArr = arr;
    ASSERT_EQ(&*constArr.begin(), &constArr[Key(0)]);
    ASSERT_EQ(&*constArr.end(), &constArr[Key(0)] + static_cast<size_t>(constArr.size()));
    ASSERT_EQ(&constArr.front(), &constArr[Key(0)]);
    ASSERT_EQ(&constArr.back(), &constArr[Key(9)]);
    ASSERT_EQ(constArr.data(), &constArr[Key(0)]);
}

// Empty HeapArray.
TEST_F(ITypHeapArrayTest, Empty) {
    {
        HeapArray arr{Key{0}};
        ASSERT_TRUE(bool{arr});
        ASSERT_NE(nullptr, arr.data());
        ASSERT_EQ(Key(0), arr.size());
    }
    {
        HeapArray arr{Key{0}, std::nothrow};
        ASSERT_TRUE(bool{arr});
        ASSERT_NE(nullptr, arr.data());
        ASSERT_EQ(Key(0), arr.size());
    }
}

// `Uninit`ialized HeapArray.
TEST_F(ITypHeapArrayTest, Uninit) {
    auto arr = PODHeapArray::Uninit(Key{1});
    ASSERT_TRUE(bool{arr});

    arr[Key(0)] = 5;
    ASSERT_EQ(arr[Key(0)], 5);
}

// Too large to allocate, fallible allocations
TEST_F(ITypHeapArrayTest, OutOfMemory) {
    constexpr Key kLargeSize{0x1'0000'0000'0000};
    if constexpr (std::numeric_limits<size_t>::max() < uint64_t{kLargeSize}) {
        GTEST_SKIP();
    }
    {
        HeapArray arr{kLargeSize, std::nothrow};
        EXPECT_FALSE(bool{arr});
    }
    {
        auto arr = PODHeapArray::Uninit(kLargeSize, std::nothrow);
        EXPECT_FALSE(bool{arr});
    }
}

// Name "*DeathTest" per https://google.github.io/googletest/advanced.html#death-test-naming
using ITypHeapArrayDeathTest = ITypHeapArrayTest;

// Too large to allocate, non-fallible allocations
TEST_F(ITypHeapArrayDeathTest, OutOfMemory) {
    constexpr Key kLargeSize{0x1'0000'0000'0000};
    if constexpr (std::numeric_limits<size_t>::max() < uint64_t{kLargeSize}) {
        GTEST_SKIP();
    }

    EXPECT_DEATH(HeapArray{kLargeSize}, "");

    EXPECT_DEATH(((void)PODHeapArray::Uninit(kLargeSize)), "");
}

// Out of bounds accesses should crash even in release (the underlying container
// should have asserts enabled).
TEST_F(ITypHeapArrayDeathTest, OutOfBounds) {
    // MSVC doesn't have asserts (without _MSVC_STL_HARDENING).
    if constexpr (DAWN_COMPILER_IS(MSVC)) {
        GTEST_SKIP();
    }

    HeapArray arr{Key{10}};
    EXPECT_DEATH(arr[Key(10)], "");

    const HeapArray& constArr = arr;
    EXPECT_DEATH(constArr[Key(10)], "");
}

// Using uninitialized memory should crash on MSan builds.
TEST_F(ITypHeapArrayDeathTest, ReadUninitWithPODType) {
    if constexpr (!DAWN_MSAN_ENABLED()) {
        GTEST_SKIP();
    }

    // Note we use `int` here instead of `Val` because TypedIntegers have inconsistent default-init
    // behavior between assert builds and non-assert builds.
    auto arr = PODHeapArray::Uninit(Key{1});
    EXPECT_DEATH([&]() { printf("%d\n", arr[Key{0}]); }(), "");
}

}  // anonymous namespace
}  // namespace dawn
