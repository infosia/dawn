// Copyright 2019 The Dawn & Tint Authors
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

#include "dawn/native/ErrorScope.h"

#include <utility>

#include "dawn/common/Assert.h"
#include "dawn/common/StringViewUtils.h"

namespace dawn::native {

namespace {

wgpu::ErrorType ErrorFilterToErrorType(wgpu::ErrorFilter filter) {
    switch (filter) {
        case wgpu::ErrorFilter::Validation:
            return wgpu::ErrorType::Validation;
        case wgpu::ErrorFilter::OutOfMemory:
            return wgpu::ErrorType::OutOfMemory;
        case wgpu::ErrorFilter::Internal:
            return wgpu::ErrorType::Internal;
    }
    DAWN_UNREACHABLE();
}

}  // namespace

ErrorScope::ErrorScope(wgpu::ErrorFilter errorFilter)
    : mMatchedErrorType(ErrorFilterToErrorType(errorFilter)) {}

ErrorScope::ErrorScope(wgpu::ErrorType error, std::string_view message)
    : mMatchedErrorType(error), mCapturedError(error), mErrorMessage(message) {}

wgpu::ErrorType ErrorScope::GetErrorType() const {
    return mCapturedError;
}

WGPUStringView ErrorScope::GetErrorMessage() const {
    if (!mErrorMessage.empty()) {
        return ToOutputStringView(mErrorMessage);
    }
    return kEmptyOutputStringView;
}

bool ErrorScope::HasPendingAsyncTasks() const {
    return !mAsyncTasks.empty();
}

Ref<WaitListEvent> ErrorScope::CreateWaitEventForAsyncTasks() const {
    Ref<WaitListEvent> event = AcquireRef(new WaitListEvent(mAsyncTasks.size()));
    for (auto task : mAsyncTasks) {
        task->AddCompletionCallback([eventRef = Ref<WaitListEvent>(event)] { eventRef->Signal(); });
    }
    return event;
}

void ErrorScope::HandleAsyncTaskErrors() {
    for (auto task : mAsyncTasks) {
        DAWN_ASSERT(task->GetState() == AsyncTaskState::Finished);
    }
}

void ErrorScope::CaptureError(wgpu::ErrorType type, std::string_view message) {
    DAWN_ASSERT(type == mMatchedErrorType);
    // Record the error if the scope doesn't have one yet.
    if (mCapturedError == wgpu::ErrorType::NoError) {
        mCapturedError = type;
        mErrorMessage = message;
    }
}

ErrorScopeStack::ErrorScopeStack() = default;

ErrorScopeStack::~ErrorScopeStack() = default;

void ErrorScopeStack::Push(wgpu::ErrorFilter filter) {
    mScopes.push_back(ErrorScope(filter));
}

ErrorScope ErrorScopeStack::Pop() {
    DAWN_ASSERT(!mScopes.empty());
    ErrorScope scope = std::move(mScopes.back());
    mScopes.pop_back();
    return scope;
}

bool ErrorScopeStack::Empty() const {
    return mScopes.empty();
}

bool ErrorScopeStack::HandleError(wgpu::ErrorType type, std::string_view message) {
    for (auto it = mScopes.rbegin(); it != mScopes.rend(); ++it) {
        if (it->mMatchedErrorType != type) {
            // Error filter does not match. Move on to the next scope.
            continue;
        }

        // Filter matches.
        it->CaptureError(type, message);
        return true;
    }

    // The error was not captured.
    return false;
}

std::set<wgpu::ErrorType> ErrorScopeStack::HandleErrorGeneratingAsyncTask(
    Ref<ErrorGeneratingAsyncTask> task) {
    // TODO: Combine this logic with ErrorFilterToErrorType
    std::set<wgpu::ErrorType> unhandledErrorTypes{
        wgpu::ErrorType::Validation,
        wgpu::ErrorType::OutOfMemory,
        wgpu::ErrorType::Internal,
    };

    for (auto it = mScopes.rbegin(); it != mScopes.rend() && !unhandledErrorTypes.empty(); ++it) {
        if (unhandledErrorTypes.contains(it->mMatchedErrorType)) {
            it->mAsyncTasks.push_back(task);
            unhandledErrorTypes.erase(it->mMatchedErrorType);
        }
    }

    return unhandledErrorTypes;
}

}  // namespace dawn::native
