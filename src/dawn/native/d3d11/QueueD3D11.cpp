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

#include "dawn/native/d3d11/QueueD3D11.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "dawn/common/Log.h"
#include "dawn/native/WaitAnySystemEvent.h"
#include "dawn/native/d3d/D3DError.h"
#include "dawn/native/d3d/PlatformFunctions.h"
#include "dawn/native/d3d11/BufferD3D11.h"
#include "dawn/native/d3d11/CommandBufferD3D11.h"
#include "dawn/native/d3d11/DeviceD3D11.h"
#include "dawn/native/d3d11/DeviceInfoD3D11.h"
#include "dawn/native/d3d11/PhysicalDeviceD3D11.h"
#include "dawn/native/d3d11/SharedFenceD3D11.h"
#include "dawn/native/d3d11/TextureD3D11.h"
#include "dawn/platform/DawnPlatform.h"
#include "dawn/platform/tracing/TraceEvent.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native::d3d11 {

class Queue::FlushThread {
  public:
    explicit FlushThread(Queue* queue) : mQueue(queue), mThread([this] { ThreadLoop(); }) {}
    ~FlushThread() {
        {
            std::lock_guard<std::mutex> lg(mLock);
            mRunning.store(false, std::memory_order_release);
            mQueueCv.notify_one();
        }

        mThread.join();
    }

    void EnqueueFlush(HANDLE event) {
        std::lock_guard<std::mutex> lg(mLock);
        mEventsToFlush.push_back(event);
        mQueueCv.notify_one();
    }

    void EnqueueCleanupTask(std::function<void()> task) {
        std::lock_guard<std::mutex> lg(mLock);
        mCleanupTasks.emplace_back(std::move(task));
        mQueueCv.notify_one();
    }

    void WaitUntilAllCleaupTasksCompleted() {
        std::unique_lock<std::mutex> lg(mLock);
        mCleanupTaskCompletedCv.wait(lg, [this] {
            // Keep waiting until resources list is empty.
            return mCleanupTasks.empty();
        });
    }

  private:
    void ThreadLoop() {
        auto device = ToBackend(mQueue->GetDevice());
        if (device->GetFunctions()->setThreadDescription) {
            device->GetFunctions()->setThreadDescription(GetCurrentThread(),
                                                         L"QueueD3D11::FlushingThread");
        }

        ComPtr<IDXGIDevice2> dxgiDevice2;
        if (FAILED(device->GetD3D11Device3()->QueryInterface(IID_PPV_ARGS(&dxgiDevice2)))) {
            DAWN_UNREACHABLE();
        }

        absl::InlinedVector<HANDLE, 2> handles;
        while (mRunning.load(std::memory_order_acquire)) {
            {
                std::unique_lock<std::mutex> lg(mLock);
                mQueueCv.wait(lg, [this] {
                    // Keep waiting until queue is not empty or running is stopped.
                    return !mEventsToFlush.empty() || !mCleanupTasks.empty() ||
                           !mRunning.load(std::memory_order_acquire);
                });
                handles.insert(handles.end(), mEventsToFlush.begin(), mEventsToFlush.end());
                mEventsToFlush.clear();
            }

            // Flush commands first
            for (auto handle : handles) {
                TRACE_EVENT0(device->GetPlatform(), General, "D3D11Device::Flush");
                [[maybe_unused]] auto commandContext =
                    mQueue->GetScopedPendingCommandContext(SubmitMode::Passive);

                // commandContext.Flush1(D3D11_CONTEXT_TYPE_ALL, handle);
                // Flush1 seems to have bug on older GPUs. Use IDXGIDevice2::EnqueueSetEvent
                // instead. commandContext's scope is still useful to make sure only one thread can
                // use the D3D11 commands at a time.
                dxgiDevice2->EnqueueSetEvent(handle);
            }
            handles.resize(0);

            {
                // Then release resources
                std::unique_lock<std::mutex> lg(mLock);
                for (auto& task : mCleanupTasks) {
                    std::move(task)();
                }
                mCleanupTasks.clear();
                mCleanupTaskCompletedCv.notify_all();
            }
        }
    }

