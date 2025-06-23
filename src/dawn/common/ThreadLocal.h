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

#ifndef SRC_DAWN_COMMON_THREADLOCAL_H_
#define SRC_DAWN_COMMON_THREADLOCAL_H_

#include <cstdint>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "dawn/common/MutexProtected.h"
#include "dawn/common/TypeTraits.h"

namespace dawn {

using ThreadUniqueId = uint64_t;

// Returns a Dawn unique thread ID that can be used for thread local objects. This is NOT the same
// as a PID.
ThreadUniqueId GetThreadUniqueId();

// Returns true if the thread associated with the ID is still running.
bool IsThreadAlive(ThreadUniqueId ThreadUniqueId);

template <typename T>
class PerThread;

template <NonPointer T>
class PerThread<T> {
  public:
    T* Get() {
        return mData.Use([&](auto data) {
            if (auto it = data->find(GetThreadUniqueId()); it != data->end()) {
                return it->second.get();
            }

            // Each time a new thread creates a value, attempt to clean up terminated thread values
            // before adding the new one.
            Trim(data);

            auto [it, inserted] = data->try_emplace(GetThreadUniqueId(), new T);
            DAWN_ASSERT(inserted);
            return it->second.get();
        });
    }

    void Set(T* value) {
        mData.Use([&](auto data) {
            // Each time a new thread sets a value, attempt to clean up terminated thread values
            // before adding the new one.
            Trim(data);

            auto [it, inserted] = data->try_emplace(GetThreadUniqueId(), value);
            if (!inserted) {
                it.second.reset(value);
            }
        });
    }

    void Trim() {
        mData.Use([](auto data) { Trim(data); });
    }

  private:
    // The values are always stored as std::unique_ptrs so that even if the map is rehashed or
    // changed across threads, results are always the same. The lock is to prevent any rehashing
    // from racing with reading a from the map.
    using Map = absl::flat_hash_map<ThreadUniqueId, std::unique_ptr<T>>;
    using MapUsage = MutexProtected<Map>::Usage;

    MutexProtected<Map> mData;

    static void Trim(MapUsage& data) {
        for (auto it = data->begin(); it != data->end();) {
            if (!IsThreadAlive(it->first)) {
                data->erase(it++);
            } else {
                it++;
            }
        }
    }
};

template <Pointer T>
class PerThread<T> {
  public:
    T Get() {
        return mData.Use([](auto data) {
            if (auto it = data->find(GetThreadUniqueId()); it != data->end()) {
                return it->second.get();
            }
            return nullptr;
        });
    }

    void Set(T value) {
        mData.Use([&](auto data) {
            // Each time a new thread sets a value, attempt to clean up terminated thread values
            // before adding the new one.
            Trim(data);

            auto& [it, inserted] = data->try_emplace(GetThreadUniqueId(), value);
            if (!inserted) {
                it->second = value;
            }
        });
    }

    void Trim() {
        mData.Use([](auto data) { Trim(data); });
    }

  private:
    // The lock is to prevent any rehashing from racing with reading a from the map.
    using Map = absl::flat_hash_map<ThreadUniqueId, std::remove_pointer_t<T>>;
    using MapUsage = MutexProtected<Map>::Usage;

    MutexProtected<Map> mData;

    static void Trim(MapUsage& data) {
        for (auto it = data->begin(); it != data->end();) {
            if (!IsThreadAlive(it->first)) {
                data->erase(it++);
            } else {
                it++;
            }
        }
    }
};

}  // namespace dawn

#endif  // SRC_DAWN_COMMON_THREADLOCAL_H_
