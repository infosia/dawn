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

#include <gtest/gtest.h>

#include "partition_alloc/pointers/raw_ptr.h"
#include "src/utils/span.h"
#include "src/utils/typed_integer.h"

namespace dawn {
namespace {

static constexpr std::array<int, 5> kSpanData = {1, 2, 3, 4, 5};

using Index = TypedInteger<struct IndexT, uint32_t>;

struct FakeRange {
    size_t size() const { return kSpanData.size(); }
    const int* data() const { return kSpanData.data(); }
};

struct FakeTypedRange {
    Index size() const {
        return Index{uint32_t{kSpanData.size()}};
    }
    const int* data() const { return kSpanData.data(); }
};

// No compile
//  const pointer to non-const span.
//  same for 2* pointer.
//  need unsafe buffer for the two constructors that have it.
//  test for incompatible range
//    - const for nonconst span
//    - data not returning a pointer.
//    - size not returning a size_t or Index.
// every method takind an Index doesn't comiple with size_t.

TEST(SpanTest, Constructor_Default) {
    {
        Span<int> sp;
        EXPECT_EQ(sp.size(), 0u);
        EXPECT_EQ(sp.data(), nullptr);
    }
    {
        TypedSpan<int, Index> sp;
        EXPECT_EQ(sp.size(), Index{0u});
        EXPECT_EQ(sp.data(), nullptr);
    }
}

TEST(SpanTest, Constructor_PointerAndSize) {
    int data[] = {1, 2, 3};
    int constData[] = {1, 2, 3};
    raw_ptr<int> rptr = data;

    // T* + size for Span<T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<int> DAWN_UNSAFE_BUFFERS(sp{&data[0], 3});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), data);
    }
    // const T* + size for Span<const T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<const int> DAWN_UNSAFE_BUFFERS(sp{&constData[0], 3});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), constData);
    }
    // T* + size for Span<const T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<const int> DAWN_UNSAFE_BUFFERS(sp{&data[0], 3});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), data);
    }

    // T* + size for TypedSpan<T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<int, Index> DAWN_UNSAFE_BUFFERS(sp{&data[0], Index{3u}});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), data);
    }
    // const T* + size for TypedSpan<const T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<const int, Index> DAWN_UNSAFE_BUFFERS(sp{&constData[0], Index{3u}});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), constData);
    }
    // T* + size for TypedSpan<const T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<const int, Index> DAWN_UNSAFE_BUFFERS(sp{&data[0], Index{3u}});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), data);
    }
}

TEST(SpanTest, Constructor_TwoIterators) {
    std::array<int, 3> data = {1, 2, 3};
    const std::array<int, 3> constData = {1, 2, 3};

    // 2 x T* for Span<T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<int> DAWN_UNSAFE_BUFFERS(sp{data.begin(), data.end()});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), data.data());
    }
    // 2 x const T* for Span<const T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<const int> DAWN_UNSAFE_BUFFERS(sp{constData.begin(), constData.end()});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), constData.data());
    }
    // 2 x T* for Span<const T>
    {
        // SAFETY: Test for the unsafe constructor.
        Span<const int> DAWN_UNSAFE_BUFFERS(sp{data.begin(), data.end()});
        EXPECT_EQ(sp.size(), 3u);
        EXPECT_EQ(sp.data(), data.data());
    }

    // 2 x T* for TypedSpan<T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<int, Index> DAWN_UNSAFE_BUFFERS(sp{data.begin(), data.end()});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), data.data());
    }
    // 2 x const T* for TypedSpan<const T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<const int, Index> DAWN_UNSAFE_BUFFERS(sp{constData.begin(), constData.end()});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), constData.data());
    }
    // 2 x T* for TypedSpan<const T, ...>
    {
        // SAFETY: Test for the unsafe constructor.
        TypedSpan<const int, Index> DAWN_UNSAFE_BUFFERS(sp{data.begin(), data.end()});
        EXPECT_EQ(sp.size(), Index{3u});
        EXPECT_EQ(sp.data(), data.data());
    }
}

TEST(SpanDeathTest, Constructor_TwoIteratorsInverted) {
    std::array<int, 3> data = {1, 2, 3};
    DAWN_UNSAFE_BUFFERS(EXPECT_DEATH((Span<int>{data.end(), data.begin()}), ""));
}