    raw_ptr<Queue> mQueue;

    std::deque<HANDLE> mEventsToFlush;
    std::deque<std::function<void()>> mCleanupTasks;

    std::mutex mLock;
    std::condition_variable mQueueCv;
    std::condition_variable mCleanupTaskCompletedCv;

    std::atomic_bool mRunning{true};

    std::thread mThread;
};

ResultOrError<Ref<Queue>> Queue::Create(Device* device, const QueueDescriptor* descriptor) {
    Ref<Queue> queue = AcquireRef(new Queue(device, descriptor));
    DAWN_TRY(queue->Initialize());
    return queue;
}

MaybeError Queue::Initialize() {
    if (!GetDevice()->IsToggleEnabled(Toggle::D3D11DisableFence)) {
        DAWN_TRY(InitializeD3DFence());
    }

    mBackgroundFlushThread = std::make_unique<FlushThread>(this);

    return {};
}

MaybeError Queue::InitializeD3DFence() {
    // Create the fence.
    D3D11_FENCE_FLAG flags = D3D11_FENCE_FLAG_SHARED;

    DAWN_TRY(CheckHRESULT(
        ToBackend(GetDevice())->GetD3D11Device5()->CreateFence(0, flags, IID_PPV_ARGS(&mFence)),
        "D3D11: creating fence"));

    DAWN_TRY_ASSIGN(mSharedFence, SharedFence::Create(ToBackend(GetDevice()),
                                                      "Internal shared DXGI fence", mFence));

    return {};
}

MaybeError Queue::InitializePendingContext() {
    // Initialize mPendingCommands. After this, calls to the use the command context
    // are thread safe.
    CommandRecordingContext commandContext;
    DAWN_TRY(commandContext.Initialize(ToBackend(GetDevice())));

    mPendingCommands.Use(
        [&](auto pendingCommandContext) { *pendingCommandContext = std::move(commandContext); });

    // Configure the command context's uniform buffer. This is used to emulate builtins.
    // Creating the buffer is done outside of Initialize because it requires mPendingCommands
    // to already be initialized.
    Ref<BufferBase> uniformBuffer;
    DAWN_TRY_ASSIGN(uniformBuffer,
                    CommandRecordingContext::CreateInternalUniformBuffer(GetDevice()));
    DAWN_TRY(mPendingCommands->SetInternalUniformBuffer(std::move(uniformBuffer)));

    return {};
}

void Queue::DestroyImpl() {
    // Release the shared fence here to prevent a ref-cycle with the device, but do not destroy
    // the underlying native fence so that we can return a SharedFence on EndAccess after
    // destruction.
    mSharedFence = nullptr;

    mBackgroundFlushThread = nullptr;

    mPendingCommands.Use([&](auto pendingCommands) {
        pendingCommands->Destroy();
        mPendingCommandsNeedSubmit.store(false, std::memory_order_release);
    });
}

ResultOrError<Ref<d3d::SharedFence>> Queue::GetOrCreateSharedFence() {
    if (mSharedFence == nullptr) {
        if (!mFence) {
            DAWN_ASSERT(GetDevice()->IsToggleEnabled(Toggle::D3D11DisableFence));
            return Ref<d3d::SharedFence>{};
        }
        DAWN_ASSERT(!IsAlive());
        return SharedFence::Create(ToBackend(GetDevice()), "Internal shared DXGI fence", mFence);
    }
    return mSharedFence;
}

ScopedCommandRecordingContext Queue::GetScopedPendingCommandContext(SubmitMode submitMode) {
    return mPendingCommands.Use([&](auto commands) {
        if (submitMode == SubmitMode::Normal) {
            mPendingCommandsNeedSubmit.store(true, std::memory_order_release);
        }
        return ScopedCommandRecordingContext(std::move(commands));
    });
}

