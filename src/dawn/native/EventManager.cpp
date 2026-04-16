// Copyright 2023 The Dawn & Tint Authors
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

#include "dawn/native/EventManager.h"

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/str_format.h"
#include "dawn/common/Assert.h"
#include "dawn/common/Atomic.h"
#include "dawn/common/FutureUtils.h"
#include "dawn/common/Log.h"
#include "dawn/common/NonMovable.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/Device.h"
#include "dawn/native/Instance.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/Queue.h"
#include "dawn/native/WaitAnySystemEvent.h"

namespace dawn::native {
namespace {

// Wait/poll queues with given `timeout`. `queueWaitSerials` should contain per queue, the serial up
// to which we should flush the queue if needed. Note that keys are WeakRef<QueueBase> which
// actually means the keys are not based on the QueueBase pointer, but a pointer to metadata that is
// guaranteed to be unique and alive. This ensures that each queue will be represented for multi
// source validation.
struct QueueWaitSerialsMaps {
    // A map from the queue to strong ref of the queue if possible. This avoids promoting
    // (acquire/releasing locks) for the same queue multiple times.
    absl::flat_hash_map<WeakRef<QueueBase>, Ref<QueueBase>> strongRefs;
    absl::flat_hash_map<Ref<QueueBase>, ExecutionSerial> minWaitSerials;
};

// Helper that updates the serial maps with the new `queueAndSerial`. Returns true if
// `queueAndSerial` was handled as a queue event, otherwise returns false. Note, queue events may
// not be handled as queue events if the last reference of the queue has already been dropped, or if
// the queue supports spontaneous queue events. In those case, we can handle the queue event as if
// it were just a normal event.
bool UpdateQueueWaitSerialsMap(QueueWaitSerialsMaps& queueWaitSerialMaps,
                               const QueueAndSerial* queueAndSerial) {
    DAWN_ASSERT(queueAndSerial);

    auto refIt = queueWaitSerialMaps.strongRefs.find(queueAndSerial->queue);
    bool refInserted = false;
    if (refIt == queueWaitSerialMaps.strongRefs.end()) {
        std::tie(refIt, refInserted) = queueWaitSerialMaps.strongRefs.insert(
            {queueAndSerial->queue, queueAndSerial->queue.Promote()});
        DAWN_ASSERT(refInserted);
    }
    auto queue = refIt->second;

    // If the queue failed promotion, then it means that the work is all already done, so we don't
    // need to do anything here.
    if (queue == nullptr) {
        return false;
    }

    // If the device backing the queue supports spontaneous queue events, we don't need to handle
    // them separately from other events.
    if (queue->GetDevice()->IsToggleEnabled(Toggle::SpontaneousQueueEvents)) {
        return false;
    }

    // Otherwise, track the queue and the minimum wait serial to satisfy the wait condition.
    ExecutionSerial waitSerial = queueAndSerial->completionSerial.load(std::memory_order_acquire);
    auto [serialIt, serialInserted] =
        queueWaitSerialMaps.minWaitSerials.insert({queue, waitSerial});
    if (!serialInserted) {
        serialIt->second = std::min(serialIt->second, waitSerial);
    }
    return true;
}

void WaitQueueSerials(const QueueWaitSerialsMaps& queueWaitSerialMaps, Nanoseconds timeout) {
    // Poll/wait on queues up to the lowest wait serial, but do this once per queue instead of
    // per event so that events with same serial complete at the same time instead of racing.
    for (const auto& [queue, waitSerial] : queueWaitSerialMaps.minWaitSerials) {
        [[maybe_unused]] bool hadError = queue->GetDevice()->ConsumedError(
            queue->WaitForQueueSerial(waitSerial, timeout), "waiting for work in %s.", queue.Get());
    }
}

struct AlreadyCompletedEvent final : public EventManager::TrackedEvent {
    explicit AlreadyCompletedEvent(wgpu::CallbackMode callbackMode)
        : TrackedEvent(callbackMode, TrackedEvent::Completed{}) {}
    ~AlreadyCompletedEvent() override { EnsureComplete(EventCompletionType::Shutdown); }
    void Complete(EventCompletionType) override {}
};

}  // namespace

class EventManager::Waiter : public NonMovable {
  public:
    void Signal() {
        mState.Use([](auto state) {
            state->signaled = true;

            // If we are backed by a system event, we need to make sure to signal the event as well.
            if (!state->pipe) {
                return;
            }
            auto sender = std::move(state->pipe->first);
            if (sender.IsValid()) {
                std::move(sender).Signal();
            }
        });
    }

