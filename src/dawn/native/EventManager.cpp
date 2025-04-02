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

#include "absl/strings/str_format.h"
#include "dawn/common/Assert.h"
#include "dawn/common/FutureUtils.h"
#include "dawn/common/Log.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/Device.h"
#include "dawn/native/Instance.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/Queue.h"
#include "dawn/native/SystemEvent.h"
#include "dawn/native/WaitAnySystemEvent.h"

namespace dawn::native {
namespace {

// Ref<TrackedEvent> plus a few extra fields needed for some implementations.
// Sometimes they'll be unused, but that's OK; it simplifies code reuse.
struct TrackedFutureWaitInfo {
    FutureID futureID;
    Ref<EventManager::TrackedEvent> event;
    // Used by EventManager::ProcessPollEvents
    size_t indexInInfos;
    // Used by EventManager::ProcessPollEvents and ::WaitAny
    bool ready;
};

// Wrapper around an iterator to yield event specific objects and a pointer
// to the ready bool. We pass this into helpers so that they can extract
// the event specific objects and get pointers to the ready status - without
// allocating duplicate storage to store the objects and ready bools.
template <typename Traits>
class WrappingIterator {
  public:
    // Specify required iterator traits.
    using value_type = Traits::value_type;
    using difference_type = typename Traits::WrappedIter::difference_type;
    using iterator_category = typename Traits::WrappedIter::iterator_category;
    using pointer = value_type*;
    using reference = value_type&;

    WrappingIterator() = default;
    WrappingIterator(const WrappingIterator&) = default;
    WrappingIterator& operator=(const WrappingIterator&) = default;

    explicit WrappingIterator(Traits::WrappedIter wrappedIt) : mWrappedIt(wrappedIt) {}

    bool operator!=(const WrappingIterator& rhs) const { return rhs.mWrappedIt != mWrappedIt; }
    bool operator==(const WrappingIterator& rhs) const { return rhs.mWrappedIt == mWrappedIt; }

    difference_type operator-(const WrappingIterator& rhs) const {
        return mWrappedIt - rhs.mWrappedIt;
    }

    WrappingIterator operator+(difference_type rhs) const {
        return WrappingIterator{mWrappedIt + rhs};
    }

    WrappingIterator& operator++() {
        ++mWrappedIt;
        return *this;
    }

    value_type operator*() { return Traits::Deref(mWrappedIt); }

  private:
    Traits::WrappedIter mWrappedIt;
};

struct ExtractSystemEventAndReadyStateTraits {
    using WrappedIter = std::vector<TrackedFutureWaitInfo>::iterator;
    using value_type = std::pair<const SystemEventReceiver&, bool*>;

    static value_type Deref(const WrappedIter& wrappedIt) {
        if (wrappedIt->event->IsCompleted()) {
            return {SystemEventReceiver::CreateAlreadySignaled(), &wrappedIt->ready};
        }
        if (wrappedIt->event->IsNonProgressing()) {
            return {SystemEventReceiver::CreateNonProgressing(), &wrappedIt->ready};
        }
        if (wrappedIt->event->IsWaitListEvent()) {
            return {wrappedIt->event->GetWaitListEvent()->WaitAsync(), &wrappedIt->ready};
        }
        DAWN_ASSERT(wrappedIt->event->IsSystemEvent());
        return {
            wrappedIt->event->GetSystemEvent()->GetOrCreateSystemEventReceiver(),
            &wrappedIt->ready,
        };
    }
};

using SystemEventAndReadyStateIterator = WrappingIterator<ExtractSystemEventAndReadyStateTraits>;

struct ExtractWaitListEventAndReadyStateTraits {
    using WrappedIter = std::vector<TrackedFutureWaitInfo>::iterator;
    using value_type = std::pair<Ref<WaitListEvent>, bool*>;