ScopedSwapStateCommandRecordingContext Queue::GetScopedSwapStatePendingCommandContext(
    SubmitMode submitMode) {
    return mPendingCommands.Use([&](auto commands) {
        if (submitMode == SubmitMode::Normal) {
            mPendingCommandsNeedSubmit.store(true, std::memory_order_release);
        }
        return ScopedSwapStateCommandRecordingContext(std::move(commands));
    });
}

MaybeError Queue::SubmitPendingCommands() {
    bool needsSubmit = mPendingCommands.Use([&](auto pendingCommands) {
        pendingCommands->ReleaseKeyedMutexes();
        return mPendingCommandsNeedSubmit.exchange(false, std::memory_order_acq_rel);
    });
    if (needsSubmit) {
        return NextSerial();
    }
    return {};
}

MaybeError Queue::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
    // CommandBuffer::Execute() will modify the state of the global immediate device context, it
    // may affect following usage of it.
    // TODO(dawn:1770): figure how if we need to track and restore the state of the immediate
    // device context.
    TRACE_EVENT_BEGIN0(GetDevice()->GetPlatform(), Recording, "CommandBufferD3D11::Execute");
    {
        auto commandContext =
            GetScopedSwapStatePendingCommandContext(QueueBase::SubmitMode::Normal);
        for (uint32_t i = 0; i < commandCount; ++i) {
            DAWN_TRY(ToBackend(commands[i])->Execute(&commandContext));
        }
    }
    DAWN_TRY(SubmitPendingCommands());
    TRACE_EVENT_END0(GetDevice()->GetPlatform(), Recording, "CommandBufferD3D11::Execute");

    return {};
}

MaybeError Queue::CheckAndMapReadyBuffers(ExecutionSerial completedSerial) {
    if (mPendingMapBuffers.Empty() || mPendingMapBuffers.FirstSerial() > completedSerial) {
        // Skip early so that we don't need to acquire the command context.
        return {};
    }

    auto commandContext = GetScopedPendingCommandContext(QueueBase::SubmitMode::Passive);

    for (const auto& bufferEntry : mPendingMapBuffers.IterateUpTo(completedSerial)) {
        DAWN_TRY(
            bufferEntry.buffer->FinalizeMap(&commandContext, completedSerial, bufferEntry.mode));
    }
    mPendingMapBuffers.ClearUpTo(completedSerial);
    return {};
}

void Queue::TrackPendingMapBuffer(Ref<Buffer>&& buffer,
                                  wgpu::MapMode mode,
                                  ExecutionSerial readySerial) {
    mPendingMapBuffers.Enqueue({buffer, mode}, readySerial);
}

MaybeError Queue::WriteBufferImpl(BufferBase* buffer,
                                  uint64_t bufferOffset,
                                  const void* data,
                                  size_t size) {
    if (size == 0) {
        // skip the empty write
        return {};
    }

    auto commandContext = GetScopedPendingCommandContext(QueueBase::SubmitMode::Normal);
    return ToBackend(buffer)->Write(&commandContext, bufferOffset, data, size);
}

MaybeError Queue::WriteTextureImpl(const TexelCopyTextureInfo& destination,
                                   const void* data,
                                   size_t dataSize,
                                   const TexelCopyBufferLayout& dataLayout,
                                   const Extent3D& writeSizePixel) {
    if (writeSizePixel.width == 0 || writeSizePixel.height == 0 ||
        writeSizePixel.depthOrArrayLayers == 0) {
        return {};
    }

    auto commandContext = GetScopedPendingCommandContext(QueueBase::SubmitMode::Normal);
    TextureCopy textureCopy;
    textureCopy.texture = destination.texture;
    textureCopy.mipLevel = destination.mipLevel;
    textureCopy.origin = destination.origin;
    textureCopy.aspect = SelectFormatAspects(destination.texture->GetFormat(), destination.aspect);

    SubresourceRange subresources = GetSubresourcesAffectedByCopy(textureCopy, writeSizePixel);

    Texture* texture = ToBackend(destination.texture);
    DAWN_TRY(texture->SynchronizeTextureBeforeUse(&commandContext));
    return texture->Write(&commandContext, subresources, destination.origin, writeSizePixel,
                          static_cast<const uint8_t*>(data) + dataLayout.offset,
                          dataLayout.bytesPerRow, dataLayout.rowsPerImage);
}