    bool Wait(Nanoseconds timeout) const {
        return mState.Use([&](auto state) {
            // If we have a pipe, we should not be calling Wait, and should instead be accessing the
            // receiver and waiting on it.
            DAWN_CHECK(!state->pipe);
            return state.WaitFor(timeout, [](const State& s) { return s.signaled; });
        });
    }

    const SystemEventReceiver& GetOrCreateSystemEventReceiver() {
        return mState.Use([](auto state) {
            if (!state->pipe) {
                if (state->signaled) {
                    state->pipe = {SystemEventPipeSender{},
                                   SystemEventReceiver::CreateAlreadySignaled()};
                } else {
                    state->pipe = CreateSystemEventPipe();
                }
            }
            return std::cref(state->pipe->second);
        });
    }

  private:
    struct State {
        bool signaled = false;
        std::optional<std::pair<SystemEventPipeSender, SystemEventReceiver>> pipe;
    };
    MutexCondVarProtected<State> mState;
};

// EventManager

EventManager::EventManager(InstanceBase* instance) : mInstance(instance) {}

EventManager::~EventManager() {
    mEventState.Use([&](auto state) {
        // If the event manager is being destroyed, it shouldn't be possible to have waits in
        // flight.
        DAWN_CHECK(state->waiters.empty());

        // For all non-spontaneous events, call their callbacks now.
        for (auto& [futureID, event] : state->events) {
            if (event->mCallbackMode != wgpu::CallbackMode::AllowSpontaneous) {
                event->EnsureComplete(EventCompletionType::Shutdown);
            }
        }
    });
}

FutureID EventManager::TrackEvent(Ref<TrackedEvent>&& event) {
    if (!ValidateCallbackMode(ToAPI(event->mCallbackMode))) {
        mInstance->EmitLog(WGPULoggingType_Error,
                           absl::StrFormat("Invalid callback mode: %d",
                                           static_cast<uint32_t>(event->mCallbackMode)));
        return kNullFutureID;
    }

    FutureID futureID = mNextFutureID++;
    event->mFutureID = futureID;

    // Handle the event now if it's spontaneous and ready.
    if (event->mCallbackMode == wgpu::CallbackMode::AllowSpontaneous) {
        if (event->IsReadyToComplete()) {
            event->EnsureComplete(EventCompletionType::Ready);
            return futureID;
        }
    }

    if (const auto* queueAndSerial = event->GetIfQueueAndSerial()) {
        if (auto q = queueAndSerial->queue.Promote()) {
            q->TrackSerialTask(QueuePriority::UserVisible, queueAndSerial->completionSerial,
                               [this, event]() {
                                   // If this is executed, we can be sure that the raw pointer
                                   // to this EventManager is valid because the task is ran by
                                   // the Queue and:
                                   //   Queue -[refs]->
                                   //     Device -[refs]->
                                   //       Adapter -[refs]->
                                   //         Instance -[owns]->
                                   //           EventManager.
                                   SetFutureReady(event.Get());
                               });
        }
    }

    mEventState.Use([&](auto state) {
        if (event->mCallbackMode != wgpu::CallbackMode::WaitAnyOnly) {
            FetchMax(mLastProcessEventID, futureID);
        }
        state->events.emplace(futureID, std::move(event));
    });
    return futureID;
}

void EventManager::SetFutureReady(Ref<TrackedEvent> event) {
    event->SetReadyToComplete();

    // Sometimes, events might become ready before they are even tracked. This can happen
    // because tracking is ordered to uphold callback ordering, but events may become ready in
    // any order. If the event is spontaneous, it will be completed when it is tracked.
    if (event->mFutureID == kNullFutureID) {
        return;
    }

    // Signal all relevant waiters that the event has become ready.
    mEventState.Use([&](auto state) {
        if (auto node = state->waiters.extract(event->mFutureID)) {
            for (Waiter* waiter : node.mapped()) {
                waiter->Signal();
            }
        }
    });

    // Handle spontaneous completion now.
    if (event->mCallbackMode == wgpu::CallbackMode::AllowSpontaneous) {
        // Since we use the presence of the event to indicate whether the callback has already
        // been called in WaitAny when searching for the matching FutureID, untrack the event
        // after calling the callbacks to ensure that we can't race on two different threads
        // waiting on the same future. Note that only one thread will actually call the callback
        // since EnsureComplete is thread safe.
        event->EnsureComplete(EventCompletionType::Ready);
        mEventState.Use([&](auto state) { state->events.erase(event->mFutureID); });
    }
}

bool EventManager::ProcessPollEvents() {
    QueueWaitSerialsMaps queueWaitSerialMaps;
    SortedEventMap completables;
    bool hasProgressingEvents = false;
    FutureID lastProcessEventID;

    mEventState.Use([&](auto state) {
        // Iterate all events and record poll events and spontaneous events since they are both
        // allowed to be completed in the ProcessPoll call. Note that spontaneous events are
        // allowed to trigger anywhere which is why we include them in the call.
        lastProcessEventID = mLastProcessEventID.load(std::memory_order_acquire);
        for (const auto& [futureID, event] : state->events) {
            if (event->mCallbackMode == wgpu::CallbackMode::WaitAnyOnly) {
                continue;
            }

            if (event->IsReadyToComplete()) {
                completables.emplace(futureID, event);
                continue;
            }
            if (event->IsProgressing()) {
                hasProgressingEvents = true;
            }

            // Record queue's and their min serial to force a submit if applicable.
            if (const auto* queueAndSerial = event->GetIfQueueAndSerial()) {
                UpdateQueueWaitSerialsMap(queueWaitSerialMaps, queueAndSerial);
            }
        }
    });

    // This call is a no-op if `queueWaitSerialMaps` is empty, otherwise, it ensures that the
    // lowest serial work is submitted on each queue.
    WaitQueueSerials(queueWaitSerialMaps, Nanoseconds(0));

    // Complete the events that are completable.
    for (auto& [_, event] : completables) {
        if (event) {
            event->EnsureComplete(EventCompletionType::Ready);
        }
    }

    // Since we use the presence of the event to indicate whether the callback has already been
    // called in WaitAny when searching for the matching FutureID, untrack the event after
    // calling the callbacks to ensure that we can't race on two different threads waiting on
    // the same future. Note that only one thread will actually call the callback since
    // EnsureComplete is thread safe.
    mEventState.Use([&](auto state) {
        for (auto& [futureID, _] : completables) {
            state->events.erase(futureID);
        }
    });

    return hasProgressingEvents ||
           (lastProcessEventID != mLastProcessEventID.load(std::memory_order_acquire));
}

wgpu::WaitStatus EventManager::WaitAny(std::span<FutureWaitInfo> infos, Nanoseconds timeout) {
    bool foundNonQueueEvent = false;
    QueueWaitSerialsMaps queueWaitSerialMaps;
    SortedEventMap completables;

    auto ProcessInfos = [&](Waiter* waiter) {
        FutureID firstInvalidFutureID = mNextFutureID;
        mEventState.Use([&](auto state) {
            for (auto& info : infos) {
                FutureID futureID = info.future.id;
                info.completed = false;

                // Check for cases that are undefined behavior in the API contract.
                DAWN_ASSERT(futureID != 0);
                DAWN_ASSERT(futureID < firstInvalidFutureID);

                // Try to find the event, if we don't find it, we can assume that it has already
                // been completed so we can signal the waiter here.
                auto eventIt = state->events.find(futureID);
                if (eventIt == state->events.end()) {
                    if (waiter != nullptr) {
                        waiter->Signal();
                    }
                    continue;
                }

                // Otherwise, we want to add the waiter for it.
                if (waiter != nullptr) {
                    state->waiters[futureID].push_back(waiter);
                }

                // We also handle coalescing the queue events per queue on the lowest serial,
                // and tracking the different types of events for multi-source validation later.
                TrackedEvent* event = eventIt->second.Get();
                if (event->IsReadyToComplete() && waiter != nullptr) {
                    waiter->Signal();
                }
                if (const auto* queueAndSerial = event->GetIfQueueAndSerial()) {
                    foundNonQueueEvent |=
                        !UpdateQueueWaitSerialsMap(queueWaitSerialMaps, queueAndSerial);
                } else {
                    foundNonQueueEvent = true;
                }
            }
        });
    };

    auto PollInfos = [&](bool shouldComplete, Waiter* waiter) {
        mEventState.Use([&](auto state) {
            for (auto& info : infos) {
                FutureID futureID = info.future.id;

                if (shouldComplete) {
                    auto eventIt = state->events.find(futureID);
                    if (eventIt == state->events.end()) {
                        info.completed = true;
                        completables.emplace(futureID, nullptr);
                    } else {
                        Ref<TrackedEvent> event = eventIt->second;
                        if (event->IsReadyToComplete()) {
                            info.completed = true;
                            completables.emplace(futureID, std::move(event));
                        }
                    }
                }

                // Remove the waiter if relevant.
                if (waiter != nullptr) {
                    std::erase(state->waiters[futureID], waiter);
                    if (state->waiters[futureID].empty()) {
                        state->waiters.erase(futureID);
                    }
                }
            }
        });
    };

    if (timeout == Nanoseconds(0)) {
        ProcessInfos(/*waiter=*/nullptr);
        WaitQueueSerials(queueWaitSerialMaps, timeout);
        PollInfos(/*shouldComplete=*/true, /*waiter=*/nullptr);
    } else {
        // Add the waiter to each of the events and initialize a cleanup routine to ensure that we
        // always remove the waiter at the end of this scope, optionally completing events if
        // `shouldComplete` is set to true.
        Waiter waiter;
        ProcessInfos(&waiter);
        bool shouldComplete = false;
        absl::Cleanup removeWaiter = [&]() { PollInfos(shouldComplete, &waiter); };

        // There's currently a couple of different cases to handle here depending on the
        // capabilities of the backends. Each case is documented in their respective clause below,
        // and each case is mutually exclusive so at most one clause should ever run.

        // When `queueWaitSerialMaps` is empty, that means that all events are waitable via the
        // waiter, either because they are not queue events, or because they are queue events that
        // are equivalent to normal events.
        if (queueWaitSerialMaps.minWaitSerials.empty()) {
            waiter.Wait(timeout);
            shouldComplete = true;
        }

        // When there is exactly 1 queue in `queueWaitSerialMaps` and there are no other non-queue
        // events, we can just rely on `WaitQueueSerials` to handle a timed wait on that queue.
        if (queueWaitSerialMaps.minWaitSerials.size() == 1 && !foundNonQueueEvent) {
            WaitQueueSerials(queueWaitSerialMaps, timeout);
            shouldComplete = true;
        }

        // Otherwise, we either have multiple queues and/or a mix of queue and non-queue events. In
        // this case, we want to try to promote each queue's lowest serial event and the Waiter to
        // SystemEvents, and use system primitives to wait for anything. If we are unable to promote
        // any of the queue events, that means that we cannot successfully handle this multi-source
        // wait, so log the error and exit.
        if (!shouldComplete) {
            bool ready = false;
            std::vector<SystemEventReceiver> receivers;
            receivers.resize(queueWaitSerialMaps.minWaitSerials.size());
            std::vector<std::pair<const SystemEventReceiver&, bool*>> waitables;
            waitables.reserve(queueWaitSerialMaps.minWaitSerials.size() + 1);

            auto maybeError = [&]() -> MaybeError {
                size_t index = 0;
                for (const auto& [queue, waitSerial] : queueWaitSerialMaps.minWaitSerials) {
                    DAWN_TRY_ASSIGN(*(receivers.begin() + index),
                                    queue->GetSystemEventReceiverForSerial(waitSerial));
                    waitables.push_back({*(receivers.begin() + index), &ready});
                    index++;
                }
                return {};
            }();
            if (maybeError.IsError()) {
                // Create a nicely formatted error in this case to notify the user that multi-source
                // wait is not available in this case.
                auto error =
                    DAWN_INTERNAL_ERROR("Mixed source waits with timeouts is not supported.");
                error->AppendBackendMessage(maybeError.AcquireError()->GetMessage());
                mInstance->EmitLog(WGPULoggingType_Error, error->GetFormattedMessage());
                return wgpu::WaitStatus::Error;
            }

            if (foundNonQueueEvent) {
                waitables.push_back({waiter.GetOrCreateSystemEventReceiver(), &ready});
            }

            shouldComplete = WaitAnySystemEvent(waitables.begin(), waitables.end(), timeout);
        }
    }

    // Complete the events that are completable.
    for (auto& [_, event] : completables) {
        if (event) {
            event->EnsureComplete(EventCompletionType::Ready);
        }
    }

    // Since we use the presence of the event to indicate whether the callback has already been
    // called in WaitAny when searching for the matching FutureID, untrack the event after
    // calling the callbacks to ensure that we can't race on two different threads waiting on
    // the same future. Note that only one thread will actually call the callback since
    // EnsureComplete is thread safe.
    mEventState.Use([&](auto state) {
        for (auto& [futureID, _] : completables) {
            state->events.erase(futureID);
        }
    });

    return !completables.empty() ? wgpu::WaitStatus::Success : wgpu::WaitStatus::TimedOut;
}

// QueueAndSerial

QueueAndSerial::QueueAndSerial(QueueBase* q, ExecutionSerial serial)
    : queue(GetWeakRef(q)), completionSerial(serial) {}

ExecutionSerial QueueAndSerial::GetCompletedSerial() const {
    if (auto q = queue.Promote()) {
        return q->GetCompletedCommandSerial();
    }
    return completionSerial;
}

// EventManager::TrackedEvent

Ref<EventManager::TrackedEvent> EventManager::TrackedEvent::CreateAlreadyCompletedEvent(
    EventManager* eventManager,
    wgpu::CallbackMode callbackMode) {
    Ref<TrackedEvent> event = AcquireRef(new AlreadyCompletedEvent(callbackMode));
    eventManager->TrackEvent(Ref<TrackedEvent>(event));
    return event;
}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode, bool value)
    : mCallbackMode(callbackMode), mIsReadyToComplete(value) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode,
                                         QueueBase* queue,
                                         ExecutionSerial completionSerial)
    : mCallbackMode(callbackMode), mQueueAndSerial(std::in_place, queue, completionSerial) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode, Completed tag)
    : mCallbackMode(callbackMode), mIsReadyToComplete(true) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode, NonProgressing tag)
    : mCallbackMode(callbackMode), mIsProgressing(false) {}