    static value_type Deref(const WrappedIter& wrappedIt) {
        if (wrappedIt->event->IsCompleted()) {
            return {WaitListEvent::CreateAlreadySignaled(), &wrappedIt->ready};
        }
        if (wrappedIt->event->IsNonProgressing()) {
            return {WaitListEvent::CreateNonProgressing(), &wrappedIt->ready};
        }
        DAWN_ASSERT(wrappedIt->event->IsWaitListEvent());
        return {wrappedIt->event->GetWaitListEvent(), &wrappedIt->ready};
    }
};

using WaitListEventAndReadyStateIterator =
    WrappingIterator<ExtractWaitListEventAndReadyStateTraits>;

// Wait/poll the queue for futures in range [begin, end). `waitSerial` should be
// the serial after which at least one future should be complete. All futures must
// have completion data of type QueueAndSerial.
// Returns true if at least one future is ready. If no futures are ready or the wait
// timed out, returns false.
bool WaitQueueSerialsImpl(DeviceBase* device,
                          QueueBase* queue,
                          ExecutionSerial waitSerial,
                          std::vector<TrackedFutureWaitInfo>::iterator begin,
                          std::vector<TrackedFutureWaitInfo>::iterator end,
                          Nanoseconds timeout) {
    bool success = false;
    // TODO(dawn:1662): Make error handling thread-safe.
    auto deviceLock(device->GetScopedLock());
    if (device->ConsumedError([&]() -> MaybeError {
            if (waitSerial > queue->GetLastSubmittedCommandSerial()) {
                // Serial has not been submitted yet. Submit it now.
                // TODO(dawn:1413): This doesn't need to be a full tick. It just needs to
                // flush work up to `waitSerial`. This should be done after the
                // ExecutionQueue / ExecutionContext refactor.
                queue->ForceEventualFlushOfCommands();
                DAWN_TRY(device->Tick());
            }
            // Check the completed serial.
            ExecutionSerial completedSerial = queue->GetCompletedCommandSerial();
            if (completedSerial < waitSerial) {
                if (timeout > Nanoseconds(0)) {
                    // Wait on the serial if it hasn't passed yet.
                    DAWN_TRY_ASSIGN(success, queue->WaitForQueueSerial(waitSerial, timeout));
                }
                // Update completed serials.
                DAWN_TRY(queue->CheckPassedSerials());
                completedSerial = queue->GetCompletedCommandSerial();
            }
            // Poll futures for completion.
            for (auto it = begin; it != end; ++it) {
                ExecutionSerial serial;
                if (it->event->IsCompleted()) {
                    serial = kBeginningOfGPUTime;
                } else if (it->event->IsNonProgressing()) {
                    serial = kMaxExecutionSerial;
                } else {
                    DAWN_ASSERT(it->event->IsQueueAndSerial());
                    serial = it->event->GetQueueAndSerial().completionSerial;
                }
                if (serial <= completedSerial) {
                    success = true;
                    it->ready = true;
                }
            }
            return {};
        }())) {
        // There was an error. Pending submit may have failed or waiting for fences
        // may have lost the device. The device is lost inside ConsumedError.
        // Mark all futures as ready.
        for (auto it = begin; it != end; ++it) {
            it->ready = true;
        }
        success = true;
    }
    return success;
}

// We can replace the std::vector& when std::span is available via C++20.
wgpu::WaitStatus WaitImpl(const InstanceBase* instance,
                          std::vector<TrackedFutureWaitInfo>& futures,
                          Nanoseconds timeout) {
    bool success = false;

    if (timeout == Nanoseconds(0)) {
        for (auto& future : futures) {
            if (future.event->PollForCompletion()) {
                success = true;
                future.ready = true;
            }
        }
        return success ? wgpu::WaitStatus::Success : wgpu::WaitStatus::TimedOut;
    }

    bool foundCompleted = false;
    bool foundSystemEvent = false;
    bool foundWaitListEvent = false;
    bool foundMultipleQueues = false;

    QueueBase* waitQueue = nullptr;
    ExecutionSerial lowestWaitSerial = kMaxExecutionSerial;

    for (auto it = futures.begin(); it != futures.end(); ++it) {
        if (it->event->IsCompleted()) {
            it->ready = true;
            foundCompleted = true;
        }
        if (it->event->IsSystemEvent()) {
            foundSystemEvent = true;
        }
        if (it->event->IsWaitListEvent()) {
            foundWaitListEvent = true;
        }
        if (it->event->IsQueueAndSerial()) {
            const auto& queueAndSerial = it->event->GetQueueAndSerial();
            if (!waitQueue) {
                waitQueue = queueAndSerial.queue.Get();
                lowestWaitSerial = queueAndSerial.completionSerial;
            } else if (waitQueue == queueAndSerial.queue.Get()) {
                lowestWaitSerial = std::min(lowestWaitSerial, queueAndSerial.completionSerial);
            } else {
                foundMultipleQueues = true;
            }
        }
    }

    // We can't have a mix of system/wait-list events and queue events, but we can have a mix of
    // completed/non-progressing events with other event types since they can be lazily upgraded.
    if (foundMultipleQueues || (waitQueue && (foundWaitListEvent || foundSystemEvent))) {
        // Multi-source wait is unsupported.
        // TODO(dawn:2062): Implement support for this when the device supports it.
        // It should eventually gather the lowest serial from the queue(s), transform them
        // into completion events, and wait on all of the events. Then for any queues that
        // saw a completion, poll all futures related to that queue for completion.
        instance->EmitLog(WGPULoggingType_Error,
                          "Mixed source waits with timeouts are not currently supported.");
        return wgpu::WaitStatus::Error;
    }

    // Early out if there are any already completed events (we've already updated those).
    if (foundCompleted) {
        success = true;
    } else if (foundSystemEvent) {
        // Can upgrade completed, non-progressing and wait list events to system events.
        success |= WaitAnySystemEvent(SystemEventAndReadyStateIterator{futures.begin()},
                                      SystemEventAndReadyStateIterator{futures.end()}, timeout);
    } else if (foundWaitListEvent) {
        // Can upgrade completed and non-progressing events to wait list events.
        success |=
            WaitListEvent::WaitAny(WaitListEventAndReadyStateIterator{futures.begin()},
                                   WaitListEventAndReadyStateIterator{futures.end()}, timeout);
    } else if (waitQueue) {
        // Can upgrade completed and non-progressing events to queue and serial waits.
        DAWN_ASSERT(!foundSystemEvent && !foundWaitListEvent);
        success |= WaitQueueSerialsImpl(waitQueue->GetDevice(), waitQueue, lowestWaitSerial,
                                        futures.begin(), futures.end(), timeout);
    }
    return success ? wgpu::WaitStatus::Success : wgpu::WaitStatus::TimedOut;
}

// Reorder callbacks to enforce callback ordering required by the spec.
// Returns an iterator just past the last ready callback.
auto PrepareReadyCallbacks(std::vector<TrackedFutureWaitInfo>& futures) {
    // Partition the futures so the following sort looks at fewer elements.
    auto endOfReady =
        std::partition(futures.begin(), futures.end(),
                       [](const TrackedFutureWaitInfo& future) { return future.ready; });

    // Enforce the following rules from https://gpuweb.github.io/gpuweb/#promise-ordering:
    // 1. For some GPUQueue q, if p1 = q.onSubmittedWorkDone() is called before
    //    p2 = q.onSubmittedWorkDone(), then p1 must settle before p2.
    // 2. For some GPUQueue q and GPUBuffer b on the same GPUDevice,
    //    if p1 = b.mapAsync() is called before p2 = q.onSubmittedWorkDone(),
    //    then p1 must settle before p2.
    //
    // To satisfy the rules, we need only put lower future ids before higher future
    // ids. Lower future ids were created first.
    std::sort(futures.begin(), endOfReady,
              [](const TrackedFutureWaitInfo& a, const TrackedFutureWaitInfo& b) {
                  return a.futureID < b.futureID;
              });

    return endOfReady;
}

}  // namespace

// EventManager

EventManager::EventManager(InstanceBase* instance) : mInstance(instance) {
    // Construct the non-movable inner struct.
    mEvents.Use([&](auto events) { (*events).emplace(); });
}

EventManager::~EventManager() {
    DAWN_ASSERT(IsShutDown());
}

MaybeError EventManager::Initialize(const UnpackedPtr<InstanceDescriptor>& descriptor) {
    if (descriptor) {
        mTimedWaitAnyEnable = descriptor->capabilities.timedWaitAnyEnable;
        mTimedWaitAnyMaxCount =
            std::max(kTimedWaitAnyMaxCountDefault, descriptor->capabilities.timedWaitAnyMaxCount);
    }
    if (mTimedWaitAnyMaxCount > kTimedWaitAnyMaxCountDefault) {
        // We don't yet support a higher timedWaitAnyMaxCount because it would be complicated
        // to implement on Windows, and it isn't that useful to implement only on non-Windows.
        return DAWN_VALIDATION_ERROR("Requested timedWaitAnyMaxCount is not supported");
    }

    return {};
}

void EventManager::ShutDown() {
    mEvents.Use([&](auto events) { (*events).reset(); });
}

bool EventManager::IsShutDown() const {
    return mEvents.Use([](auto events) { return !events->has_value(); });
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
        bool isReady = false;
        if (event->IsCompleted()) {
            isReady = true;
        }
        if (event->IsSystemEvent()) {
            isReady = event->GetSystemEvent()->IsSignaled();
        }
        if (event->IsWaitListEvent()) {
            isReady = event->GetWaitListEvent()->IsSignaled();
        }
        if (event->IsQueueAndSerial()) {
            const auto& queueAndSerial = event->GetQueueAndSerial();
            isReady = queueAndSerial.completionSerial <=
                      queueAndSerial.queue->GetCompletedCommandSerial();
        }
        if (isReady) {
            event->EnsureComplete(EventCompletionType::Ready);
            return futureID;
        }
    }

