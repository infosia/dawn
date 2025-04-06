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

#include <processthreadsapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
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

using namespace std::chrono;

namespace dawn::native::d3d11 {

class Queue::PollingThread {
  public:
    PollingThread(Queue* queue, TIMECAPS timecaps)
        : mQueue(queue), mTimeCaps(std::move(timecaps)), mThread([this] { ThreadLoop(); }) {}
    ~PollingThread() {
        mRunning.store(false, std::memory_order_release);
        mEventQueryQueueCv.notify_one();

        mThread.join();
    }

    MaybeError OnSubmit(ExecutionSerial submitSerial,
                        const ScopedCommandRecordingContext* commandContext) {
        {
            std::lock_guard<std::mutex> lg(mEventQueryQueueLock);

            ComPtr<ID3D11Query> query;

            if (!mFreeQueries.empty()) {
                query = std::move(mFreeQueries.back());
                mFreeQueries.pop_back();
            } else {
                D3D11_QUERY_DESC queryDesc = {};
                queryDesc.Query = D3D11_QUERY_EVENT;
                DAWN_TRY(CheckHRESULT(ToBackend(mQueue->GetDevice())
                                          ->GetD3D11Device3()
                                          ->CreateQuery(&queryDesc, &query),
                                      "CreateQuery"));
            }

            commandContext->End(query.Get());

            mPendingQueries.push_back(
                {submitSerial, std::move(query), /*queryTime=*/steady_clock::now()});
        }
        mEventQueryQueueCv.notify_one();

        return {};
    }

    ResultOrError<ExecutionSerial> GetLastCompletedSerialOrError() {
        std::lock_guard<std::mutex> lg(mSerialCheckLock);
        if (DAWN_UNLIKELY(mErrorData)) {
            return std::move(mErrorData);
        }
        return mCompletedSerial;
    }

    ResultOrError<bool> WaitForSerial(ExecutionSerial serial, Nanoseconds timeout) {
        if (timeout == Nanoseconds(0)) {
            std::lock_guard<std::mutex> lg(mSerialCheckLock);
            return CheckPassedSerial(serial);
        }

        TRACE_EVENT1(mQueue->GetDevice()->GetPlatform(), General, "Queue::WaitForSerial", "serial",
                     uint64_t(serial));

        std::unique_lock<std::mutex> lg(mSerialCheckLock);

        mSerialToWait.store(uint64_t(serial), std::memory_order_release);
        mSleepCv.notify_one();

        // wait_for seems to have bug if the timeout is too large. In that case, use infinity wait.
        if (uint64_t(timeout) > 1'000'000'000'000) {
            mSerialCheckCv.wait(
                lg, [this, serial] { return mCompletedSerial >= serial || mErrorData; });
        } else {
            mSerialCheckCv.wait_for(lg, nanoseconds(uint64_t(timeout)), [this, serial] {
                return mCompletedSerial >= serial || mErrorData;
            });
        }

        return CheckPassedSerial(serial);
    }

  private:
    struct EventQueryEntry {
        ExecutionSerial serial;
        ComPtr<ID3D11Query> query;
        steady_clock::time_point queryTime;
    };

    void ThreadLoop() {
        auto device = ToBackend(mQueue->GetDevice());
        if (device->GetFunctions()->setThreadDescription) {
            device->GetFunctions()->setThreadDescription(GetCurrentThread(),
                                                         L"QueueD3D11::PollingThread");
        }

        while (mRunning.load(std::memory_order_acquire)) {
            std::optional<EventQueryEntry> queryEntry;
            {
                std::unique_lock<std::mutex> lg(mEventQueryQueueLock);
                mEventQueryQueueCv.wait(lg, [this] {
                    // Keep waiting until there is a request or running is stopped.
                    return !mPendingQueries.empty() || !mRunning.load(std::memory_order_acquire);
                });
                if (!mPendingQueries.empty()) {
                    queryEntry = mPendingQueries.front();
                    mPendingQueries.pop_front();
                }
            }

            if (queryEntry) {
                WaitUntilQueryDone(*queryEntry);
            }
        }
    }