TEST(SpanTest, ConstructorFromCompatibleRange) {
    // std::array<T> -> Span<T>
    {
        std::array<int, 3> data = {1, 2, 3};
        Span<int> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
    // const std::array<T> -> Span<const T>
    {
        const std::array<int, 3> data = {1, 2, 3};
        Span<const int> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
    // const std::vector<T> -> Span<T>
    {
        std::vector<int> data{{1, 2, 3}};
        Span<const int> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
    // const std::string -> Span<char>
    {
        std::string data = "foo";
        Span<char> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
    // std::string_view -> Span<char>
    {
        std::string_view data = "foo";
        Span<const char> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
    // FakeRange -> Span<const int>
    {
        FakeRange data;
        Span<const int> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }

    // FakeTypedRange -> TypedSpan<const int, ...>
    {
        FakeTypedRange data;
        TypedSpan<const int, Index> sp{data};
        EXPECT_EQ(sp.size(), data.size());
        EXPECT_EQ(sp.data(), data.data());
    }
}

TEST(SpanTest, CopyConstructor) {
    std::array<int, 3> data = {1, 2, 3};
    Span<int> sp{data};

    Span<int> sp2{sp};
    ASSERT_EQ(sp.data(), sp2.data());
    ASSERT_EQ(sp.size(), sp2.size());

    Span<const int> sp3{sp};
    ASSERT_EQ(sp.data(), sp3.data());
    ASSERT_EQ(sp.size(), sp3.size());
}

TEST(SpanTest, CopyAssignment) {
    std::array<int, 3> data = {1, 2, 3};
    Span<int> sp{data};

    Span<int> sp2;
    sp2 = sp;
    ASSERT_EQ(sp.data(), sp2.data());
    ASSERT_EQ(sp.size(), sp2.size());
}

TEST(SpanTest, MoveConstructor) {
    std::array<int, 3> data = {1, 2, 3};
    Span<int> sp{data};

    Span<int> sp2{std::move(sp)};
    ASSERT_EQ(data.data(), sp2.data());
    ASSERT_EQ(data.size(), sp2.size());

    // Move actually does a copy so sp stays the same.
    ASSERT_EQ(data.data(), sp.data());
    ASSERT_EQ(data.size(), sp.size());
}

TEST(SpanTest, MoveAssignment) {
    std::array<int, 3> data = {1, 2, 3};
    Span<int> sp{data};

    Span<int> sp2;
    sp2 = std::move(sp);
    ASSERT_EQ(data.data(), sp2.data());
    ASSERT_EQ(data.size(), sp2.size());

    // Move actually does a copy so sp stays the same.
    ASSERT_EQ(data.data(), sp.data());
    ASSERT_EQ(data.size(), sp.size());
}

TEST(SpanTest, BeginEnd) {
    {
        Span<const int> sp(FakeRange{});
        ASSERT_EQ(sp.begin(), &*kSpanData.begin());
        ASSERT_EQ(sp.end(), &*kSpanData.end());
        ASSERT_EQ(sp.cbegin(), &*kSpanData.cbegin());
        ASSERT_EQ(sp.cend(), &*kSpanData.cend());
    }
    {
        TypedSpan<const int, Index> sp(FakeTypedRange{});
        ASSERT_EQ(sp.begin(), &*kSpanData.begin());
        ASSERT_EQ(sp.end(), &*kSpanData.end());
        ASSERT_EQ(sp.cbegin(), &*kSpanData.cbegin());
        ASSERT_EQ(sp.cend(), &*kSpanData.cend());
    }
}

TEST(SpanTest, BeginEndForIteration) {
    // Uses begin/end
    {
        Span<const int> sp(FakeRange{});

        int expected = 1;
        for (const int& i : sp) {
            EXPECT_EQ(i, expected);
            expected++;
        }
    }

    // Uses cbegin/cend
    {
        const Span<const int> sp(FakeRange{});

        int expected = 1;
        for (const int& i : sp) {
            EXPECT_EQ(i, expected);
            expected++;
        }
    }

    // Typed, uses begin/end
    {
        TypedSpan<const int, Index> sp(FakeTypedRange{});

        int expected = 1;
        for (const int& i : sp) {
            EXPECT_EQ(i, expected);
            expected++;
        }
    }

    // Typed, uses cbegin/cend
    {
        const TypedSpan<const int, Index> sp(FakeTypedRange{});

        int expected = 1;
        for (const int& i : sp) {
            EXPECT_EQ(i, expected);
            expected++;
        }
    }
}

TEST(SpanTest, FrontBack) {
    {
        Span<const int> sp(FakeRange{});
        EXPECT_EQ(&sp.front(), &kSpanData.front());
        EXPECT_EQ(&sp.back(), &kSpanData.back());
    }
    {
        TypedSpan<const int, Index> sp(FakeTypedRange{});
        EXPECT_EQ(&sp.front(), &kSpanData.front());
        EXPECT_EQ(&sp.back(), &kSpanData.back());
    }
}

TEST(SpanDeathTest, FrontBackOfEmpty) {
    Span<const int> sp;
    EXPECT_DEATH(sp.front(), "");
    EXPECT_DEATH(sp.back(), "");
}

TEST(SpanTest, Indexing) {
    {
        Span<const int> sp(FakeRange{});
        for (size_t i = 0; i < kSpanData.size(); i++) {
            EXPECT_EQ(&sp.at(i), &kSpanData[i]);
            EXPECT_EQ(&sp[i], &kSpanData[i]);
        }
    }
    {
        TypedSpan<const int, Index> sp(FakeTypedRange{});
        for (size_t i = 0; i < kSpanData.size(); i++) {
            Index id{static_cast<uint32_t>(i)};
            EXPECT_EQ(&sp.at(id), &kSpanData[i]);
            EXPECT_EQ(&sp[id], &kSpanData[i]);
        }
    }
}

TEST(SpanDeathTest, IndexingOOB) {
    Span<const int> sp(FakeRange{});
    EXPECT_DEATH(sp.at(sp.size()), "");
    EXPECT_DEATH(sp[sp.size()], "");
}

// .data() and .size() are tested in every test essentially.

TEST(SpanTest, Empty) {
    ASSERT_FALSE(Span<const int>{FakeRange{}}.empty());
    // SAFETY: Test for the unsafe constructor.
    ASSERT_FALSE(DAWN_UNSAFE_BUFFERS((Span<const int>{static_cast<int*>(nullptr), 1u})).empty());
    ASSERT_TRUE(Span<const int>{}.empty());
    // SAFETY: Test for the unsafe constructor.
    ASSERT_TRUE(DAWN_UNSAFE_BUFFERS((Span<const int>{kSpanData.data(), 0u})).empty());

    ASSERT_FALSE((TypedSpan<const int, Index>{FakeTypedRange{}}.empty()));
    // SAFETY: Test for the unsafe constructor.
    ASSERT_FALSE(
        DAWN_UNSAFE_BUFFERS((TypedSpan<const int, Index>{static_cast<int*>(nullptr), Index{1u}}))
            .empty());
    ASSERT_TRUE((TypedSpan<const int, Index>{}.empty()));
    // SAFETY: Test for the unsafe constructor.
    ASSERT_TRUE(
        DAWN_UNSAFE_BUFFERS((TypedSpan<const int, Index>{kSpanData.data(), Index{0u}})).empty());
}

TEST(SpanTest, SizeBytes) {
    ASSERT_EQ(Span<int>{}.size_bytes(), 0u);
    ASSERT_EQ((TypedSpan<int, Index>{}.size_bytes()), 0u);

    std::array<int, 3> ints;
    ASSERT_EQ(Span<int>{ints}.size_bytes(), sizeof(ints));

    std::array<double, 10> doubles;
    ASSERT_EQ(Span<double>{doubles}.size_bytes(), sizeof(doubles));
}

// subspan first last and death tests

/*

// If the index/size is 64-bit, it needs to be narrowed to size_t. Verify that's checked correctly.
TEST_F(ITypSpanDeathTest, OversizedIndex) {
    // These tests are only relevant on 32-bit builds.
    if constexpr (sizeof(size_t) > sizeof(uint32_t)) {
        GTEST_SKIP();
    }

    using Key64 = TypedInteger<struct Key64T, uint64_t>;
    static constexpr Key64 kHugeKey64{0x1'0000'0000LLU};

    std::array<Val, 10> arr;
    ityp::span<Key64, Val> span(arr.data(), Key64(arr.size()));

    span[Key64(9u)];
    // Regular out-of-bounds.
    EXPECT_DEATH(span[Key64(10u)], "");

    span[Key64(0u)];
    // If this were cast to a 32-bit size_t without a check, it would be in-bounds.
    EXPECT_DEATH(span[kHugeKey64], "");
}

*/

}  // anonymous namespace
}  // namespace dawn
