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

#ifndef SRC_DAWN_COMMON_CONCURRENT_HASHMAP_H_
#define SRC_DAWN_COMMON_CONCURRENT_HASHMAP_H_

#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/container/hash_container_defaults.h"
#include "dawn/common/Assert.h"
#include "dawn/common/Mutex.h"
#include "dawn/common/MutexProtected.h"
#include "dawn/common/RefCounted.h"
#include "dawn/common/Result.h"

namespace dawn {

// A hash table that is thread safe and guarantees that:
// - If multiple threads try to insert and initialize the same entry in the table,
//   only one of them will proceed, the other threads will just get the result.
// TODO(42241511): This hash table currently only supports insertion, in future we
// should support deletion and possibly LRU style cache.
template <typename Key, typename Value, typename HashFunc = absl::DefaultHashContainerHash<Key>>
class ConcurrentHashMap {
  public:
    ~ConcurrentHashMap() {
        mHashMap.Use([](auto hashMap) { hashMap->clear(); });
    }

    // Attempt to retrieve a value with the given key from the hash table.
    // If the value doesn't exist, it will be added and initialized with the provided initializer
    // function.
    template <typename Error>
    Result<Value, Error> GetOrAdd(
        const Key& key,
        const std::function<Result<Value, Error>(const Key&)>& initializer) {
        auto entryRef = mHashMap.Use([&key](auto hashMap) -> auto {
            auto& entry = (*hashMap)[key];
            if (!entry) {
                entry = AcquireRef(new Entry);
            }
            return entry;
        });

        // Allow only one thread to initialize the entry.
        return entryRef->valueOrNone.Use(
            [&key, &initializer](auto valueOrNone) -> Result<Value, Error> {
                if (valueOrNone->has_value()) {
                    return valueOrNone->value();
                }

                auto result = initializer(key);
                if (result.IsError()) {
                    return result;
                }

                *valueOrNone = result.AcquireSuccess();

                return valueOrNone->value();
            });
    }

  private:
    // Also we use Ref<Entry> because while one thread tries to initialize an entry's value, another
    // thread might delete the same entry. So using Ref we can safely modify the value even if the
    // entry is no longer in the table.
    struct Entry : public RefCounted {
        MutexProtected<std::optional<Value>> valueOrNone;
    };

    MutexProtected<absl::flat_hash_map<Key, Ref<Entry>, HashFunc>> mHashMap;
};

}  // namespace dawn

#endif
