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

    // Type for use with Uninit() which requires a POD value type.
    using PODHeapArray = ityp::heap_array<Key, int>;
};

TEST_F(ITypHeapArrayTest, KeyTypes) {
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, uint8_t>, int> a;
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, uint32_t>, int> b;
    [[maybe_unused]] ityp::heap_array<TypedInteger<struct KU8, size_t>, int> c;

    enum class E : uint32_t {};
    [[maybe_unused]] ityp::heap_array<E, int> e;

    [[maybe_unused]] ityp::heap_array<uint16_t, int> i;
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
    ASSERT_EQ(&*arr.end(), DAWN_UNSAFE_BUFFERS(&arr[Key(0)]) + static_cast<size_t>(arr.size()));
    ASSERT_EQ(&arr.front(), &arr[Key(0)]);
    ASSERT_EQ(&arr.back(), &arr[Key(9)]);
    ASSERT_EQ(arr.data(), &arr[Key(0)]);

    // const versions
    const HeapArray& constArr = arr;
    ASSERT_EQ(&*constArr.begin(), &constArr[Key(0)]);
    ASSERT_EQ(&*constArr.end(),
              DAWN_UNSAFE_BUFFERS(&constArr[Key(0)]) + static_cast<size_t>(constArr.size()));
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

// Checks that various byte types are always aligned to alignof(std::max_align_t).
template <typename HA>
void CheckByteAllocation(const HA& ha) {
    ASSERT_EQ(reinterpret_cast<uintptr_t>(ha.data()) % alignof(std::max_align_t), 0u);
}
TEST_F(ITypHeapArrayTest, ByteTypeAlignment) {
    CheckByteAllocation(ityp::heap_array<size_t, std::byte>(16));
    CheckByteAllocation(ityp::heap_array<size_t, char>(16));
    // uint8_t is not technically guaranteed, but as of this writing, some Dawn code relies on it.
    CheckByteAllocation(ityp::heap_array<size_t, uint8_t>(16));
}

// Check that various non-byte types are always aligned as expected.
template <typename T>
void CheckTypedAllocation(const ityp::heap_array<size_t, T>& ha) {
    ASSERT_EQ(reinterpret_cast<uintptr_t>(ha.data()) % alignof(T), 0u);
}
TEST_F(ITypHeapArrayTest, NonByteTypeAlignment) {
    CheckTypedAllocation(ityp::heap_array<size_t, uint32_t>(16));
    struct A {
        uint8_t x;
    };
    CheckTypedAllocation(ityp::heap_array<size_t, A>{16});
    struct B {
        std::max_align_t x;
    };
    CheckTypedAllocation(ityp::heap_array<size_t, B>{16});
    struct alignas(256) C {
        int x;
    };
    static_assert(alignof(C) == 256);
    CheckTypedAllocation(ityp::heap_array<size_t, C>{16});
}

// Test allocations close to the size of the address space.
TEST_F(ITypHeapArrayTest, OutOfMemoryAtLimit) {
    // Allocation of uint8_t values
    {
        // Test sizes that are blocked by the heap_array implementation before trying to allocate,
        // and one element smaller so that heap_array will actually try to allocate.
        for (size_t leaveSpace : {0, 4094, 4095}) {
            ityp::heap_array<size_t, uint8_t> arr{std::numeric_limits<size_t>::max() - leaveSpace,
                                                  std::nothrow};
            EXPECT_FALSE(bool{arr});
        }
    }
    // Allocation of uint32_t values
    {
        static constexpr size_t kMaxSize = std::numeric_limits<size_t>::max() / sizeof(uint32_t);

        // Test sizes that are blocked by the heap_array implementation before trying to allocate,
        // and one element smaller so that heap_array will actually try to allocate.
        for (size_t leaveSpace : {0, 1022, 1023}) {
            ityp::heap_array<size_t, uint32_t> arr{kMaxSize - leaveSpace, std::nothrow};
            EXPECT_FALSE(bool{arr});
        }

        // Cases where size * sizeof(val) is larger than the address space and could overflow.
        {
            ityp::heap_array<size_t, uint32_t> arr{kMaxSize + 1, std::nothrow};
            EXPECT_FALSE(bool{arr});
        }
        {
            ityp::heap_array<size_t, uint32_t> arr{kMaxSize + 2, std::nothrow};
            EXPECT_FALSE(bool{arr});
        }
    }
}

// 64-bit size is way too large to allocate (but well within size_t on 64-bit).
TEST_F(ITypHeapArrayTest, OutOfMemory64) {
    using Key64 = TypedInteger<struct Key64T, uint64_t>;
    static constexpr Key64 kHugeSize{0x1000'0000'0000'0000};

    {
        ityp::heap_array<Key64, Val> arr{kHugeSize, std::nothrow};
        EXPECT_FALSE(bool{arr});
    }
    {
        auto arr = ityp::heap_array<Key64, int>::Uninit(kHugeSize, std::nothrow);
        EXPECT_FALSE(bool{arr});
    }
}

// Name "*DeathTest" per https://google.github.io/googletest/advanced.html#death-test-naming
using ITypHeapArrayDeathTest = ITypHeapArrayTest;

// Test allocations close to the size of the address space.
TEST_F(ITypHeapArrayDeathTest, OutOfMemoryAtLimit) {
    // Allocation of uint8_t values
    {
        // Exact maximum amount that will fit in the address space
        EXPECT_DEATH((ityp::heap_array<size_t, uint8_t>{std::numeric_limits<size_t>::max()}), "");
        // And a bit less.
        EXPECT_DEATH((ityp::heap_array<size_t, uint8_t>{std::numeric_limits<size_t>::max() - 4095}),
                     "");
    }
    // Allocation of uint32_t values
    {
        static constexpr size_t kMaxSize = std::numeric_limits<size_t>::max() / sizeof(uint32_t);
        // Maximum amount that will fit in the address space
        EXPECT_DEATH((ityp::heap_array<size_t, uint32_t>{kMaxSize}), "");
        // And a bit less.
        EXPECT_DEATH((ityp::heap_array<size_t, uint32_t>{kMaxSize - 1023}), "");

        // Cases where size * sizeof(val) is larger than the address space and could overflow.
        EXPECT_DEATH((ityp::heap_array<size_t, uint32_t>{kMaxSize + 1}), "");
        EXPECT_DEATH((ityp::heap_array<size_t, uint32_t>{kMaxSize + 2}), "");
    }
}

// 64-bit size is way too large to allocate (but well within size_t on 64-bit).
TEST_F(ITypHeapArrayDeathTest, OutOfMemory64) {
    using Key64 = TypedInteger<struct Key64T, uint64_t>;
    static constexpr Key64 kHugeSize{0x1000'0000'0000'0000};

    EXPECT_DEATH((ityp::heap_array<Key64, Val>{kHugeSize}), "");
    EXPECT_DEATH(((void)ityp::heap_array<Key64, int>::Uninit(kHugeSize)), "");
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

// If the index/size is 64-bit, it needs to be narrowed to size_t. Verify that's checked correctly.
TEST_F(ITypHeapArrayDeathTest, OversizedIndex) {
    // These tests are only relevant on 32-bit builds.
    if constexpr (sizeof(size_t) > sizeof(uint32_t)) {
        GTEST_SKIP();
    }

    using Key64 = TypedInteger<struct Key64T, uint64_t>;
    static constexpr Key64 kHugeKey64{0x1000'0000'0000'0000};

    // Crash either due to OOM (on 64-bit) or due to narrowing (on 32-bit).
    EXPECT_DEATH((ityp::heap_array<Key64, Val>(kHugeKey64)), "");

    ityp::heap_array<Key64, Val> vec(Key64(10));

    vec[Key64(9)];
    // Regular out-of-bounds.
    EXPECT_DEATH(vec[Key64(10)], "");

    vec[Key64(0)];
    // If this were cast to a 32-bit size_t without a check, it would be in-bounds.
    EXPECT_DEATH(vec[kHugeKey64], "");
}

// Using uninitialized memory should crash on MSan builds.
TEST_F(ITypHeapArrayDeathTest, ReadUninitWithPODType) {
    if constexpr (!DAWN_MSAN_ENABLED()) {
        GTEST_SKIP();
    }

    auto arr = PODHeapArray::Uninit(Key{1});
    EXPECT_DEATH([&]() { printf("%d\n", arr[Key{0}]); }(), "");
}

}  // anonymous namespace
}  // namespace dawn