    mEvents.Use([&](auto events) {
        if (!events->has_value()) {
            return;
        }
        if (event->mCallbackMode != wgpu::CallbackMode::WaitAnyOnly) {
            FutureID lastProcessedEventID = mLastProcessEventID.load(std::memory_order_acquire);
            while (lastProcessedEventID < futureID &&
                   !mLastProcessEventID.compare_exchange_weak(lastProcessedEventID, futureID,
                                                              std::memory_order_acq_rel)) {
            }
        }
        (*events)->emplace(futureID, std::move(event));
    });
    return futureID;
}

void EventManager::SetFutureReady(TrackedEvent* event) {
    if (event->IsSystemEvent()) {
        event->GetSystemEvent()->Signal();
    }
    if (event->IsWaitListEvent()) {
        event->GetWaitListEvent()->Signal();
    }
    if (event->IsQueueAndSerial()) {
        auto& queueAndSerial = std::get<QueueAndSerial>(event->mCompletionData);
        queueAndSerial.completionSerial = queueAndSerial.queue->GetCompletedCommandSerial();
    }

    // Sometimes, events might become ready before they are even tracked. This can happen because
    // tracking is ordered to uphold callback ordering, but events may become ready in any order. If
    // the event is spontaneous, it will be completed when it is tracked.
    if (event->mFutureID == kNullFutureID) {
        return;
    }

    // Handle spontaneous completion now.
    if (event->mCallbackMode == wgpu::CallbackMode::AllowSpontaneous) {
        mEvents.Use([&](auto events) {
            if (!events->has_value()) {
                return;
            }
            (*events)->erase(event->mFutureID);
        });
        event->EnsureComplete(EventCompletionType::Ready);
    }
}

