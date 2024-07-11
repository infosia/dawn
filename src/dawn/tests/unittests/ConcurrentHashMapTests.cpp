// Copyright 2024 The Dawn & Tint Authors
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

#include <memory>
#include <thread>

#include "dawn/common/ConcurrentHashMap.h"
#include "dawn/utils/TestUtils.h"
#include "gtest/gtest.h"

namespace dawn {
namespace {

struct Error {};

class HashMap : public ConcurrentHashMap<uint32_t, uint32_t> {
  public:
    uint32_t GetOrAddValue(uint32_t key, uint32_t value) {
        auto result = GetOrAdd<Error>(
            key, [value](const uint32_t&) -> Result<uint32_t, Error> { return value; });

        EXPECT_TRUE(result.IsSuccess());
        return result.AcquireSuccess();
    }

    Result<uint32_t, Error> GetOrAddWithError(uint32_t key, Error error) {
        return GetOrAdd<Error>(key, [error](const uint32_t&) -> Result<uint32_t, Error> {
            return std::make_unique<Error>(error);
        });
    }

    uint32_t GetValue(uint32_t key) {
        auto result = GetOrAddWithError(key, {});

        EXPECT_TRUE(result.IsSuccess());
        return result.AcquireSuccess();
    }
};

// Test the duplicated inserts won't affect the entry in the table, and will return the existing
// value.
TEST(ConcurrentHashMapTest, DuplicatedInserts) {
    HashMap hashMap;

    hashMap.GetOrAddValue(1, 1);

    // Subsequent GetOrAddValue calls with the same key should return the same value inserted above.
    for (uint32_t i = 0; i < 100; ++i) {
        EXPECT_EQ(1u, hashMap.GetOrAddValue(1, i));
    }
}

// Insert on multithreads
TEST(ConcurrentHashMapTest, InsertOnMultithreads) {
    HashMap hashMap;

    utils::RunInParallel(100, [&](uint32_t i) { hashMap.GetOrAddValue(i, i); });

    for (uint32_t i = 0; i < 100; ++i) {
        EXPECT_EQ(i, hashMap.GetValue(i));
    }
}

// Test if multiple threads try to insert with the same key, only one should successfully execute
// the initializing function.
TEST(ConcurrentHashMapTest, DuplicatedInsertOnMultithreads) {
    HashMap hashMap;

    uint32_t numInserts = 0;

    utils::RunInParallel(100, [&](uint32_t threadId) {
        auto result = hashMap.GetOrAdd<Error>(/*key=*/1, [&](uint32_t) -> Result<uint32_t, Error> {
            numInserts++;
            return threadId;
        });
        EXPECT_TRUE(result.IsSuccess());
        result.AcquireSuccess();
    });

    EXPECT_EQ(1u, numInserts);

    EXPECT_LE(0u, hashMap.GetValue(1));
    EXPECT_LT(hashMap.GetValue(1), 100u);
}

// Test that if a thread tries to insert with an error, it won't affect the value in the table.
// Other threads can still insert the value.
TEST(ConcurrentHashMapTest, MixInsertWithErrorAndSuccessOnMultithreads) {
    HashMap hashMap;

    uint32_t numInserts = 0;

    // Even thread will return error, odd thread will attempt to insert a valid value.
    utils::RunInParallel(100, [&](uint32_t threadId) {
        auto result = hashMap.GetOrAdd<Error>(/*key=*/1, [&](uint32_t) -> Result<uint32_t, Error> {
            if (threadId % 2 == 0) {
                return std::make_unique<Error>();
            }
            numInserts++;
            return threadId;
        });
        if (result.IsSuccess()) {
            EXPECT_TRUE(result.AcquireSuccess() % 2 != 0);
        } else {
            result.AcquireError();
        }
    });

    EXPECT_EQ(1u, numInserts);
    // Only odd value should be inserted successfully.
    EXPECT_TRUE(hashMap.GetValue(1) % 2 != 0);
}

}  // anonymous namespace
}  // namespace dawn
