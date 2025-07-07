// Copyright 2021 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_ASYNCTASK_H_
#define SRC_DAWN_NATIVE_ASYNCTASK_H_

#include <functional>
#include <memory>
#include <mutex>

#include "absl/container/flat_hash_set.h"
#include "dawn/common/Ref.h"
#include "dawn/common/RefCounted.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::platform {
class WaitableEvent;
class WorkerTaskPool;
}  // namespace dawn::platform

namespace dawn::native {

// TODO(crbug.com/dawn/826): we'll add additional things to AsyncTask in the future, like
// Cancel() and RunNow(). Cancelling helps avoid running the task's body when we are just
// shutting down the device. RunNow() could be used for more advanced scenarios, for example
// always doing ShaderModule initial compilation asynchronously, but being able to steal the
// task if we need it for synchronous pipeline compilation.
// using AsyncTask = std::function<void()>;

enum class AsyncTaskState : uint8_t {
    Pending,
    Running,
    Finished,
};

class AsyncTask : public RefCounted {
  public:
    AsyncTask();

    AsyncTaskState GetState() const { return mState; }

  private:
    // Friends with the task manager to privately manage when tasks are executed.
    friend class AsyncTaskManager;
    void Run();

    virtual void RunImpl() = 0;

    std::atomic<AsyncTaskState> mState = AsyncTaskState::Pending;
};

class AsyncTaskManager {
  public:
    explicit AsyncTaskManager(dawn::platform::WorkerTaskPool* workerTaskPool);

    template <typename TaskType, class... Args>
    Ref<TaskType> PostTask(Args&&... args) {
        Ref<TaskType> task = AcquireRef(new TaskType(std::forward<Args>(args)...));
        PostConstructedTask(task);
        return task;
    }

    void WaitAllPendingTasks();
    bool HasPendingTasks();

  private:
    struct WaitableTask {
        Ref<AsyncTask> asyncTask;
        raw_ptr<AsyncTaskManager> taskManager;
        std::unique_ptr<dawn::platform::WaitableEvent> waitableEvent;
    };

    void PostConstructedTask(Ref<AsyncTask> task);

    static void RunTask(void* task);
    void HandleTaskCompletion(WaitableTask* task);

    std::mutex mPendingTasksMutex;
    absl::flat_hash_set<std::unique_ptr<WaitableTask>> mPendingTasks;
    raw_ptr<dawn::platform::WorkerTaskPool> mWorkerTaskPool;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_ASYNCTASK_H_
