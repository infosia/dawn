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

#include "dawn/native/WaitListEvent.h"

#include <array>

#include "dawn/common/Ref.h"

namespace dawn::native {

bool WaitListEvent::State::IsSignaled() const {
    return remainingSignalCount == 0;
}

WaitListEvent::WaitListEvent(uint64_t requiredSignalCount) : mState(requiredSignalCount) {}

WaitListEvent::~WaitListEvent() = default;

bool WaitListEvent::IsSignaled() const {
    return mState.Use([](auto state) { return state->IsSignaled(); });
}

void WaitListEvent::Signal() {
    mState.Use([](auto state) {
        DAWN_ASSERT(state->remainingSignalCount > 0);
        state->remainingSignalCount -= 1;

        if (state->IsSignaled()) {
            for (SyncWaiter* w : std::move(state->syncWaiters)) {
                w->Use([](auto done) { *done = true; });
            }
            for (auto& sender : std::move(state->asyncWaiters)) {
                std::move(sender).Signal();
            }
        }
    });
}

bool WaitListEvent::Wait(Nanoseconds timeout) {
    bool ready = false;
    std::array<std::pair<Ref<WaitListEvent>, bool*>, 1> events{{{this, &ready}}};
    return WaitListEvent::WaitAny(events.begin(), events.end(), timeout);
}

SystemEventReceiver WaitListEvent::WaitAsync() {
    return mState.Use([](auto state) {
        if (state->IsSignaled()) {
            return SystemEventReceiver::CreateAlreadySignaled();
        }
        SystemEventPipeSender sender;
        SystemEventReceiver receiver;
        std::tie(sender, receiver) = CreateSystemEventPipe();
        state->asyncWaiters.push_back(std::move(sender));
        return receiver;
    });
}

}  // namespace dawn::native
