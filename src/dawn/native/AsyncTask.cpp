// Copyright 2022 The Dawn & Tint Authors
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

#include "dawn/native/AsyncTask.h"

#include <utility>

#include "dawn/platform/DawnPlatform.h"

namespace dawn::native {

AsyncTask::AsyncTask(std::function<void()> task) : mTask(task) {}

void AsyncTask::Wait() {
    WaitUntilAtLeastState(AsyncTaskState::Finished);
}

void AsyncTask::WaitUntilAtLeastState(AsyncTaskState state) {
    std::unique_lock lock(mStateMutex);
    mStateConditionVariable.wait(lock, [this, state] { return mState >= state; });
}

void AsyncTask::AddCompletionCallback(std::function<void()> completionCallback) {
    std::scoped_lock<std::mutex> lock(mStateMutex);

    // If this task has already completed, call the completion callback immediately.
    if (mState == AsyncTaskState::Finished) {
        completionCallback();
        return;
    }

    mCompletionCallbacks.push_back(completionCallback);
}

void AsyncTask::Run() {
    {
        std::scoped_lock<std::mutex> lock(mStateMutex);
        DAWN_ASSERT(mState == AsyncTaskState::Pending);
        mState = AsyncTaskState::Running;
        mStateConditionVariable.notify_all();
    }
    mTask();
    {
        std::scoped_lock<std::mutex> lock(mStateMutex);
        mState = AsyncTaskState::Finished;
        for (auto completionCallback : mCompletionCallbacks) {
            completionCallback();
        }
        mCompletionCallbacks.clear();
        mStateConditionVariable.notify_all();
    }
}

ErrorGeneratingAsyncTask::ErrorGeneratingAsyncTask(std::function<MaybeError()> task)
    : AsyncTask([this, task] {
          // Wrap the task which returns a MaybeError in a void function and store the error in a
          // member.
          MaybeError taskResult = task();
          if (taskResult.IsError()) {
              mErrorData = taskResult.AcquireError();
          }
      }) {}

InternalErrorType ErrorGeneratingAsyncTask::GetErrorType() const {
    return mErrorData ? mErrorData->GetType() : InternalErrorType::None;
}

AsyncTaskManager::AsyncTaskManager(dawn::platform::WorkerTaskPool* workerTaskPool)
    : mWorkerTaskPool(workerTaskPool) {}

AsyncTaskManager::~AsyncTaskManager() {
    // Pending tasks call back into this task manager. Make sure they all finish before destructing.
    WaitAllPendingTasks();
}

void AsyncTaskManager::PostConstructedTask(Ref<AsyncTask> asyncTask) {
    // We insert new waitableTask objects into mPendingTasks in main thread (PostTask()),
    // and we may remove waitableTask objects from mPendingTasks in either main thread
    // (WaitAllPendingTasks()) or sub-thread (TaskCompleted), so mPendingTasks should be
    // protected by a mutex.
    // Hold the mutex until the task is fully posted otherwise it could complete and be deleted
    // from mPending tasks before it is fully initialized.
    std::scoped_lock<std::mutex> lock(mPendingTasksMutex);

    // If these allocations becomes expensive, we can slab-allocate tasks.
    auto iter = mPendingTasks.emplace(std::make_unique<WaitableTask>());

    // Should never be inserting the same value twice.
    DAWN_ASSERT(iter.second);

    WaitableTask* waitableTask = iter.first->get();
    waitableTask->taskManager = this;
    waitableTask->asyncTask = asyncTask;
    waitableTask->waitableEvent = mWorkerTaskPool->PostWorkerTask(RunTask, waitableTask);
}

void AsyncTaskManager::HandleTaskCompletion(WaitableTask* task) {
    std::scoped_lock<std::mutex> lock(mPendingTasksMutex);
    mPendingTasks.erase(task);
}

void AsyncTaskManager::WaitAllPendingTasks() {
    absl::flat_hash_set<std::unique_ptr<WaitableTask>> allPendingTasks;

    {
        std::scoped_lock<std::mutex> lock(mPendingTasksMutex);
        allPendingTasks.swap(mPendingTasks);
    }

    for (auto& task : allPendingTasks) {
        task->waitableEvent->Wait();
    }
}

bool AsyncTaskManager::HasPendingTasks() {
    std::scoped_lock<std::mutex> lock(mPendingTasksMutex);
    return !mPendingTasks.empty();
}

void AsyncTaskManager::RunTask(void* task) {
    WaitableTask* waitableTask = static_cast<WaitableTask*>(task);
    DAWN_ASSERT(waitableTask->asyncTask->GetState() == AsyncTaskState::Pending);
    waitableTask->asyncTask->Run();
    DAWN_ASSERT(waitableTask->asyncTask->GetState() == AsyncTaskState::Finished);
    waitableTask->taskManager->HandleTaskCompletion(waitableTask);
}

}  // namespace dawn::native
