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
    std::unique_lock<std::mutex> lock(mMutex);
    mCompletionConditionVariable.wait(lock,
                                      [this]() { return mState == AsyncTaskState::Completed; });
}

void AsyncTask::AddCompletionCallback(AsyncTaskCompletionCallback completionCallback) {
    std::scoped_lock<std::mutex> lock(mMutex);

    // If this task has already completed, call the completion callback immediately.
    if (mState == AsyncTaskState::Completed) {
        completionCallback();
        return;
    }

    mCompletionCallbacks.push_back(completionCallback);
}

void AsyncTask::Run() {
    {
        AsyncTaskState prevState = mState.exchange(AsyncTaskState::Running);
        DAWN_ASSERT(prevState == AsyncTaskState::Pending);
    }

    mTask();

    // AsyncTask may have a much longer life time than the task itself.
    // Reset it to release any references that were captured.
    mTask = nullptr;

    // Grab the completion callbacks while locked but call them outside the lock.
    std::vector<AsyncTaskCompletionCallback> completionCallbacks;
    {
        std::scoped_lock<std::mutex> lock(mMutex);
        AsyncTaskState prevState = mState.exchange(AsyncTaskState::Completed);
        DAWN_ASSERT(prevState == AsyncTaskState::Running);
        completionCallbacks = std::move(mCompletionCallbacks);
        mCompletionCallbacks.clear();
    }

    for (auto completionCallback : completionCallbacks) {
        completionCallback();
    }

    mCompletionConditionVariable.notify_all();
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

void AsyncTaskManager::PostConstructedTask(const std::vector<Ref<AsyncTask>>& dependencies,
                                           Ref<AsyncTask> asyncTask) {
    // We insert new waitableTask objects into mPendingTasks in main thread (PostTask()),
    // and we may remove waitableTask objects from mPendingTasks in either main thread
    // (WaitAllPendingTasks()) or sub-thread (TaskCompleted), so mPendingTasks should be
    // protected by a mutex.
    // Hold the mutex until the task is fully posted otherwise it could complete and be deleted
    // from mPending tasks before it is fully initialized.
    WaitableTask* waitableTask = mPendingTasks.Use([](auto pendingTasks) {
        // If these allocations becomes expensive, we can slab-allocate tasks.
        auto iter = pendingTasks->emplace(std::make_unique<WaitableTask>());

        // Should never be inserting the same value twice.
        DAWN_ASSERT(iter.second);

        return iter.first->get();
    });

    waitableTask->taskManager = this;
    waitableTask->asyncTask = asyncTask;
    if (dependencies.empty()) {
        // Force the task to start now by setting only one dependency and immediately notifying the
        // task that its dependency has finished. This will move it into the running tasks list.
        waitableTask->pendingDependencyCount = 1;
        HandleTaskDependencyCompletion(waitableTask);
    } else {
        waitableTask->pendingDependencyCount = dependencies.size();
        for (auto& dependency : dependencies) {
            dependency->AddCompletionCallback(
                [this, waitableTask]() { HandleTaskDependencyCompletion(waitableTask); });
        }
    }
}

void AsyncTaskManager::HandleTaskCompletion(WaitableTask* task) {
    DAWN_ASSERT(task);
    DAWN_ASSERT(task->asyncTask->GetState() == AsyncTaskState::Completed);

    mRunningTasks.Use([&task](auto runningTasks) { return runningTasks->erase(task); });
}

void AsyncTaskManager::HandleTaskDependencyCompletion(WaitableTask* task) {
    DAWN_ASSERT(task);
    DAWN_ASSERT(task->asyncTask->GetState() == AsyncTaskState::Pending);

    uint64_t prevDependencyCount =
        task->pendingDependencyCount.fetch_sub(1, std::memory_order_release);
    if (prevDependencyCount == 1) {
        // Move the task unique pointer from the set of pending tasks to running tasks.
        std::unique_ptr<WaitableTask> taskPtr;
        mPendingTasks.Use(
            [&](auto pendingTasks) { taskPtr = std::move(pendingTasks->extract(task).value()); });
        DAWN_ASSERT(taskPtr);
        mRunningTasks.Use([&](auto runningTasks) {
            auto [_, inserted] = runningTasks->emplace(std::move(taskPtr));

            // Should never be inserting the same value twice.
            DAWN_ASSERT(inserted);

            // Hold the running task set mutex while writing to waitableEvent. The task could run
            // and try to modify itself while this write is ongoing.
            task->waitableEvent = mWorkerTaskPool->PostWorkerTask(RunTask, task);
        });
    }
}

void AsyncTaskManager::WaitAllPendingTasks() {
    auto getOnePendingTask = [&]() {
        return mRunningTasks.Use([this](auto runningTasks) {
            // First look for a task in running tasks
            if (!runningTasks->empty()) {
                return runningTasks->begin()->get()->asyncTask;
            }

            // Next, check for a task in the pending tasks. Hold both locks so no tasks transition
            // between sets.
            return mPendingTasks.Use([](auto pendingTasks) -> Ref<AsyncTask> {
                if (!pendingTasks->empty()) {
                    return pendingTasks->begin()->get()->asyncTask;
                }

                return nullptr;
            });
        });
    };

    // Continuously grab a task which has not completed and wait on it until no tasks are left.
    while (Ref<AsyncTask> pendingTask = getOnePendingTask()) {
        pendingTask->Wait();
    }
}

bool AsyncTaskManager::HasPendingTasks() {
    return mRunningTasks.Use([this](auto runningTasks) {
        if (!runningTasks->empty()) {
            return true;
        }

        return mPendingTasks.Use([](auto pendingTasks) { return !pendingTasks->empty(); });
    });
}

void AsyncTaskManager::RunTask(void* task) {
    WaitableTask* waitableTask = static_cast<WaitableTask*>(task);
    waitableTask->asyncTask->Run();
    waitableTask->taskManager->HandleTaskCompletion(waitableTask);
}

}  // namespace dawn::native