bool EventManager::ProcessPollEvents() {
    DAWN_ASSERT(!IsShutDown());

    std::vector<TrackedFutureWaitInfo> futures;
    wgpu::WaitStatus waitStatus;
    bool hasProgressingEvents = false;
    FutureID lastProcessEventID;
    mEvents.Use([&](auto events) {
        // Iterate all events and record poll events and spontaneous events since they are both
        // allowed to be completed in the ProcessPoll call. Note that spontaneous events are allowed
        // to trigger anywhere which is why we include them in the call.
        lastProcessEventID = mLastProcessEventID.load(std::memory_order_acquire);
        futures.reserve((*events)->size());
        for (auto& [futureID, event] : **events) {
            if (event->mCallbackMode != wgpu::CallbackMode::WaitAnyOnly) {
                // Figure out if there are any progressing events. If we only have non-progressing
                // events, we need to return false to indicate that there isn't any polling work to
                // be done.
                if (!event->IsNonProgressing()) {
                    hasProgressingEvents = true;
                }

                futures.push_back(TrackedFutureWaitInfo{futureID, event, 0, false});
            }
        }
    });

    // If there wasn't anything to wait on, we can skip the wait and just return.
    if (futures.size() == 0) {
        return false;
    }

    // Wait and enforce callback ordering.
    waitStatus = WaitImpl(mInstance, futures, Nanoseconds(0));
    if (waitStatus == wgpu::WaitStatus::TimedOut) {
        return hasProgressingEvents;
    }
    DAWN_ASSERT(waitStatus == wgpu::WaitStatus::Success);

    // Enforce callback ordering.
    auto readyEnd = PrepareReadyCallbacks(futures);
    bool hasIncompleteEvents = readyEnd != futures.end();

    // For all the futures we are about to complete, first ensure they're untracked.
    mEvents.Use([&](auto events) {
        for (auto it = futures.begin(); it != readyEnd; ++it) {
            (*events)->erase(it->futureID);
        }
    });

    // Finally, call callbacks while comparing the last process event id with any new ones that may
    // have been created via the callbacks.
    for (auto it = futures.begin(); it != readyEnd; ++it) {
        it->event->EnsureComplete(EventCompletionType::Ready);
    }
    // Note that in the event of all progressing events completing, but there exists non-progressing
    // events, we will return true one extra time.
    return hasIncompleteEvents ||
           (lastProcessEventID != mLastProcessEventID.load(std::memory_order_acquire));
}

