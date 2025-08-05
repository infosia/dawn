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

#include "dawn/native/ExecutionQueue.h"

#include <atomic>
#include <utility>
#include <vector>

#include "dawn/common/Atomic.h"
#include "dawn/native/Device.h"

namespace dawn::native {

ExecutionSerial ExecutionQueueBase::GetPendingCommandSerial() const {
    return ExecutionSerial(mLastSubmittedSerial.load(std::memory_order_acquire) + 1);
}

ExecutionSerial ExecutionQueueBase::GetLastSubmittedCommandSerial() const {
    return ExecutionSerial(mLastSubmittedSerial.load(std::memory_order_acquire));
}

ExecutionSerial ExecutionQueueBase::GetCompletedCommandSerial() const {
    return ExecutionSerial(mCompletedSerial.load(std::memory_order_acquire));
}

MaybeError ExecutionQueueBase::WaitForQueueSerial(ExecutionSerial waitSerial, Nanoseconds timeout) {
    std::optional<DeviceGuard> deviceGuard;
    if (GetDevice()->IsToggleEnabled(Toggle::WaitIsThreadSafe)) {
        // If thread-safe, WaitFor* functions should never be holding the device lock when called.
        DAWN_ASSERT(GetDevice()->IsNotLockedByCurrentThread());
    } else {
        GetDevice()->GetGuard(deviceGuard);
    }

    {
        auto deviceGuard = GetDevice()->GetGuard();
        if (waitSerial > GetLastSubmittedCommandSerial()) {
            // Serial has not been submitted yet. Submit it now.
            DAWN_TRY(EnsureCommandsFlushed(waitSerial));
        }
    }

    // Serial is already complete.
    if (waitSerial <= GetCompletedCommandSerial()) {
        return {};
    }

    if (timeout > Nanoseconds(0)) {
        // Wait on the serial if it hasn't passed yet.
        ExecutionSerial completedSerial = kWaitSerialTimeout;
        DAWN_TRY_ASSIGN(completedSerial, WaitForQueueSerialImpl(waitSerial, timeout));
        UpdateCompletedSerialTo(completedSerial);
        return {};
    }
    return UpdateCompletedSerial();
}

MaybeError ExecutionQueueBase::CheckPassedSerials() {
    ExecutionSerial completedSerial;
    DAWN_TRY_ASSIGN(completedSerial, CheckAndUpdateCompletedSerials());

    DAWN_ASSERT(completedSerial <=
                ExecutionSerial(mLastSubmittedSerial.load(std::memory_order_acquire)));

    // Atomically set mCompletedSerial to completedSerial if completedSerial is larger.
    FetchMax(mCompletedSerial, uint64_t(completedSerial));
    return {};
}

MaybeError ExecutionQueueBase::UpdateCompletedSerial() {
    ExecutionSerial completedSerial;
    DAWN_TRY_ASSIGN(completedSerial, CheckAndUpdateCompletedSerials());

    DAWN_ASSERT(completedSerial <=
                ExecutionSerial(mLastSubmittedSerial.load(std::memory_order_acquire)));
    UpdateCompletedSerialTo(completedSerial);
    return {};
}

void ExecutionQueueBase::TrackSerialTask(ExecutionSerial serial, Task&& task) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (serial > GetCompletedCommandSerial()) {
            mWaitingTasks.Enqueue(std::move(task), serial);
            return;
        }
    }
    task();
}

void ExecutionQueueBase::UpdateCompletedSerialTo(ExecutionSerial completedSerial) {
    std::vector<Task> tasks;
    {
        std::unique_lock<std::mutex> lock(mMutex);

        // We update the completed serial as soon as possible before waiting for callback rights so
        // that we almost always process as many callbacks as possible.
        FetchMax(mCompletedSerial, uint64_t(completedSerial));

        if (mCallingCallbacks) {
            mCv.wait(lock, [&] { return !mCallingCallbacks; });
        }

        auto serial = GetCompletedCommandSerial();
        for (auto task : mWaitingTasks.IterateUpTo(serial)) {
            tasks.push_back(std::move(task));
        }
        mWaitingTasks.ClearUpTo(serial);

        if (tasks.size() > 0) {
            mCallingCallbacks = true;
        }
    }

    // Call the callbacks without holding the lock on the ExecutionQueue to avoid lock-inversion
    // issues when dealing with potential re-entrant callbacks.
    for (auto task : tasks) {
        task();
    }

    if (tasks.size() > 0) {
        std::lock_guard<std::mutex> lock(mMutex);
        mCallingCallbacks = false;
        mCv.notify_all();
    }
}

MaybeError ExecutionQueueBase::EnsureCommandsFlushed(ExecutionSerial serial) {
    DAWN_ASSERT(serial <= GetPendingCommandSerial());
    if (serial > GetLastSubmittedCommandSerial()) {
        ForceEventualFlushOfCommands();
        DAWN_TRY(SubmitPendingCommands());
        DAWN_ASSERT(serial <= GetLastSubmittedCommandSerial());
    }
    return {};
}

MaybeError ExecutionQueueBase::SubmitPendingCommands() {
    if (mInSubmit) {
        return {};
    }

    mInSubmit = true;
    auto result = SubmitPendingCommandsImpl();
    mInSubmit = false;

    return result;
}

void ExecutionQueueBase::AssumeCommandsComplete() {
    // Bump serials so any pending callbacks can be fired.
    // TODO(crbug.com/dawn/831): This is called during device destroy, which is not
    // thread-safe yet. Two threads calling destroy would race setting these serials.
    ExecutionSerial completed =
        ExecutionSerial(mLastSubmittedSerial.fetch_add(1u, std::memory_order_release) + 1);
    UpdateCompletedSerialTo(completed);
}

void ExecutionQueueBase::IncrementLastSubmittedCommandSerial() {
    mLastSubmittedSerial.fetch_add(1u, std::memory_order_release);
}

bool ExecutionQueueBase::HasScheduledCommands() const {
    return mLastSubmittedSerial.load(std::memory_order_acquire) >
               mCompletedSerial.load(std::memory_order_acquire) ||
           HasPendingCommands();
}

// All prevously submitted works at the moment will supposedly complete at this serial.
// Internally the serial is computed according to whether frontend and backend have pending
// commands. There are 4 cases of combination:
//   1) Frontend(No), Backend(No)
//   2) Frontend(No), Backend(Yes)
//   3) Frontend(Yes), Backend(No)
//   4) Frontend(Yes), Backend(Yes)
// For case 1, we don't need the serial to track the task as we can ack it right now.
// For case 2 and 4, there will be at least an eventual submission, so we can use
// 'GetPendingCommandSerial' as the serial.
// For case 3, we can't use 'GetPendingCommandSerial' as it won't be submitted surely. Instead we
// use 'GetLastSubmittedCommandSerial', which must be fired eventually.
ExecutionSerial ExecutionQueueBase::GetScheduledWorkDoneSerial() const {
    return HasPendingCommands() ? GetPendingCommandSerial() : GetLastSubmittedCommandSerial();
}

}  // namespace dawn::native
