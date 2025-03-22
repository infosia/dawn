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
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "dawn/common/Log.h"
#include "dawn/native/AsyncTask.h"
#include "dawn/native/WaitAnySystemEvent.h"
#include "dawn/native/d3d/D3DError.h"
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
    FlushThread(Queue* queue) : mQueue(queue), mThread([this] { ThreadLoop(); }) {}
    ~FlushThread() {
        mRunning.store(false, std::memory_order_release);
        mCv.notify_one();

        mThread.join();
    }

    MaybeError Initialize() {
        auto device = ToBackend(mQueue->GetDevice());
        DAWN_TRY(
            CheckHRESULT(device->GetD3D11Device3()->QueryInterface(IID_PPV_ARGS(&mDXGIDevice2)),
                         "D3D11 querying IDXGIDevice2"));
        return {};
    }

    void EnqueueFlush() {
        std::lock_guard<std::mutex> lg(mLock);
        mPendingFlush = true;
        mCv.notify_one();
    }

  private:
    void ThreadLoop() {
        while (mRunning.load(std::memory_order_acquire)) {
            bool hasRequest = false;

            {
                std::unique_lock<std::mutex> lg(mLock);
                mCv.wait(lg, [this] {
                    // Keep waiting until there is a request or running is stopped.
                    return mPendingFlush || !mRunning.load(std::memory_order_acquire);
                });
                hasRequest = mPendingFlush;
                mPendingFlush = false;
            }

            if (hasRequest) {
                TRACE_EVENT0(mQueue->GetDevice()->GetPlatform(), General, "D3D11Device::Flush");
                auto commandContext = mQueue->GetScopedPendingCommandContext(SubmitMode::Passive);

                commandContext.Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);
#if 0
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                int a = 0;
                a = 1;
                (void)a;
#endif
            }
        }
    }

    raw_ptr<Queue> mQueue;
    ComPtr<IDXGIDevice2> mDXGIDevice2;

    bool mPendingFlush = false;

    std::mutex mLock;
    std::condition_variable mCv;

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
    DAWN_TRY(mBackgroundFlushThread->Initialize());

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

    mPendingCommands.Use([&](auto pendingCommands) {
        pendingCommands->Destroy();
        mPendingCommandsNeedSubmit.store(false, std::memory_order_release);
    });

    mBackgroundFlushThread = nullptr;
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

