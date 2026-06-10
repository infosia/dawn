// Copyright 2018 The Dawn & Tint Authors
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

#include "src/dawn/native/vulkan/QueueVk.h"

#include <array>
#include <limits>
#include <optional>
#include <utility>

#include "absl/cleanup/cleanup.h"
#include "dawn/platform/DawnPlatform.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "src/dawn/common/MatchVariant.h"
#include "src/dawn/common/Math.h"
#include "src/dawn/native/Buffer.h"
#include "src/dawn/native/CommandValidation.h"
#include "src/dawn/native/Commands.h"
#include "src/dawn/native/DynamicUploader.h"
#include "src/dawn/native/WaitAnySystemEvent.h"
#include "src/dawn/native/vulkan/CommandBufferVk.h"
#include "src/dawn/native/vulkan/CommandRecordingContextVk.h"
#include "src/dawn/native/vulkan/DeviceVk.h"
#include "src/dawn/native/vulkan/FencedDeleter.h"
#include "src/dawn/native/vulkan/TextureVk.h"
#include "src/dawn/native/vulkan/UniqueVkHandle.h"
#include "src/dawn/native/vulkan/UtilsVulkan.h"
#include "src/dawn/native/vulkan/VulkanError.h"
#include "src/dawn/platform/metrics/HistogramMacros.h"
#include "src/dawn/platform/tracing/TraceEvent.h"
#include "src/dawn/utils/SystemHandle.h"
#include "src/utils/compiler.h"

namespace dawn::native::vulkan {

namespace {

// Destroys command pool/buffer.
// TODO(dawn:1601) Revisit this and potentially bake into pool/buffer objects instead.
void DestroyCommandPoolAndBuffer(const VulkanFunctions& fn,
                                 VkDevice device,
                                 const CommandPoolAndBuffer& commands) {
    // The VkCommandBuffer memory should be wholly owned by the pool and freed when it is
    // destroyed, but that's not the case in some drivers and they leak memory. So we call
    // FreeCommandBuffers before DestroyCommandPool to be safe.
    // TODO(enga): Only do this on a known list of bad drivers.
    if (commands.pool != VK_NULL_HANDLE) {
        if (commands.commandBuffer != VK_NULL_HANDLE) {
            fn.FreeCommandBuffers(device, commands.pool, 1, &commands.commandBuffer);
        }
        fn.DestroyCommandPool(device, commands.pool, nullptr);
    }
}

}  // anonymous namespace

// static
ResultOrError<Ref<Queue>> Queue::Create(Device* device,
                                        const QueueDescriptor* descriptor,
                                        uint32_t family) {
    Ref<Queue> queue = AcquireRef(new Queue(device, descriptor, family));
    DAWN_TRY(queue->Initialize());
    return queue;
}

Queue::Queue(Device* device, const QueueDescriptor* descriptor, uint32_t family)
    : QueueBase(device, descriptor), mQueueFamily(family) {}

Queue::~Queue() {}

MaybeError Queue::Initialize() {
    Device* device = ToBackend(GetDevice());
    device->fn.GetDeviceQueue(device->GetVkDevice(), mQueueFamily, 0, &mQueue);

    DAWN_TRY(PrepareRecordingContext());

    SetLabelImpl();
    return {};
}

MaybeError Queue::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
    TRACE_EVENT_BEGIN0(GetDevice()->GetPlatform(), Recording, "CommandBufferVk::RecordCommands");
    CommandRecordingContext* recordingContext = GetPendingRecordingContext();
    for (uint32_t i = 0; i < commandCount; ++i) {
        DAWN_UNSAFE_TODO(DAWN_TRY(ToBackend(commands[i])->RecordCommands(recordingContext)));
    }
    TRACE_EVENT_END0(GetDevice()->GetPlatform(), Recording, "CommandBufferVk::RecordCommands");

    DAWN_TRY(SubmitPendingCommandsImpl());

    return {};
}

void Queue::SetLabelImpl() {
    Device* device = ToBackend(GetDevice());
    // TODO(crbug.com/dawn/1344): When we start using multiple queues this needs to be adjusted
    // so it doesn't always change the default queue's label.
    SetDebugName(device, VK_OBJECT_TYPE_QUEUE, mQueue, "Dawn_Queue", GetLabel());
}

