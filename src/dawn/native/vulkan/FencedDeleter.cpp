// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/vulkan/FencedDeleter.h"

#include <algorithm>

#include "dawn/native/Queue.h"
#include "dawn/native/vulkan/DeviceVk.h"

// Listed in order of deletion.
//  - Buffers and images must be deleted before memories because it is invalid to free memory
// that still have resources bound to it.
//  - Vulkan swapchains must be destroyed before their corresponding VkSurface
#define FENCED_DEVICE_DELETER_TYPES(X)                                                           \
    X(VkBuffer, mBuffersToDelete, DestroyBuffer)                                                 \
    X(VkImage, mImagesToDelete, DestroyImage)                                                    \
    X(VkDeviceMemory, mMemoriesToDelete, FreeMemory)                                             \
    X(VkPipelineLayout, mPipelineLayoutsToDelete, DestroyPipelineLayout)                         \
    X(VkRenderPass, mRenderPassesToDelete, DestroyRenderPass)                                    \
    X(VkFence, mFencesToDelete, DestroyFence)                                                    \
    X(VkFramebuffer, mFramebuffersToDelete, DestroyFramebuffer)                                  \
    X(VkImageView, mImageViewsToDelete, DestroyImageView)                                        \
    X(VkPipeline, mPipelinesToDelete, DestroyPipeline)                                           \
    X(VkSwapchainKHR, mSwapChainsToDelete, DestroySwapchainKHR)                                  \
    X(VkSemaphore, mSemaphoresToDelete, DestroySemaphore)                                        \
    X(VkDescriptorPool, mDescriptorPoolsToDelete, DestroyDescriptorPool)                         \
    X(VkQueryPool, mQueryPoolsToDelete, DestroyQueryPool)                                        \
    X(VkSamplerYcbcrConversion, mSamplerYcbcrConversionsToDelete, DestroySamplerYcbcrConversion) \
    X(VkSampler, mSamplersToDelete, DestroySampler)

#define FENCED_INSTANCE_DELETER_TYPES(X) X(VkSurfaceKHR, mSurfacesToDelete, DestroySurfaceKHR)

#define FENCED_DELETER_TYPES(X)    \
    FENCED_DEVICE_DELETER_TYPES(X) \
    FENCED_INSTANCE_DELETER_TYPES(X)

namespace dawn::native::vulkan {

FencedDeleter::FencedDeleter(Device* device) : mDevice(device) {}

FencedDeleter::~FencedDeleter() {
    std::unique_lock<std::mutex> lock(mMutex);
#define X(Type, queue, ...) DAWN_ASSERT(queue.Empty());
    FENCED_DELETER_TYPES(X)
#undef X
}

#define X(Type, queue, ...)                                          \
    void FencedDeleter::DeleteWhenUnused(Type handle) {              \
        std::unique_lock<std::mutex> lock(mMutex);                   \
        ExecutionSerial deletionSerial = GetCurrentDeletionSerial(); \
        queue.Enqueue(handle, deletionSerial);                       \
        EnsureTask(deletionSerial);                                  \
    }
FENCED_DELETER_TYPES(X)
#undef X

ExecutionSerial FencedDeleter::GetLastPendingDeletionSerial() {
    std::unique_lock<std::mutex> lock(mMutex);
    ExecutionSerial lastSerial = kBeginningOfGPUTime;
#define X(Type, queue, ...)                                    \
    if (!queue.Empty()) {                                      \
        lastSerial = std::max(lastSerial, queue.LastSerial()); \
    }
    FENCED_DELETER_TYPES(X)
#undef X
    return lastSerial;
}

ExecutionSerial FencedDeleter::GetCurrentDeletionSerial() {
    return mDevice->GetQueue()->GetPendingCommandSerial();
}

void FencedDeleter::Tick(ExecutionSerial completedSerial) {
    std::unique_lock<std::mutex> lock(mMutex);
    VkDevice vkDevice = mDevice->GetVkDevice();
    VkInstance instance = mDevice->GetVkInstance();

#define X(Type, queue, DeleteFn)                             \
    for (Type handle : queue.IterateUpTo(completedSerial)) { \
        mDevice->fn.DeleteFn(vkDevice, handle, nullptr);     \
    }                                                        \
    queue.ClearUpTo(completedSerial);
    FENCED_DEVICE_DELETER_TYPES(X)
#undef X

#define X(Type, queue, DeleteFn)                             \
    for (Type handle : queue.IterateUpTo(completedSerial)) { \
        mDevice->fn.DeleteFn(instance, handle, nullptr);     \
    }                                                        \
    queue.ClearUpTo(completedSerial);
    FENCED_INSTANCE_DELETER_TYPES(X)
#undef X
}

void FencedDeleter::EnsureTask(ExecutionSerial deletionSerial) {
    // Schedules a new task to process deletions if the deletion serial is newer than the one for
    // the last scheduled task.
    if (mLastTaskSerial.exchange(deletionSerial) < deletionSerial) {
        mDevice->GetQueue()->TrackSerialTask(deletionSerial,
                                             [this, deletionSerial]() { Tick(deletionSerial); });
    }
}

}  // namespace dawn::native::vulkan
