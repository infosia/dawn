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

    bool IsSignaled() const;
    void Signal();
    bool Wait(Nanoseconds timeout);
    SystemEventReceiver WaitAsync();

    template <typename It>
    static bool WaitAny(It eventAndReadyStateBegin, It eventAndReadyStateEnd, Nanoseconds timeout);

  private:
    ~WaitListEvent() override;

    struct SyncWaiter {
        std::condition_variable cv;
        std::mutex mutex;
        bool waitDone = false;
    };

    mutable std::mutex mMutex;
    std::atomic_bool mSignaled;
    std::vector<raw_ptr<SyncWaiter>> mSyncWaiters;
    std::vector<SystemEventPipeSender> mAsyncWaiters;
};

template <typename It>
bool WaitListEvent::WaitAny(It eventAndReadyStateBegin,
                            It eventAndReadyStateEnd,
                            Nanoseconds timeout) {
    static_assert(std::is_base_of_v<std::random_access_iterator_tag,
                                    typename std::iterator_traits<It>::iterator_category>);
    static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type,
                                 std::pair<Ref<WaitListEvent>, bool*>>);

    const size_t count = std::distance(eventAndReadyStateBegin, eventAndReadyStateEnd);
    if (count == 0) {
        return false;
    }

    bool foundSignaled = false;
    for (auto it = eventAndReadyStateBegin; it != eventAndReadyStateEnd; ++it) {
        const auto& event = (*it).first;
        if (event->IsSignaled()) {
            bool* isReady = (*it).second;
            *isReady = true;
            foundSignaled = true;
        }
    }
    if (foundSignaled) {
        return true;
    }

    // Sort the events by address to get a globally consistent order.
    struct EventAndIndex {
        WaitListEvent* event;
        size_t origIndex;
    };
    std::vector<EventAndIndex> events(count);

    for (size_t i = 0; i < count; i++) {
        events[i].event = (*(eventAndReadyStateBegin + i)).first.Get();
        events[i].origIndex = i;
    }

    std::sort(events.begin(), events.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.event < rhs.event; });

    // Acquire locks in order and enqueue our waiter.
    SyncWaiter waiter;
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[i].event;
        // Skip over multiple waits on the same event.
        if (i > 0 && event == events[i - 1].event) {
            continue;
        }
        event->mMutex.lock();
        event->mSyncWaiters.push_back(&waiter);
    }

    // We have acquired locks for all the events we're going to wait on and enqueued our waiter;
    // lock our waiter since it could be woken up as soon as the event is unlocked and unlock the
    // events in reverse order now to prevent lock order inversion.
    std::unique_lock<std::mutex> waiterLock(waiter.mutex);
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[count - 1 - i].event;
        // Skip over multiple waits on the same event.
        if (i > 0 && event == events[count - i].event) {
            continue;
        }
        event->mMutex.unlock();
    }

    // Any values larger than those representatable by std::chrono::nanoseconds will be treated as
    // infinite waits - in particular this covers values greater than INT64_MAX.
    static constexpr uint64_t kMaxDurationNanos = std::chrono::nanoseconds::max().count();
    if (timeout > Nanoseconds(kMaxDurationNanos)) {
        waiter.cv.wait(waiterLock, [&waiter]() { return waiter.waitDone; });
    } else {
        waiter.cv.wait_for(waiterLock, std::chrono::nanoseconds(static_cast<uint64_t>(timeout)),
                           [&waiter]() { return waiter.waitDone; });
    }

    // Remove our waiter from the events.
    for (size_t i = 0; i < count; i++) {
        WaitListEvent* event = events[i].event;
        // We could be woken by the CV before the atomic release store to `mSignaled` is visible -
        // locking the mutex ensures that the atomic acquire load in `IsSignaled()` sees the correct
        // value. This is correct even with multiple waits on the same event since we release the
        // lock at the end of this scope so there's no deadlock.
        std::lock_guard<std::mutex> eventLock(event->mMutex);
        if (event->IsSignaled()) {
            bool* isReady = (*(eventAndReadyStateBegin + events[i].origIndex)).second;
            *isReady = true;
            foundSignaled = true;
        }
        // We can still avoid iterating over `mSyncWaiters` for duplicate waits on the same event.
        if (i > 0 && event == events[i - 1].event) {
            continue;
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