bool Queue::HasPendingCommands() const {
    return mRecordingContext.needsSubmit;
}

VkQueue Queue::GetVkQueue() const {
    return mQueue;
}

ResultOrError<ExecutionSerial> Queue::CheckAndUpdateCompletedSerials() {
    // TODO(crbug.com/40643114): Revisit whether this lock is needed for this backend.
    auto deviceGuard = GetDevice()->GetGuard();

    Device* device = ToBackend(GetDevice());
    return mFencesInFlight.Use([&](auto fencesInFlight) -> ResultOrError<ExecutionSerial> {
        ExecutionSerial fenceSerial(0u);
        while (!fencesInFlight->empty()) {
            if (fencesInFlight->front().waiterCount > 0) {
                // If there is currently another waiter, that waiter will also call this function
                // eventually, so we defer it to the last waiter to handle recycling the fence.
                return fenceSerial;
            }

            const auto& fenceOrHandle = fencesInFlight->front().fenceOrHandle;
            ExecutionSerial tentativeSerial = fencesInFlight->front().serial;

            VkResult result = MatchVariant(
                fenceOrHandle,
                [&](const VkFence& fence) {
                    return VkResult::WrapUnsafe(
                        INJECT_ERROR_OR_RUN(device->fn.GetFenceStatus(device->GetVkDevice(), fence),
                                            VK_ERROR_DEVICE_LOST));
                },
                [&](const utils::SystemHandle& exportedHandle) {
                    return WaitOneSystemHandle(exportedHandle.Duplicate(), Nanoseconds(0u))
                               ? VkResult::WrapUnsafe(VK_SUCCESS)
                               : VkResult::WrapUnsafe(VK_NOT_READY);
                });

            // Fence are added in order, so we can stop searching as soon
            // as we see one that's not ready.
            if (result == VK_NOT_READY) {
                return fenceSerial;
            } else {
                DAWN_TRY(CheckVkSuccess(::VkResult(result), "GetFenceStatus"));
            }

            // Update fenceSerial since fence is ready.
            fenceSerial = tentativeSerial;

            MatchVariant(
                fenceOrHandle,  // Comment to force formatting.
                [&](const VkFence& fence) { mUnusedFences->push_back(fence); },
                [](const utils::SystemHandle&) {
                    // Exported handles are closed/destroyed when popped.
                });
            fencesInFlight->pop_front();
        }
        return fenceSerial;
    });
}

void Queue::ForceEventualFlushOfCommands() {
    mRecordingContext.needsSubmit |= mRecordingContext.used;
}

MaybeError Queue::WaitForIdleForDestructionImpl() {
    // Immediately tag the recording context as unused so we don't try to submit it in Tick.
    // Move the mRecordingContext.used to mUnusedCommands so it can be cleaned up in
    // ShutDownImpl
    if (mRecordingContext.used) {
        CommandPoolAndBuffer commands = {mRecordingContext.commandPool,
                                         mRecordingContext.commandBuffer};
        mUnusedCommands->push_back(commands);
        mRecordingContext = CommandRecordingContext();
    }

    Device* device = ToBackend(GetDevice());

    // Ignore the result of QueueWaitIdle: it can return OOM which we can't really do anything
    // about, Device lost, which means workloads running on the GPU are no longer accessible
    // (so they are as good as waited on) or success.
    [[maybe_unused]] VkResult waitIdleResult =
        VkResult::WrapUnsafe(device->fn.QueueWaitIdle(mQueue));

    DAWN_TRY(WaitForQueueSerial(GetLastSubmittedCommandSerial(),
                                std::numeric_limits<Nanoseconds>::max()));
    return {};
}

CommandRecordingContext* Queue::GetPendingRecordingContext(SubmitMode submitMode) {
    DAWN_ASSERT(mRecordingContext.commandBuffer != VK_NULL_HANDLE);
    mRecordingContext.needsSubmit |= (submitMode == SubmitMode::Normal);
    mRecordingContext.used = true;
    return &mRecordingContext;
}