bool Queue::HasPendingCommands() const {
    return mPendingCommandsNeedSubmit.load(std::memory_order_acquire);
}

void Queue::ForceEventualFlushOfCommands() {}

MaybeError Queue::WaitForIdleForDestruction() {
    if (!mPendingCommands->IsValid()) {
        return {};
    }

    DAWN_TRY(NextSerial());
    // Wait for all in-flight commands to finish executing
    DAWN_TRY_ASSIGN(std::ignore, WaitForQueueSerial(GetLastSubmittedCommandSerial(),
                                                    std::numeric_limits<Nanoseconds>::max()));

    mBackgroundFlushThread->WaitUntilAllCleaupTasksCompleted();
    return CheckPassedSerials();
}

MaybeError Queue::NextSerial() {
    auto commandContext = GetScopedPendingCommandContext(SubmitMode::Passive);

    DAWN_TRY(commandContext.FlushBuffersForSyncingWithCPU());

    const ExecutionSerial submitSerial = GetPendingCommandSerial();
    if (commandContext->AcquireNeedsFence()) {
        DAWN_ASSERT(mFence);

        TRACE_EVENT1(GetDevice()->GetPlatform(), General, "D3D11Device::SignalFence", "serial",
                     uint64_t(submitSerial));
        DAWN_TRY(CheckHRESULT(commandContext.Signal(mFence.Get(), uint64_t(submitSerial)),
                              "D3D11 command queue signal fence"));
    }

    SystemEventReceiver receiver;
    DAWN_TRY_ASSIGN(receiver, GetSystemEventReceiver());
    mBackgroundFlushThread->EnqueueFlush(receiver.GetPrimitive().Get());
    mPendingEvents->push_back({submitSerial, std::move(receiver)});

    IncrementLastSubmittedCommandSerial();

    return {};
}

ResultOrError<ExecutionSerial> Queue::CheckAndUpdateCompletedSerials() {
    ExecutionSerial completedSerial;
    std::vector<SystemEventReceiver> returnedReceivers;
    DAWN_TRY_ASSIGN(
        completedSerial,
        mPendingEvents.Use([&](auto pendingEvents) -> ResultOrError<ExecutionSerial> {
            if (pendingEvents->empty()) {
                return GetLastSubmittedCommandSerial();
            }

            absl::InlinedVector<HANDLE, 8> handles;
            const size_t numberOfHandles =
                std::min(pendingEvents->size(), static_cast<size_t>(MAXIMUM_WAIT_OBJECTS));
            handles.reserve(numberOfHandles);
            // Gather events in reversed order (from the most recent to the oldest events).
            std::for_each_n(pendingEvents->rbegin(), numberOfHandles, [&handles](const auto& e) {
                handles.push_back(e.receiver.GetPrimitive().Get());
            });
            DWORD result =
                WaitForMultipleObjects(handles.size(), handles.data(), /*bWaitAll=*/false,
                                       /*dwMilliseconds=*/0);
            DAWN_INTERNAL_ERROR_IF(result == WAIT_FAILED, "WaitForMultipleObjects() failed");

            DAWN_INTERNAL_ERROR_IF(
                result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + handles.size(),
                "WaitForMultipleObjects() get abandoned event");

            if (result == WAIT_TIMEOUT) {
                return GetCompletedCommandSerial();
            }

            DAWN_CHECK(result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + pendingEvents->size());
            const size_t completedEventIndex = result - WAIT_OBJECT_0;
            // |WaitForMultipleObjects()| returns the smallest index, if more than one
            // events are signalled. So the number of completed events are
            // |mPendingEvents.size() - index|.
            const size_t completedEvents = pendingEvents->size() - completedEventIndex;
            auto completedSerial = pendingEvents->at(completedEvents - 1).serial;
            returnedReceivers.reserve(completedEvents);
            std::for_each_n(pendingEvents->begin(), completedEvents, [&returnedReceivers](auto& e) {
                returnedReceivers.emplace_back(std::move(e.receiver));
            });
            pendingEvents->erase(pendingEvents->begin(), pendingEvents->begin() + completedEvents);

            return completedSerial;
        }));

    DAWN_TRY(CheckAndMapReadyBuffers(completedSerial));

    if (!returnedReceivers.empty()) {
        DAWN_TRY(ReturnSystemEventReceivers(std::move(returnedReceivers)));
    }

    return completedSerial;
}

