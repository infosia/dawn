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

#include "dawn/common/MutexProtected.h"
#include "dawn/common/RefCounted.h"
#include "dawn/common/Time.h"
#include "dawn/native/SystemEvent.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native {

class WaitListEvent : public RefCounted {
  public:
    explicit WaitListEvent(uint64_t requiredSignalCount = 1);

    bool IsSignaled() const;
    void Signal();
    bool Wait(Nanoseconds timeout);
    SystemEventReceiver WaitAsync();

    template <typename It>
    static bool WaitAny(It eventAndReadyStateBegin, It eventAndReadyStateEnd, Nanoseconds timeout);

  private:
    ~WaitListEvent() override;

    using SyncWaiter = MutexCondVarProtected<bool>;

    struct State {
        uint64_t remainingSignalCount;
        std::vector<raw_ptr<SyncWaiter>> syncWaiters;
        std::vector<SystemEventPipeSender> asyncWaiters;

        bool IsSignaled() const;
    };
    MutexProtected<State> mState;
};

template <typename It>
bool WaitListEvent::WaitAny(It eventAndReadyStateBegin,
                            It eventAndReadyStateEnd,
                            Nanoseconds timeout)
    // This method conditionally locks a set of events, and then unlocks them.
    // The thread-safety analysis does not handle conditional locking, so
    // we turn off the analysis to avoid spurious warnings.
    DAWN_NO_THREAD_SAFETY_ANALYSIS {
    static_assert(std::is_base_of_v<std::random_access_iterator_tag,
                                    typename std::iterator_traits<It>::iterator_category>);
    static_assert(std::is_same_v<typename std::iterator_traits<It>::value_type,
                                 std::pair<Ref<WaitListEvent>, bool*>>);

    const size_t count = std::distance(eventAndReadyStateBegin, eventAndReadyStateEnd);
    if (count == 0) {
        return false;
    }

    // For each event, we take the event lock and queue the waiter if the event is not already
    // signaled.
    SyncWaiter waiter;
    bool foundSignaled = false;
    for (size_t i = 0; i < count; i++) {
        auto [eventRef, readyPtr] = (*(eventAndReadyStateBegin + i));
        WaitListEvent* event = eventRef.Get();
        event->mState.Use([&](auto state) {
            // If the event was already signaled, set the flag so we don't need to wait. We still
            // iterate through the rest of the list though because duplicate events need their
            // respective |ready| value set appropriately.
            if (state->IsSignaled()) {
                *readyPtr = true;
                foundSignaled = true;
                return;
            }

            // Otherwise, we add the SyncWaiter to the event.
            state->syncWaiters.push_back(&waiter);
        });
    }

    // If an event was already signaled, we can return early out now after removing the waiter from
    // all events.
    if (foundSignaled) {
        for (size_t i = 0; i < count; i++) {
            WaitListEvent* event = (*(eventAndReadyStateBegin + i)).first.Get();
            event->mState.Use([&](auto state) { std::erase(state->syncWaiters, &waiter); });
        }
        return true;
    }

    bool result =
        waiter.Use([timeout](auto w) { return w.WaitFor(timeout, [](auto& x) { return x; }); });

    // Iterate the events one more time to update any events and erase the waiter.
    for (size_t i = 0; i < count; i++) {
        auto [eventRef, readyPtr] = (*(eventAndReadyStateBegin + i));
        WaitListEvent* event = eventRef.Get();
        event->mState.Use([&](auto state) {
            if (state->IsSignaled()) {
                *readyPtr = true;
            }
            std::erase(state->syncWaiters, &waiter);
        });
    }

    return result;
}

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_WAITLISTEVENT_H_