MaybeError Queue::PrepareRecordingContext() {
    DAWN_ASSERT(!mRecordingContext.needsSubmit);
    DAWN_ASSERT(mRecordingContext.commandBuffer == VK_NULL_HANDLE);
    DAWN_ASSERT(mRecordingContext.commandPool == VK_NULL_HANDLE);

    CommandPoolAndBuffer commands;
    DAWN_TRY_ASSIGN(commands, BeginVkCommandBuffer());

    mRecordingContext.commandBuffer = commands.commandBuffer;
    mRecordingContext.commandPool = commands.pool;
    mRecordingContext.commandBufferList.push_back(commands.commandBuffer);
    mRecordingContext.commandPoolList.push_back(commands.pool);

    return {};
}

// Splits the recording context, ending the current command buffer and beginning a new one.
// This should not be necessary in most cases, and is provided only to work around driver issues
// on some hardware.
MaybeError Queue::SplitRecordingContext(CommandRecordingContext* recordingContext) {
    DAWN_ASSERT(recordingContext->used);
    Device* device = ToBackend(GetDevice());

    DAWN_TRY(CheckVkSuccess(device->fn.EndCommandBuffer(recordingContext->commandBuffer),
                            "vkEndCommandBuffer"));

    CommandPoolAndBuffer commands;
    DAWN_TRY_ASSIGN(commands, BeginVkCommandBuffer());

    recordingContext->commandBuffer = commands.commandBuffer;
    recordingContext->commandPool = commands.pool;
    recordingContext->commandBufferList.push_back(commands.commandBuffer);
    recordingContext->commandPoolList.push_back(commands.pool);
    recordingContext->hasRecordedRenderPass = false;

    return {};
}

ResultOrError<CommandPoolAndBuffer> Queue::BeginVkCommandBuffer() {
    Device* device = ToBackend(GetDevice());
    VkDevice vkDevice = device->GetVkDevice();

    // First try to recycle unused command pools.
    auto result =
        mUnusedCommands.Use([&](auto unusedCommands) -> std::optional<CommandPoolAndBuffer> {
            if (!unusedCommands->empty()) {
                CommandPoolAndBuffer recycledCommands = unusedCommands->back();
                unusedCommands->pop_back();
                return recycledCommands;
            }
            return std::nullopt;
        });

    CommandPoolAndBuffer commands;
    if (result) {
        commands = *result;
    } else {
        // Create a new command pool for our commands and allocate the command buffer.
        VkCommandPoolCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        createInfo.queueFamilyIndex = mQueueFamily;

        DAWN_TRY(CheckVkSuccess(
            device->fn.CreateCommandPool(vkDevice, &createInfo, nullptr, &*commands.pool),
            "vkCreateCommandPool"));

        VkCommandBufferAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.commandPool = commands.pool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        DAWN_TRY_WITH_CLEANUP(CheckVkSuccess(device->fn.AllocateCommandBuffers(
                                                 vkDevice, &allocateInfo, &commands.commandBuffer),
                                             "vkAllocateCommandBuffers"),
                              { DestroyCommandPoolAndBuffer(device->fn, vkDevice, commands); });
    }

    // Start the recording of commands in the command buffer.
    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    DAWN_TRY_WITH_CLEANUP(
        CheckVkSuccess(device->fn.BeginCommandBuffer(commands.commandBuffer, &beginInfo),
                       "vkBeginCommandBuffer"),
        { DestroyCommandPoolAndBuffer(device->fn, vkDevice, commands); });

    return commands;
}