ResultOrError<bool> Queue::WaitForQueueSerial(ExecutionSerial serial, Nanoseconds timeout) {
    ExecutionSerial completedSerial = GetCompletedCommandSerial();
    if (serial <= completedSerial) {
        return true;
    }

    if (serial > GetLastSubmittedCommandSerial()) {
        return DAWN_FORMAT_INTERNAL_ERROR(
            "Wait a serial (%llu) which is greater than last submitted command serial (%llu).",
            uint64_t(serial), uint64_t(GetLastSubmittedCommandSerial()));
    }

    bool didComplete = false;
    std::vector<SystemEventReceiver> returnedReceivers;
    DAWN_TRY_ASSIGN(didComplete, mPendingEvents.Use([&](auto pendingEvents) -> ResultOrError<bool> {
        DAWN_ASSERT(!pendingEvents->empty());
        DAWN_ASSERT(serial >= pendingEvents->front().serial);
        DAWN_ASSERT(serial <= pendingEvents->back().serial);
        auto it = std::lower_bound(
            pendingEvents->begin(), pendingEvents->end(), serial,
            [](const SerialEventReceiverPair& a, ExecutionSerial b) { return a.serial < b; });
        DAWN_ASSERT(it != pendingEvents->end());
        DAWN_ASSERT(it->serial == serial);

        // TODO(crbug.com/335553337): call WaitForSingleObject() without holding the mutex.
        DWORD result =
            WaitForSingleObject(it->receiver.GetPrimitive().Get(), ToMilliseconds(timeout));
        DAWN_INTERNAL_ERROR_IF(result == WAIT_FAILED, "WaitForSingleObject() failed");

        if (result != WAIT_OBJECT_0) {
            return false;
        }

        // Events before |it| should be signalled as well.
        const size_t completedEvents = std::distance(pendingEvents->begin(), it) + 1;
        returnedReceivers.reserve(completedEvents);
        std::for_each_n(pendingEvents->begin(), completedEvents, [&returnedReceivers](auto& e) {
            returnedReceivers.emplace_back(std::move(e.receiver));
        });
        pendingEvents->erase(pendingEvents->begin(), pendingEvents->begin() + completedEvents);
        return true;
    }));

    if (!returnedReceivers.empty()) {
        DAWN_TRY(ReturnSystemEventReceivers(std::move(returnedReceivers)));
    }

    return didComplete;
}

void Queue::SetEventOnCompletion(ExecutionSerial serial, HANDLE event) {
    DAWN_UNREACHABLE();
}

void Queue::ScheduleCleanupTask(std::function<void()> task) {
    mBackgroundFlushThread->EnqueueCleanupTask(std::move(task));
}

void Queue::WaitUntilAllCleaupTasksCompleted() {
    mBackgroundFlushThread->WaitUntilAllCleaupTasksCompleted();
}

}  // namespace dawn::native::d3d11