    void WaitUntilQueryDone(const EventQueryEntry& queryEntry) {
        const uint64_t serialVal = uint64_t(queryEntry.serial);

        TRACE_EVENT1(mQueue->GetDevice()->GetPlatform(), General, "Queue::WaitUntilQueryDone",
                     "serial", serialVal);

        BOOL doneData = FALSE;

        bool flushed = mLastFlushTime > queryEntry.queryTime;

        while (!doneData && mRunning.load(std::memory_order_acquire)) {
            HRESULT hr;
            {
                auto commandContext = mQueue->GetScopedPendingCommandContext(SubmitMode::Passive);
                hr = commandContext.GetData(queryEntry.query.Get(), &doneData, sizeof(doneData),
                                            D3D11_ASYNC_GETDATA_DONOTFLUSH);
            }
            if (DAWN_UNLIKELY(FAILED(hr))) {
                std::lock_guard<std::mutex> lg(mSerialCheckLock);
                mErrorData = DAWN_FORMAT_INTERNAL_ERROR(
                    "ID3D11DeviceContext::GetData() failed, hr=%u", uint32_t(hr));
                mSerialCheckCv.notify_one();
                break;
            } else if (hr == S_OK) {
                DAWN_ASSERT(doneData);

                {
                    std::lock_guard<std::mutex> lg(mEventQueryQueueLock);
                    // Recycle the query
                    mFreeQueries.push_back(std::move(queryEntry.query));
                }
                {
                    std::unique_lock<std::mutex> lg(mSerialCheckLock);
                    DAWN_ASSERT(queryEntry.serial >= mCompletedSerial);
                    // Notify client about the completedSerial
                    mCompletedSerial = queryEntry.serial;
                }
                mSerialCheckCv.notify_one();
            } else {
                // If data is not available, the returned value is S_FALSE which is > 0, so
                // FAILED(hr) will be false.
                DAWN_ASSERT(hr == S_FALSE);

                const bool isUrgent = mSerialToWait.load(std::memory_order_acquire) >= serialVal;

                if (!flushed) {
                    auto now = steady_clock::now();
                    auto elapsedMS = duration_cast<milliseconds>(now - queryEntry.queryTime);

                    if (isUrgent || elapsedMS.count() > 5) {
                        // If time since we start this querying is longer than 5ms, we flush the
                        // command buffer. To avoid infinity wait.
                        auto commandContext =
                            mQueue->GetScopedPendingCommandContext(SubmitMode::Passive);
                        TRACE_EVENT0(mQueue->GetDevice()->GetPlatform(), General,
                                     "D3D11Device::Flush");
                        commandContext.Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

                        mLastFlushTime = steady_clock::now();
                        flushed = true;
                    }
                }

                if (!isUrgent) {
                    // By default, windows only sleep in 15ms period, if we want to sleep in shorter
                    // duration, we need to set the timer's period.
                    auto device = ToBackend(mQueue->GetDevice());
                    device->GetFunctions()->timeBeginPeriod(mTimeCaps.wPeriodMin);

                    std::unique_lock<std::mutex> lg(mSerialCheckLock);
                    mSleepCv.wait_for(lg, std::chrono::milliseconds(1));

                    device->GetFunctions()->timeEndPeriod(mTimeCaps.wPeriodMin);
                } else {
                    std::this_thread::yield();
                }
            }
        }
    }

    ResultOrError<bool> CheckPassedSerial(ExecutionSerial serial) {
        if (DAWN_UNLIKELY(mErrorData)) {
            return std::move(mErrorData);
        }

        return mCompletedSerial >= serial;
    }

    std::vector<ComPtr<ID3D11Query>> mFreeQueries;
    // Events associated with submitted commands. They are in old to recent order.
    std::deque<EventQueryEntry> mPendingQueries;

    raw_ptr<Queue> mQueue;

    std::mutex mEventQueryQueueLock;
    std::condition_variable mEventQueryQueueCv;
    std::condition_variable mSleepCv;
    std::mutex mSerialCheckLock;
    std::condition_variable mSerialCheckCv;

    ExecutionSerial mCompletedSerial = kBeginningOfGPUTime;
    std::atomic<uint64_t> mSerialToWait = uint64_t(kBeginningOfGPUTime);
    std::optional<steady_clock::time_point> mLastFlushTime;

    std::unique_ptr<ErrorData> mErrorData;

    const TIMECAPS mTimeCaps;

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

    // We need time caps in order to sleep in the polling thread with higher resolution.
    TIMECAPS timecaps;
    if (ToBackend(GetDevice())->GetFunctions()->timeGetDevCaps(&timecaps, sizeof(timecaps)) !=
        MMSYSERR_NOERROR) {
        timecaps.wPeriodMin = 1;
    }

    mPollingThread = std::make_unique<PollingThread>(this, std::move(timecaps));

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

    mPollingThread = nullptr;

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
    return CheckPassedSerials();
}

MaybeError Queue::NextSerial() {
    auto commandContext = GetScopedPendingCommandContext(SubmitMode::Passive);

    DAWN_TRY(commandContext.FlushBuffersForSyncingWithCPU());

    const ExecutionSerial submitSerial = GetPendingCommandSerial();

    DAWN_TRY(mPollingThread->OnSubmit(submitSerial, &commandContext));

    if (commandContext->AcquireNeedsFence()) {
        DAWN_ASSERT(mFence);

        TRACE_EVENT1(GetDevice()->GetPlatform(), General, "D3D11Device::SignalFence", "serial",
                     uint64_t(submitSerial));
        DAWN_TRY(CheckHRESULT(commandContext.Signal(mFence.Get(), uint64_t(submitSerial)),
                              "D3D11 command queue signal fence"));
    }

    IncrementLastSubmittedCommandSerial();

    return {};
}

ResultOrError<ExecutionSerial> Queue::CheckAndUpdateCompletedSerials() {
    ExecutionSerial completedSerial;
    DAWN_TRY_ASSIGN(completedSerial, mPollingThread->GetLastCompletedSerialOrError());

    DAWN_TRY(CheckAndMapReadyBuffers(completedSerial));

    return completedSerial;
}

ResultOrError<bool> Queue::WaitForQueueSerial(ExecutionSerial serial, Nanoseconds timeout) {
    return WaitForQueueSerialImpl(serial, timeout);
}

ResultOrError<bool> Queue::WaitForQueueSerialImpl(ExecutionSerial serial, Nanoseconds timeout) {
    ExecutionSerial completedSerial = GetCompletedCommandSerial();
    if (serial <= completedSerial) {
        return true;
    }

    if (serial > GetLastSubmittedCommandSerial()) {
        return DAWN_FORMAT_INTERNAL_ERROR(
            "Wait a serial (%llu) which is greater than last submitted command serial (%llu).",
            uint64_t(serial), uint64_t(GetLastSubmittedCommandSerial()));
    }

    return mPollingThread->WaitForSerial(serial, timeout);
}

void Queue::SetEventOnCompletion(ExecutionSerial serial, HANDLE event) {
    DAWN_UNREACHABLE();
}

}  // namespace dawn::native::d3d11