MaybeError Queue::SubmitPendingCommandsImpl() {
    if (!mRecordingContext.needsSubmit) {
        return {};
    }

    Device* device = ToBackend(GetDevice());

    // Ensure that after calling this method we have a fresh recording context even if one of the
    // DAWN_TRY calls below fail.
    absl::Cleanup recycleContext = [&]() {
        [[maybe_unused]] bool hadError = GetDevice()->ConsumedError(
            RecycleRecordingContext(), "Recycling recording context after submit failed for %s",
            this);
    };

    if (!mRecordingContext.mappableBuffersForEagerTransition.empty()) {
        // Transition mappable buffers back to map usages with the submit.
        Buffer::TransitionMappableBuffersEagerly(
            device, &mRecordingContext, mRecordingContext.mappableBuffersForEagerTransition);
    }

    for (auto texture : mRecordingContext.specialSyncTextures) {
        DAWN_TRY(texture->OnBeforeSubmit(&mRecordingContext));
    }

    DAWN_TRY(CheckVkSuccess(device->fn.EndCommandBuffer(mRecordingContext.commandBuffer),
                            "vkEndCommandBuffer"));

    std::vector<VkPipelineStageFlags> dstStageMasks(mRecordingContext.waitSemaphores.size(),
                                                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(mRecordingContext.waitSemaphores.size());
    submitInfo.pWaitSemaphores = AsVkArray(mRecordingContext.waitSemaphores.data());
    submitInfo.pWaitDstStageMask = dstStageMasks.data();
    submitInfo.commandBufferCount = mRecordingContext.commandBufferList.size();
    submitInfo.pCommandBuffers = mRecordingContext.commandBufferList.data();
    submitInfo.signalSemaphoreCount = mRecordingContext.signalSemaphores.size();
    submitInfo.pSignalSemaphores = AsVkArray(mRecordingContext.signalSemaphores.data());

    VkFence fence = VK_NULL_HANDLE;
    DAWN_TRY_ASSIGN(fence, GetUnusedFence());

    platform::metrics::DawnHistogramTimer timer(device->GetPlatform());
    TRACE_EVENT_BEGIN0(device->GetPlatform(), Recording, "vkQueueSubmit");
    DAWN_TRY_WITH_CLEANUP(
        CheckVkSuccess(device->fn.QueueSubmit(mQueue, 1, &submitInfo, fence), "vkQueueSubmit"), {
            // If submitting to the queue fails, move the fence back into the unused fence
            // list, as if it were never acquired. Not doing so would leak the fence since
            // it would be neither in the unused list nor in the in-flight list.
            mUnusedFences->push_back(fence);
        });
    TRACE_EVENT_END0(device->GetPlatform(), Recording, "vkQueueSubmit");
    timer.RecordMicroseconds("Vulkan.VkQueueSubmitUS");

    // Enqueue the semaphores before incrementing the serial, so that they can be deleted as
    // soon as the current submission is finished.
    for (VkSemaphore semaphore : mRecordingContext.waitSemaphores) {
        device->GetFencedDeleter()->DeleteWhenUnused(semaphore);
    }
    IncrementLastSubmittedCommandSerial();
    mFencesInFlight->push_back(FenceInFlight{fence, GetLastSubmittedCommandSerial(), 0});

    for (auto texture : mRecordingContext.specialSyncTextures) {
        DAWN_TRY(texture->OnAfterSubmit());
    }

    return {};
}

MaybeError Queue::RecycleRecordingContext() {
    ExecutionSerial lastSubmittedSerial = GetLastSubmittedCommandSerial();

    for (size_t i = 0; i < mRecordingContext.commandBufferList.size(); ++i) {
        CommandPoolAndBuffer commands = {mRecordingContext.commandPoolList[i],
                                         mRecordingContext.commandBufferList[i]};
        TrackSerialTask(QueuePriority::UserVisible, lastSubmittedSerial, [commands, this]() {
            Device* device = ToBackend(GetDevice());
            VkDevice vkDevice = device->GetVkDevice();

            MaybeError result = CheckVkSuccess(
                device->fn.ResetCommandPool(vkDevice, commands.pool, 0), "vkResetCommandPool");
            if (result.IsError()) {
                result.AcquireError();
                DestroyCommandPoolAndBuffer(device->fn, vkDevice, commands);
                return;
            }

            mUnusedCommands->push_back(commands);
        });
    }

    mRecordingContext = CommandRecordingContext();
    DAWN_TRY(PrepareRecordingContext());

    return {};
}

ResultOrError<VkFence> Queue::GetUnusedFence() {
    Device* device = ToBackend(GetDevice());
    VkDevice vkDevice = device->GetVkDevice();

    // Returns VK_NULL_HANDLE if there are no unused fences available, otherwise resets an unused
    // fence and tries to return it.
    auto TryPopUnusedFence = [&]() -> ResultOrError<VkFence> {
        return mUnusedFences.Use([&](auto unusedFences) -> ResultOrError<VkFence> {
            VkFence fence = VK_NULL_HANDLE;
            if (!unusedFences->empty()) {
                fence = unusedFences->back();
                DAWN_ASSERT(fence != VK_NULL_HANDLE);
                DAWN_TRY(
                    CheckVkSuccess(device->fn.ResetFences(vkDevice, 1, &*fence), "vkResetFences"));
                unusedFences->pop_back();
            }
            return fence;
        });
    };

    auto CreateFence = [&]() -> ResultOrError<VkFence> {
        VkFenceCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;

        VkExportFenceCreateInfo exportFenceCreateInfo;
        exportFenceCreateInfo.sType = VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO;
        exportFenceCreateInfo.pNext = nullptr;
#if DAWN_PLATFORM_IS(POSIX)
        exportFenceCreateInfo.handleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
#elif DAWN_PLATFORM_IS(WINDOWS)
        exportFenceCreateInfo.handleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#endif
        if (GetDevice()->IsToggleEnabled(Toggle::ExportQueueEventsToSystemEvents)) {
            createInfo.pNext = &exportFenceCreateInfo;
        }

        VkFence newFence = VK_NULL_HANDLE;
        DAWN_TRY(CheckVkSuccess(device->fn.CreateFence(vkDevice, &createInfo, nullptr, &*newFence),
                                "vkCreateFence"));
        return newFence;
    };

    VkFence fence = VK_NULL_HANDLE;
    DAWN_TRY_ASSIGN(fence, TryPopUnusedFence());
    while (fence == VK_NULL_HANDLE) {
        ExecutionSerial lowestSerial = kBeginningOfGPUTime;
        mFencesInFlight.Use([&](auto fencesInFlight) {
            if (fencesInFlight->size() >= kMaxFencesInFlight) {
                lowestSerial = fencesInFlight->front().serial;
            }
        });

        if (lowestSerial == kBeginningOfGPUTime) {
            // We haven't reached the maximum capacity of fences, so create a new fence.
            DAWN_TRY_ASSIGN(fence, CreateFence());
            break;
        }

        DAWN_TRY(WaitForQueueSerial(lowestSerial, Nanoseconds(UINT64_MAX)));
        DAWN_TRY_ASSIGN(fence, TryPopUnusedFence());
    }
    DAWN_CHECK(fence != VK_NULL_HANDLE);
    return fence;
}

void Queue::DestroyImpl(DestroyReason reason) {
    Device* device = ToBackend(GetDevice());
    VkDevice vkDevice = device->GetVkDevice();

    // Immediately tag the recording context as unused so we don't try to submit it in Tick.
    mRecordingContext.needsSubmit = false;
    if (mRecordingContext.commandPool != VK_NULL_HANDLE) {
        DestroyCommandPoolAndBuffer(
            device->fn, vkDevice, {mRecordingContext.commandPool, mRecordingContext.commandBuffer});
    }

    for (VkSemaphore semaphore : mRecordingContext.waitSemaphores) {
        device->fn.DestroySemaphore(vkDevice, semaphore, nullptr);
    }
    mRecordingContext.waitSemaphores.clear();
    mRecordingContext.signalSemaphores.clear();

    // Free any recycled command pools.
    mUnusedCommands.Use([&](auto unusedCommands) {
        for (const CommandPoolAndBuffer& commands : *unusedCommands) {
            DestroyCommandPoolAndBuffer(device->fn, vkDevice, commands);
        }
        unusedCommands->clear();
    });

    // Some fences might still be marked as in-flight if we shut down because of a device loss.
    // Delete them since at this point all commands are complete.
    mFencesInFlight.Use([&](auto fencesInFlight) {
        while (!fencesInFlight->empty()) {
            MatchVariant(
                fencesInFlight->front().fenceOrHandle,
                [&](const VkFence& fence) { device->fn.DestroyFence(vkDevice, fence, nullptr); },
                [](const utils::SystemHandle&) {
                    // Exported handles are closed/destroyed when popped or when the deque is
                    // cleared.
                });
            fencesInFlight->pop_front();
        }
    });

    mUnusedFences.Use([&](auto unusedFences) {
        for (VkFence fence : *unusedFences) {
            device->fn.DestroyFence(vkDevice, fence, nullptr);
        }
        unusedFences->clear();
    });

    QueueBase::DestroyImpl(reason);
}

ResultOrError<ExecutionSerial> Queue::WaitForQueueSerialImpl(ExecutionSerial waitSerial,
                                                             Nanoseconds timeout) {
    Device* device = ToBackend(GetDevice());
    VkDevice vkDevice = device->GetVkDevice();
    // If the client has passed a finite timeout, the function will eventually return due to
    // either (1) the fences being signaled, (2) the timeout being reached, or (3) the device
    // being lost. If the client has passed an infinite timeout, this function might hang forever
    // if the fences are never signaled (which matches the semantics that the client has
    // specified).
    // TODO(crbug.com/344798087): Handle the issue of timeouts in a more general way further up the
    // stack.
    std::variant<VkFence, utils::SystemHandle> fenceOrHandle;
    ExecutionSerial completedSerial = kWaitSerialTimeout;

    mFencesInFlight.Use([&](auto fencesInFlight) {
        for (auto it = fencesInFlight->begin(); it != fencesInFlight->end(); ++it) {
            if (it->serial >= waitSerial) {
                completedSerial = it->serial;
                it->waiterCount++;

                MatchVariant(
                    it->fenceOrHandle,  // Comment to force formatting.
                    [&](const VkFence& fence) { fenceOrHandle = fence; },
                    [&](const utils::SystemHandle& exportedHandle) {
                        fenceOrHandle = exportedHandle.Duplicate();
                    });
                break;
            }
        }
    });

    // If we did not find a serial to wait for, then that means the serial must already be
    // completed.
    if (completedSerial == kWaitSerialTimeout) {
        return waitSerial;
    }

    // Cleanup routine to ensure that we unset the waiterCount for the fence or handle.
    absl::Cleanup decrementWaiterCount = [&] {
        mFencesInFlight.Use([&](auto fencesInFlight) {
            for (auto it = fencesInFlight->begin(); it != fencesInFlight->end(); ++it) {
                if (it->serial == completedSerial) {
                    DAWN_CHECK(it->waiterCount > 0);
                    it->waiterCount--;
                    break;
                }
            }
        });
    };

    return MatchVariant(
        fenceOrHandle,
        [&](VkFence& fence) -> ResultOrError<ExecutionSerial> {
            if (fence == VK_NULL_HANDLE) {
                // If the fence is VK_NULL_HANDLE, that means that it was exported but was
                // already complete by the time it was finished exporting.
                return waitSerial;
            }

            VkResult waitResult = VkResult::WrapUnsafe(VK_TIMEOUT);
            do {
                // There is evidence that `VK_TIMEOUT` can get returned even when the client
                // has specified an infinite timeout (e.g., due to signals). Retry waiting on
                // the fence in this case in order to satisfy the semantics that the function
                // should return only when either (a) the fences are signaled or (b) the
                // passed-in timeout is reached. Note that this can result in this function
                // busy-looping forever in this case, but the client has explicitly requested
                // this behavior by passing in an infinite timeout.
                // TODO(crbug.com/344798087): Handle the issue of timeouts in a more general way
                // further up the stack.
                waitResult = VkResult::WrapUnsafe(
                    INJECT_ERROR_OR_RUN(device->fn.WaitForFences(vkDevice, 1, &*fence, true,
                                                                 static_cast<uint64_t>(timeout)),
                                        VK_ERROR_DEVICE_LOST));
            } while (waitResult == VK_TIMEOUT &&
                     static_cast<uint64_t>(timeout) == std::numeric_limits<uint64_t>::max());

            if (waitResult != VK_TIMEOUT) {
                DAWN_TRY(CheckVkSuccess(::VkResult(waitResult), "vkWaitForFences"));
                return completedSerial;
            } else {
                return kWaitSerialTimeout;
            }
        },
        [&](utils::SystemHandle& waitHandle) -> ResultOrError<ExecutionSerial> {
            if (WaitOneSystemHandle(std::move(waitHandle), timeout)) {
                return completedSerial;
            } else {
                return kWaitSerialTimeout;
            }
        });
}

ResultOrError<SystemEventReceiver> Queue::GetSystemEventReceiverForSerial(ExecutionSerial serial) {
    Device* device = ToBackend(GetDevice());
    DAWN_CHECK(device->IsToggleEnabled(Toggle::ExportQueueEventsToSystemEvents));

    return mFencesInFlight.Use([&](auto fencesInFlight) -> ResultOrError<SystemEventReceiver> {
        for (auto it = fencesInFlight->begin(); it != fencesInFlight->end(); ++it) {
            // Search for the first fence/handle >= serial.
            if (it->serial < serial) {
                continue;
            }

            return MatchVariant(
                it->fenceOrHandle,
                [&](VkFence& fence) -> ResultOrError<SystemEventReceiver> {
                    if (fence == VK_NULL_HANDLE) {
                        // The fence is only ever null if we've exported it, and by the time the
                        // export completed, the fence already completed.
                        return SystemEventReceiver::CreateAlreadySignaled();
                    }

                    utils::SystemHandle::Handle handle = utils::SystemHandle::kInvalidHandle;
#if DAWN_PLATFORM_IS(POSIX)
                    VkFenceGetFdInfoKHR info;
                    info.sType = VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
                    info.pNext = nullptr;
                    info.fence = fence;
                    info.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
                    DAWN_TRY(CheckVkSuccess(
                        device->fn.GetFenceFdKHR(device->GetVkDevice(), &info, &handle),
                        "vkGetFenceFdKHR"));

                    // Once we export the fence to a FD, the fence is reset to unsignaled because
                    // the transference is "copy" for exported FDs, so move the fence back into the
                    // unused pool.
                    mUnusedFences->push_back(fence);

                    // If we raced, and by the time that we got the FD, the fence was already
                    // complete, it's possible for |handle| to still be equal to
                    // |kInvalidHandle| even if we succeed, so check it again and handle it with
                    // a signaled receiver if necessary. We also update the fence to be
                    // VK_NULL_HANDLE in this case so that we don't try to export it again.
                    if (handle == utils::SystemHandle::kInvalidHandle) {
                        fence = VK_NULL_HANDLE;
                        return SystemEventReceiver::CreateAlreadySignaled();
                    } else {
                        it->fenceOrHandle = std::move(utils::SystemHandle::Duplicate(handle));
                        return SystemEventReceiver(utils::SystemHandle::Acquire(handle));
                    }
#elif DAWN_PLATFORM_IS(WINDOWS)
                    VkFenceGetWin32HandleInfoKHR info;
                    info.sType = VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR;
                    info.pNext = nullptr;
                    info.fence = fence;
                    info.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
                    DAWN_TRY(CheckVkSuccess(
                        device->fn.GetFenceWin32HandleKHR(device->GetVkDevice(), &info, &handle),
                        "vkGetFenceWin32HandleKHR"));

                    // For the Windows handle, the Vulkan specs suggest that the handle is of
                    // transference "reference" type which I believe means that each handle created
                    // here can be used independently, all while the fence is also still valid.
                    // Therefore, we don't want to override the fence in the entry, and instead just
                    // use the handle for this one use case.
                    DAWN_ASSERT(handle != utils::SystemHandle::kInvalidHandle);
                    return SystemEventReceiver(utils::SystemHandle::Acquire(handle));
#else
                    DAWN_UNREACHABLE();
#endif
                },
                [&](const utils::SystemHandle& exportedHandle)
                    -> ResultOrError<SystemEventReceiver> {
                    // Already exported. Just duplicate the handle.
                    return SystemEventReceiver(exportedHandle.Duplicate());
                });
        }

        // If we didn't find any fence/handle, then we can just return an already signaled receiver.
        return SystemEventReceiver::CreateAlreadySignaled();
    });
}

}  // namespace dawn::native::vulkan