wgpu::WaitStatus EventManager::WaitAny(size_t count, FutureWaitInfo* infos, Nanoseconds timeout) {
    DAWN_ASSERT(!IsShutDown());

    // Validate for feature support.
    if (timeout > Nanoseconds(0)) {
        if (!mTimedWaitAnyEnable) {
            mInstance->EmitLog(WGPULoggingType_Error,
                               "Timeout waits are either not enabled or not supported.");
            return wgpu::WaitStatus::Error;
        }
        if (count > mTimedWaitAnyMaxCount) {
            mInstance->EmitLog(
                WGPULoggingType_Error,
                absl::StrFormat("Number of futures to wait on (%d) exceeds maximum (%d).", count,
                                mTimedWaitAnyMaxCount));
            return wgpu::WaitStatus::Error;
        }
        // UnsupportedMixedSources is validated later, in WaitImpl.
    }

    if (count == 0) {
        return wgpu::WaitStatus::Success;
    }

    // Look up all of the futures and build a list of `TrackedFutureWaitInfo`s.
    std::vector<TrackedFutureWaitInfo> futures;
    futures.reserve(count);
    bool anyCompleted = false;
    mEvents.Use([&](auto events) {
        FutureID firstInvalidFutureID = mNextFutureID;
        for (size_t i = 0; i < count; ++i) {
            FutureID futureID = infos[i].future.id;

            // Check for cases that are undefined behavior in the API contract.
            DAWN_ASSERT(futureID != 0);
            DAWN_ASSERT(futureID < firstInvalidFutureID);

            // Try to find the event.
            auto it = (*events)->find(futureID);
            if (it == (*events)->end()) {
                infos[i].completed = true;
                anyCompleted = true;
            } else {
                infos[i].completed = false;
                TrackedEvent* event = it->second.Get();
                futures.push_back(TrackedFutureWaitInfo{futureID, event, i, false});
            }
        }
    });
    // If any completed, return immediately.
    if (anyCompleted) {
        return wgpu::WaitStatus::Success;
    }
    // Otherwise, we should have successfully looked up all of them.
    DAWN_ASSERT(futures.size() == count);

    wgpu::WaitStatus waitStatus = WaitImpl(mInstance, futures, timeout);
    if (waitStatus != wgpu::WaitStatus::Success) {
        return waitStatus;
    }

    // Enforce callback ordering
    auto readyEnd = PrepareReadyCallbacks(futures);

    // For any futures that we're about to complete, first ensure they're untracked. It's OK if
    // something actually isn't tracked anymore (because it completed elsewhere while waiting.)
    mEvents.Use([&](auto events) {
        for (auto it = futures.begin(); it != readyEnd; ++it) {
            (*events)->erase(it->futureID);
        }
    });

    // Finally, call callbacks and update return values.
    for (auto it = futures.begin(); it != readyEnd; ++it) {
        // Set completed before calling the callback.
        infos[it->indexInInfos].completed = true;
        it->event->EnsureComplete(EventCompletionType::Ready);
    }

    return wgpu::WaitStatus::Success;
}