std::optional<ScopedCommandRecordingContext> Queue::TryGetScopedPendingCommandContext(
    SubmitMode submitMode) {
    std::optional<ScopedCommandRecordingContext> pendingCommandsOrNull = std::nullopt;
    mPendingCommands.TryUse([&](auto commands) {
        if (submitMode == SubmitMode::Normal) {
            mPendingCommandsNeedSubmit.store(true, std::memory_order_release);
        }
        pendingCommandsOrNull.emplace(std::move(commands));
    });

    return pendingCommandsOrNull;
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
    auto deviceLock(GetDevice()->GetScopedLock());
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
    auto commandContext = TryGetScopedPendingCommandContext(QueueBase::SubmitMode::Passive);
    if (!commandContext) {
        // The FlushThread might be busy using the Command Context. Return early in this case.
        // We can always check the pending mapped buffers at later points.
        return {};
    }
    for (const auto& bufferEntry : mPendingMapBuffers.IterateUpTo(completedSerial)) {
        DAWN_TRY(bufferEntry.buffer->FinalizeMap(&commandContext.value(), completedSerial,
                                                 bufferEntry.mode));
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
    DAWN_TRY(NextSerial());
    // Wait for all in-flight commands to finish executing
    DAWN_TRY_ASSIGN(std::ignore, WaitForQueueSerial(GetLastSubmittedCommandSerial(),
                                                    std::numeric_limits<Nanoseconds>::max()));
    return CheckPassedSerials();
}

MaybeError Queue::NextSerial() {
    DAWN_TRY(mQueryState.Use([&](auto state) -> MaybeError {
        auto commandContext = GetScopedPendingCommandContext(SubmitMode::Passive);

        DAWN_TRY(commandContext.FlushBuffersForSyncingWithCPU());

        IncrementLastSubmittedCommandSerial();
        ExecutionSerial lastSubmittedSerial = GetLastSubmittedCommandSerial();

        ComPtr<ID3D11Query> query;

        if (!state->freeQueries.empty()) {
            query = std::move(state->freeQueries.back());
            state->freeQueries.pop_back();
        } else {
            D3D11_QUERY_DESC queryDesc = {};
            queryDesc.Query = D3D11_QUERY_EVENT;
            DAWN_TRY(CheckHRESULT(
                ToBackend(GetDevice())->GetD3D11Device5()->CreateQuery(&queryDesc, &query),
                "CreateQuery"));
        }

        commandContext.End(query.Get());

        if (commandContext->AcquireNeedsFence()) {
            DAWN_ASSERT(mFence);

            TRACE_EVENT1(GetDevice()->GetPlatform(), General, "D3D11Device::SignalFence", "serial",
                         uint64_t(lastSubmittedSerial));
            DAWN_TRY(
                CheckHRESULT(commandContext.Signal(mFence.Get(), uint64_t(lastSubmittedSerial)),
                             "D3D11 command queue signal fence"));
        }

        state->pendingQueries.push_back({lastSubmittedSerial, std::move(query)});

        return {};
    }));

    mBackgroundFlushThread->EnqueueFlush();

    return {};
}

ResultOrError<ExecutionSerial> Queue::CheckAndUpdateCompletedSerials() {
    ExecutionSerial completedSerial;
    DAWN_TRY_ASSIGN(
        completedSerial, mQueryState.Use([&](auto state) -> ResultOrError<ExecutionSerial> {
            if (state->pendingQueries.empty()) {
                return GetLastSubmittedCommandSerial();
            }

            auto commandContext = TryGetScopedPendingCommandContext(SubmitMode::Passive);
            if (!commandContext) {
                // FlushThread is busy using the command context, skip checking for queries. We can
                // check again at later points.
                return GetCompletedCommandSerial();
            }

            size_t unfinishedIdx = 0;
            for (; unfinishedIdx < state->pendingQueries.size(); ++unfinishedIdx) {
                BOOL done;
                auto& query = state->pendingQueries.at(unfinishedIdx);
                if (FAILED(commandContext->GetData(query.query.Get(), &done, sizeof(done),
                                                   D3D11_ASYNC_GETDATA_DONOTFLUSH))) {
                    break;
                }

                DAWN_CHECK(done);
                state->freeQueries.push_back(std::move(query.query));
            }

            if (unfinishedIdx == 0) {
                return GetCompletedCommandSerial();
            }

            auto completedSerial = state->pendingQueries.at(unfinishedIdx - 1).serial;
            state->pendingQueries.erase(state->pendingQueries.begin(),
                                        state->pendingQueries.begin() + unfinishedIdx);

            return completedSerial;
        }));

    DAWN_TRY(CheckAndMapReadyBuffers(completedSerial));

    return completedSerial;
}

ResultOrError<bool> Queue::WaitForQueueSerial(ExecutionSerial serial, Nanoseconds timeout) {
    if (timeout == Nanoseconds(0)) {
        return WaitForQueueSerialImpl(serial, timeout);
    }
    TRACE_EVENT1(GetDevice()->GetPlatform(), General, "Queue::WaitForQueueSerial", "serial",
                 uint64_t(serial));
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

    bool done;
    DAWN_TRY_ASSIGN(done, mQueryState.Use([&](auto state) -> ResultOrError<bool> {
        DAWN_ASSERT(!state->pendingQueries.empty());
        DAWN_ASSERT(serial >= state->pendingQueries.front().serial);
        DAWN_ASSERT(serial <= state->pendingQueries.back().serial);
        auto it = std::lower_bound(
            state->pendingQueries.begin(), state->pendingQueries.end(), serial,
            [](const SerialEventQueryPair& a, ExecutionSerial b) { return a.serial < b; });
        DAWN_ASSERT(it != state->pendingQueries.end());
        DAWN_ASSERT(it->serial == serial);

        bool waitComplete = false;
        if (timeout == Nanoseconds(0)) {
            auto commandContext = TryGetScopedPendingCommandContext(SubmitMode::Passive);
            if (!commandContext) {
                // FlushThread is busy using the command context, skip checking for queries. We can
                // check again at later points.
                return false;
            }
            waitComplete = WaitForQueryData(*commandContext, it->query.Get(), timeout);
        } else {
            auto commandContext = GetScopedPendingCommandContext(SubmitMode::Passive);
            waitComplete = WaitForQueryData(commandContext, it->query.Get(), timeout);
        }

        if (!waitComplete) {
            return false;
        }

        auto numQueriesToClear = std::distance(state->pendingQueries.begin(), it) + 1;

        std::for_each_n(state->pendingQueries.begin(), numQueriesToClear,
                        [&freeQueries = state->freeQueries](auto& e) {
                            freeQueries.emplace_back(std::move(e.query));
                        });
        state->pendingQueries.erase(state->pendingQueries.begin(),
                                    state->pendingQueries.begin() + numQueriesToClear);

        return true;
    }));

    return done;
}

bool Queue::WaitForQueryData(const ScopedCommandRecordingContext& commandContext,
                             ID3D11Query* query,
                             Nanoseconds timeout) {
    using namespace std::chrono;
    auto startTime = high_resolution_clock::now();
    BOOL doneData;
    while (FAILED(commandContext.GetData(query, &doneData, sizeof(doneData),
                                         D3D11_ASYNC_GETDATA_DONOTFLUSH))) {
        auto endTime = high_resolution_clock::now();

        auto elapsed = duration_cast<nanoseconds>(endTime - startTime);
        if (static_cast<uint64_t>(elapsed.count()) >= static_cast<uint64_t>(timeout)) {
            return false;
        }

        std::this_thread::sleep_for(microseconds(1));
    }

    DAWN_CHECK(doneData);
    return true;
}

void Queue::SetEventOnCompletion(ExecutionSerial serial, HANDLE event) {
    DAWN_UNREACHABLE();
}

}  // namespace dawn::native::d3d11
