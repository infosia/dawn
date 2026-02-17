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

#ifndef SRC_DAWN_NATIVE_VULKAN_QUEUEVK_H_
#define SRC_DAWN_NATIVE_VULKAN_QUEUEVK_H_

#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "dawn/common/SerialQueue.h"
#include "dawn/common/vulkan_platform.h"
#include "dawn/native/Device.h"
#include "dawn/native/Queue.h"
#include "dawn/native/vulkan/CommandRecordingContextVk.h"

namespace dawn::native::vulkan {

class Device;

class Queue final : public QueueBase {
  public:
    static ResultOrError<Ref<Queue>> Create(Device* device,
                                            const QueueDescriptor* descriptor,
                                            uint32_t family);

    VkQueue GetVkQueue() const;

    CommandRecordingContext* GetPendingRecordingContext(SubmitMode submitMode = SubmitMode::Normal);
    MaybeError SplitRecordingContext(CommandRecordingContext* recordingContext);

  private:
    Queue(Device* device, const QueueDescriptor* descriptor, uint32_t family);
    ~Queue() override;
    using QueueBase::QueueBase;

    MaybeError Initialize();

    MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
    bool HasPendingCommands() const override;
    ResultOrError<ExecutionSerial> CheckAndUpdateCompletedSerials() override;
    void ForceEventualFlushOfCommands() override;
    MaybeError WaitForIdleForDestructionImpl() override;
    MaybeError SubmitPendingCommandsImpl() override;
    void DestroyImpl(DestroyReason reason) override;

    // Dawn API
    void SetLabelImpl() override;

    platform::JobStatus WaitForNextFenceJobLoop();
    ResultOrError<VkFence> GetUnusedFence();

    // We track which operations are in flight on the GPU with an increasing serial.
    // This works only because we have a single queue. Each submit to a queue is associated
    // to a serial and a fence, such that when the fence is "ready" we know the operations
    // have finished.
    MutexCondVarProtected<std::deque<std::pair<VkFence, ExecutionSerial>>> mFencesInFlight;

    // Fences in the unused list aren't reset yet.
    MutexProtected<std::vector<VkFence>> mUnusedFences;

    MaybeError PrepareRecordingContext();
    ResultOrError<CommandPoolAndBuffer> BeginVkCommandBuffer();

    // Command pools in the unused list haven't been reset yet.
    MutexProtected<std::vector<CommandPoolAndBuffer>> mUnusedCommands;
    // There is always a valid recording context stored in mRecordingContext
    CommandRecordingContext mRecordingContext;

    uint32_t mQueueFamily = 0;
    VkQueue mQueue = VK_NULL_HANDLE;

    // In Vulkan at the moment, we need a dedicated thread per device to wait on fences because the
    // WaitForFences API is per device. Since device and queue mapping is currently 1:1, own the
    // long running job handle here.
    std::unique_ptr<platform::JobHandle> mWaitingJobHandle = nullptr;
};

}  // namespace dawn::native::vulkan

#endif  // SRC_DAWN_NATIVE_VULKAN_QUEUEVK_H_
