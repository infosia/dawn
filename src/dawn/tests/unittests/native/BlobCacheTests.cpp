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

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include <vector>

#include "dawn/common/Version_autogen.h"
#include "dawn/native/BlobCache.h"
#include "dawn/native/CacheKey.h"
#include "dawn/native/stream/Stream.h"
#include "dawn/tests/MockCallback.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "webgpu/webgpu_cpp.h"

namespace dawn::native {
namespace {

using testing::_;
using testing::MockCppCallback;

CacheKey CreateValidKey() {
    CacheKey key;
    stream::StreamIn(&key, std::string_view(reinterpret_cast<const char*>(kDawnVersion.data()),
                                            kDawnVersion.size()));
    return key;
}

TEST(BlobCacheTests, StoreAndLoad) {
    MockCppCallback<size_t (*)(std::string_view, std::span<uint8_t>)> mockLoadCallback;
    MockCppCallback<void (*)(std::string_view, std::span<const uint8_t>)> mockStoreCallback;

    wgpu::DawnCacheDeviceDescriptor desc = {};
    desc.SetDawnLoadCacheDataCallback(mockLoadCallback.TemplatedCallback(), &mockLoadCallback);
    desc.SetDawnStoreCacheDataCallback(mockStoreCallback.TemplatedCallback(), &mockStoreCallback);

    BlobCache cache(*FromCppAPI(&desc), false);

    CacheKey key = CreateValidKey();
    std::vector<std::byte> value = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};

    std::vector<uint8_t> storedData;

    // Expect Store
    EXPECT_CALL(mockStoreCallback, Call(_, _))
        .WillOnce([&](std::string_view k, std::span<const uint8_t> v) {
            EXPECT_EQ(k, key.AsStringView());
            storedData.assign(v.begin(), v.end());
        });
    cache.Store(key, value);

    // Expect Load
    EXPECT_CALL(mockLoadCallback, Call(_, _))
        .WillOnce([&](std::string_view k, std::span<uint8_t> v) -> size_t {
            EXPECT_EQ(k, key.AsStringView());
            EXPECT_TRUE(v.empty());
            return storedData.size();
        })
        .WillOnce([&](std::string_view k, std::span<uint8_t> v) -> size_t {
            EXPECT_EQ(k, key.AsStringView());
            EXPECT_EQ(v.size(), storedData.size());
            std::ranges::copy(storedData, v.begin());
            return storedData.size();
        });

    auto result = cache.Load(key);
    EXPECT_TRUE(result.IsSuccess());
    Blob loadedBlob = result.AcquireSuccess();

    EXPECT_EQ(loadedBlob.Size(), value.size());
    EXPECT_EQ(memcmp(loadedBlob.DataPtr(), value.data(), value.size()), 0);
}

}  // namespace
}  // namespace dawn::native
