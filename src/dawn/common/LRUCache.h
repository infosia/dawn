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

#ifndef SRC_DAWN_COMMON_LRUCACHE_H_
#define SRC_DAWN_COMMON_LRUCACHE_H_

#include <array>
#include <list>

#include "absl/container/flat_hash_map.h"
#include "dawn/common/MutexProtected.h"
#include "dawn/common/Result.h"

namespace dawn {

template <typename Key, typename Value, typename Error, typename CacheFuncs>
class LRUCache {
  public:
    explicit LRUCache(size_t capacity) : mCapacity(capacity) {}

    Result<Value, Error> Get(Key& key) {
        mCache.Use([&](auto cache) {
            // Try to lookup |Key| to see if we have a cache hit.
            auto it = cache->map.find(key);
            if (it != cache->map.end()) {
                cache->list.splice(cache->list.begin(), cache->list, it->second);
                return it->second->second;
            }

            // Otherwise, we need to try to create the entry.
            auto result = Create(key);
            if (result.IsError()) [[unlikely]] {
                return Result<Value, Error>(result.AcquireError());
            }
            auto value = result.AcquireSuccess();
            cache->list.emplace_front(key, value);
            cache->map.emplace(key, cache->list.begin());

            // If the LRU has exceeded it's capacity, remove the oldest entry.
            if (cache->list.size() > mCapacity) {
                auto back = cache->list.back();
                cache->map.erase(back.first);
                cache->list.pop_back();
            }

            return value;
        });
    }

    void Clear() {
        mCache.Use([](auto cache) {
            cache.map.clear();
            cache.list.clear();
        });
    }

  private:
    // Virtual creation function that implementations of the cache need to implement.
    virtual Result<Value, Error> Create(Key& key) = 0;

    const size_t mCapacity;

    using RecentList = std::list<std::pair<Key, Value>>;
    using Map = absl::flat_hash_map<Key, typename RecentList::iterator, CacheFuncs, CacheFuncs>;
    struct Cache {
        RecentList list;
        Map map;
    };
    MutexProtected<Cache> mCache;
};

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_LRUCACHE_H_
