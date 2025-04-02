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

#ifndef SRC_DAWN_NATIVE_WAITLISTEVENT_H_
#define SRC_DAWN_NATIVE_WAITLISTEVENT_H_

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

#include "dawn/common/RefCounted.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/SystemEvent.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native {

class WaitListEvent : public RefCounted {
  public:
    WaitListEvent();

    static Ref<WaitListEvent> CreateAlreadySignaled();
    static Ref<WaitListEvent> CreateNonProgressing();

    bool IsSignaled() const;
    void Signal();
    bool Wait(Nanoseconds timeout);
    SystemEventReceiver WaitAsync();

    template <typename It>
    static bool WaitAny(It begin, It end, Nanoseconds timeout);

  private:
    ~WaitListEvent() override;

    struct SyncWaiter {
        std::condition_variable cv;
        std::mutex mutex;
        bool waitDone = false;
    };

    mutable std::recursive_mutex mMutex;
    bool mSignaled = false;
    bool mNonProgressing = false;
    std::vector<raw_ptr<SyncWaiter>> mSyncWaiters;
    std::vector<SystemEventPipeSender> mAsyncWaiters;
};

template <typename It>
bool WaitListEvent::WaitAny(It begin, It end, Nanoseconds timeout) {
    static_assert(std::is_base_of_v<std::random_access_iterator_tag,
                                    typename std::iterator_traits<It>::iterator_category>);
    static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type,
                                 std::pair<Ref<WaitListEvent>, bool*>>);

    const size_t count = std::distance(begin, end);
    if (count == 0) {
        return false;
    }

    // Sort the events by address to get a globally consistent order.
    struct EventAndIndex {
        WaitListEvent* event;
        size_t origIndex;
    };
    std::vector<EventAndIndex> events(count);

    for (size_t i = 0; i < count; i++) {
        events[i].event = (*(begin + i)).first.Get();
        events[i].origIndex = i;
    }

    std::sort(events.begin(), events.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.event < rhs.event; });

    // Acquire locks in order and enqueue our waiter.
    SyncWaiter waiter;
    bool foundSignaled = false;
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[i].event;
        event->mMutex.lock();
        if (event->mSignaled) {
            foundSignaled = true;
            *(*(begin + events[i].origIndex)).second = true;
        }
    }

    // If an event was already signaled, unlock in reverse order and return.
    if (foundSignaled) {
        for (size_t i = 0; i < count; i++) {
            WaitListEvent* event = events[count - 1 - i].event;
            event->mMutex.unlock();
        }
        return true;
    }

    // We have acquired locks for all the events we're going to wait on - enqueue our waiter now and
    // unlock in reverse order.
    std::unique_lock waiterLock(waiter.mutex);
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[count - 1 - i].event;
        event->mSyncWaiters.push_back(&waiter);
        event->mMutex.unlock();
    }

    // Treat UINT64_MAX as a special case since that's most commonly used for "infinite" waits.
    if (timeout == Nanoseconds(std::numeric_limits<uint64_t>::max())) {
        waiter.cv.wait(waiterLock, [&waiter]() { return waiter.waitDone; });
    } else {
        // Convert to double to prevent overflow as std::chrono::duration stores int64_t internally
        // but our Nanoseconds durations are uint64_t.
        std::chrono::duration<double, std::nano> duration(static_cast<double>(uint64_t(timeout)));
        waiter.cv.wait_for(waiterLock, duration, [&waiter]() { return waiter.waitDone; });
    }

    // Remove our waiter from the events.
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[i].event;
        std::lock_guard eventLock(event->mMutex);
        if (event->mSignaled) {
            foundSignaled = true;
            *(*(begin + events[i].origIndex)).second = true;
        }
        event->mSyncWaiters.erase(
            std::remove(event->mSyncWaiters.begin(), event->mSyncWaiters.end(), &waiter),
            event->mSyncWaiters.end());
    }

    DAWN_ASSERT(!waiter.waitDone || foundSignaled);
    return foundSignaled;
}

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_WAITLISTEVENT_H_