EventManager::TrackedEvent::~TrackedEvent() {
    DAWN_ASSERT(mFutureID != kNullFutureID);
#if defined(DAWN_ENABLE_ASSERTS)
    std::call_once(mFlag, []() { DAWN_ASSERT(false); });
#endif
}

Future EventManager::TrackedEvent::GetFuture() const {
    return {mFutureID};
}

QueueAndSerial* EventManager::TrackedEvent::GetIfQueueAndSerial() {
    return mQueueAndSerial ? &(*mQueueAndSerial) : nullptr;
}

const QueueAndSerial* EventManager::TrackedEvent::GetIfQueueAndSerial() const {
    return mQueueAndSerial ? &(*mQueueAndSerial) : nullptr;
}

bool EventManager::TrackedEvent::IsReadyToComplete() const {
    // Currently there are only two types of events, queue events and simple state events.
    if (mIsReadyToComplete) {
        return true;
    }
    if (const auto* queueAndSerial = GetIfQueueAndSerial()) {
        return queueAndSerial->completionSerial <= queueAndSerial->GetCompletedSerial();
    }

    return false;
}

void EventManager::TrackedEvent::SetReadyToComplete() {
    mIsReadyToComplete = true;
}

void EventManager::TrackedEvent::EnsureComplete(EventCompletionType completionType) {
    std::call_once(mFlag, [&]() { Complete(completionType); });
}

}  // namespace dawn::native