// EventManager::TrackedEvent

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode,
                                         Ref<WaitListEvent> completionEvent)
    : mCallbackMode(callbackMode), mCompletionData(std::move(completionEvent)) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode,
                                         Ref<SystemEvent> completionEvent)
    : mCallbackMode(callbackMode), mCompletionData(std::move(completionEvent)) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode,
                                         QueueBase* queue,
                                         ExecutionSerial completionSerial)
    : mCallbackMode(callbackMode), mCompletionData(QueueAndSerial{queue, completionSerial}) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode, Completed tag)
    : mCallbackMode(callbackMode), mCompletionData(tag) {}

EventManager::TrackedEvent::TrackedEvent(wgpu::CallbackMode callbackMode, NonProgressing tag)
    : mCallbackMode(callbackMode), mCompletionData(tag) {}

EventManager::TrackedEvent::~TrackedEvent() {
    DAWN_ASSERT(mFutureID != kNullFutureID);
    DAWN_ASSERT(mCompleted);
}

Future EventManager::TrackedEvent::GetFuture() const {
    return {mFutureID};
}

bool EventManager::TrackedEvent::PollForCompletion() {
    if (IsCompleted()) {
        return true;
    }
    if (IsNonProgressing()) {
        return false;
    }
    if (IsQueueAndSerial()) {
        bool ready = false;
        const auto& queueAndSerial = GetQueueAndSerial();
        QueueBase* queue = queueAndSerial.queue.Get();
        DeviceBase* device = queue->GetDevice();
        const ExecutionSerial waitSerial = queueAndSerial.completionSerial;

        // TODO(dawn:1662): Make error handling thread-safe.
        auto deviceLock(device->GetScopedLock());
        if (device->ConsumedError([&]() -> MaybeError {
                if (waitSerial > queue->GetLastSubmittedCommandSerial()) {
                    // Serial has not been submitted yet. Submit it now.
                    // TODO(dawn:1413): This doesn't need to be a full tick. It just needs to
                    // flush work up to `waitSerial`. This should be done after the
                    // ExecutionQueue / ExecutionContext refactor.
                    queue->ForceEventualFlushOfCommands();
                    DAWN_TRY(device->Tick());
                }
                // Check the completed serial.
                ExecutionSerial completedSerial = queue->GetCompletedCommandSerial();
                if (completedSerial < waitSerial) {
                    // Update completed serials.
                    DAWN_TRY(queue->CheckPassedSerials());
                    completedSerial = queue->GetCompletedCommandSerial();
                }
                if (waitSerial <= completedSerial) {
                    ready = true;
                }
                return {};
            }())) {
            // There was an error. Pending submit may have failed or waiting for fences may have
            // lost the device. The device is lost inside ConsumedError. Mark the future as ready.
            ready = true;
        }
        return ready;
    }
    if (IsSystemEvent()) {
        return GetSystemEvent()->IsSignaled();
    }
    DAWN_ASSERT(IsWaitListEvent());
    return GetWaitListEvent()->IsSignaled();
}

void EventManager::TrackedEvent::EnsureComplete(EventCompletionType completionType) {
    bool alreadyComplete = mCompleted.exchange(true);
    if (!alreadyComplete) {
        Complete(completionType);
    }
}

}  // namespace dawn::native
