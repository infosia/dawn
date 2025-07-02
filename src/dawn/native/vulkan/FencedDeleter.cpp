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

namespace dawn::native::vulkan {

FencedDeleter::FencedDeleter(Device* device) : mDevice(device) {
    mThread = std::thread(&FencedDeleter::ThreadLoop, this);
}

FencedDeleter::~FencedDeleter() {
    std::unique_lock<std::mutex> lock(mMutex);
    DAWN_ASSERT(mBuffersToDelete.Empty());
    DAWN_ASSERT(mDescriptorPoolsToDelete.Empty());
    DAWN_ASSERT(mFencesToDelete.Empty());
    DAWN_ASSERT(mFramebuffersToDelete.Empty());
    DAWN_ASSERT(mImagesToDelete.Empty());
    DAWN_ASSERT(mImageViewsToDelete.Empty());
    DAWN_ASSERT(mMemoriesToDelete.Empty());
    DAWN_ASSERT(mPipelinesToDelete.Empty());
    DAWN_ASSERT(mPipelineLayoutsToDelete.Empty());
    DAWN_ASSERT(mQueryPoolsToDelete.Empty());
    DAWN_ASSERT(mRenderPassesToDelete.Empty());
    DAWN_ASSERT(mSamplerYcbcrConversionsToDelete.Empty());
    DAWN_ASSERT(mSamplersToDelete.Empty());
    DAWN_ASSERT(mSemaphoresToDelete.Empty());
    DAWN_ASSERT(mSurfacesToDelete.Empty());
    DAWN_ASSERT(mSwapChainsToDelete.Empty());
}

void Destroy() {
    mThreadShouldExit = true;
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkBuffer buffer) {
    std::unique_lock<std::mutex> lock(mMutex);
    mBuffersToDelete.Enqueue(buffer, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkDescriptorPool pool) {
    std::unique_lock<std::mutex> lock(mMutex);
    mDescriptorPoolsToDelete.Enqueue(pool, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkDeviceMemory memory) {
    std::unique_lock<std::mutex> lock(mMutex);
    mMemoriesToDelete.Enqueue(memory, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkFence fence) {
    std::unique_lock<std::mutex> lock(mMutex);
    mFencesToDelete.Enqueue(fence, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkFramebuffer framebuffer) {
    std::unique_lock<std::mutex> lock(mMutex);
    mFramebuffersToDelete.Enqueue(framebuffer, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkImage image) {
    std::unique_lock<std::mutex> lock(mMutex);
    mImagesToDelete.Enqueue(image, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkImageView view) {
    std::unique_lock<std::mutex> lock(mMutex);
    mImageViewsToDelete.Enqueue(view, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkPipeline pipeline) {
    std::unique_lock<std::mutex> lock(mMutex);
    mPipelinesToDelete.Enqueue(pipeline, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkPipelineLayout layout) {
    std::unique_lock<std::mutex> lock(mMutex);
    mPipelineLayoutsToDelete.Enqueue(layout, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkQueryPool querypool) {
    std::unique_lock<std::mutex> lock(mMutex);
    mQueryPoolsToDelete.Enqueue(querypool, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkRenderPass renderPass) {
    std::unique_lock<std::mutex> lock(mMutex);
    mRenderPassesToDelete.Enqueue(renderPass, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkSamplerYcbcrConversion samplerYcbcrConversion) {
    std::unique_lock<std::mutex> lock(mMutex);
    mSamplerYcbcrConversionsToDelete.Enqueue(samplerYcbcrConversion, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkSampler sampler) {
    std::unique_lock<std::mutex> lock(mMutex);
    mSamplersToDelete.Enqueue(sampler, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkSemaphore semaphore) {
    std::unique_lock<std::mutex> lock(mMutex);
    mSemaphoresToDelete.Enqueue(semaphore, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkSurfaceKHR surface) {
    std::unique_lock<std::mutex> lock(mMutex);
    mSurfacesToDelete.Enqueue(surface, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

void FencedDeleter::DeleteWhenUnused(VkSwapchainKHR swapChain) {
    std::unique_lock<std::mutex> lock(mMutex);
    mSwapChainsToDelete.Enqueue(swapChain, GetCurrentDeletionSerial());
    mHasDeletions.exchange(true);
    mHasWorkCondition.notify_one();
}

ExecutionSerial FencedDeleter::GetLastPendingDeletionSerial() {
    std::unique_lock<std::mutex> lock(mMutex);

    ExecutionSerial lastSerial = kBeginningOfGPUTime;
    auto GetLastSubmitted = [&lastSerial](auto& queue) {
        if (!queue.Empty()) {
            lastSerial = std::max(lastSerial, queue.LastSerial());
        }
    };

    GetLastSubmitted(mBuffersToDelete);
    GetLastSubmitted(mDescriptorPoolsToDelete);
    GetLastSubmitted(mFencesToDelete);
    GetLastSubmitted(mFramebuffersToDelete);
    GetLastSubmitted(mImagesToDelete);
    GetLastSubmitted(mImageViewsToDelete);
    GetLastSubmitted(mMemoriesToDelete);
    GetLastSubmitted(mPipelinesToDelete);
    GetLastSubmitted(mPipelineLayoutsToDelete);
    GetLastSubmitted(mQueryPoolsToDelete);
    GetLastSubmitted(mRenderPassesToDelete);
    GetLastSubmitted(mSamplerYcbcrConversionsToDelete);
    GetLastSubmitted(mSamplersToDelete);
    GetLastSubmitted(mSemaphoresToDelete);
    GetLastSubmitted(mSurfacesToDelete);
    GetLastSubmitted(mSwapChainsToDelete);

    return lastSerial;
}

ExecutionSerial FencedDeleter::GetCurrentDeletionSerial() {
    return mDevice->GetQueue()->GetPendingCommandSerial();
}

void FencedDeleter::Tick(ExecutionSerial completedSerial) {
    std::unique_lock<std::mutex> lock(mMutex);
    ProcessDeletions(completedSerial);
}

void FencedDeleter::ProcessDeletions(ExecutionSerial completedSerial) {
    VkDevice vkDevice = mDevice->GetVkDevice();
    VkInstance instance = mDevice->GetVkInstance();

    mHasDeletions.exchange(false);

    // Buffers and images must be deleted before memories because it is invalid to free memory
    // that still have resources bound to it.
    for (VkBuffer buffer : mBuffersToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyBuffer(vkDevice, buffer, nullptr);
    }
    mBuffersToDelete.ClearUpTo(completedSerial);
    for (VkImage image : mImagesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyImage(vkDevice, image, nullptr);
    }
    mImagesToDelete.ClearUpTo(completedSerial);

    for (VkDeviceMemory memory : mMemoriesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.FreeMemory(vkDevice, memory, nullptr);
    }
    mMemoriesToDelete.ClearUpTo(completedSerial);

    for (VkPipelineLayout layout : mPipelineLayoutsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyPipelineLayout(vkDevice, layout, nullptr);
    }
    mPipelineLayoutsToDelete.ClearUpTo(completedSerial);

    for (VkRenderPass renderPass : mRenderPassesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyRenderPass(vkDevice, renderPass, nullptr);
    }
    mRenderPassesToDelete.ClearUpTo(completedSerial);

    for (VkFence fence : mFencesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyFence(vkDevice, fence, nullptr);
    }
    mFencesToDelete.ClearUpTo(completedSerial);

    for (VkFramebuffer framebuffer : mFramebuffersToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyFramebuffer(vkDevice, framebuffer, nullptr);
    }
    mFramebuffersToDelete.ClearUpTo(completedSerial);

    for (VkImageView view : mImageViewsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyImageView(vkDevice, view, nullptr);
    }
    mImageViewsToDelete.ClearUpTo(completedSerial);

    for (VkPipeline pipeline : mPipelinesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyPipeline(vkDevice, pipeline, nullptr);
    }
    mPipelinesToDelete.ClearUpTo(completedSerial);

    // Vulkan swapchains must be destroyed before their corresponding VkSurface
    for (VkSwapchainKHR swapChain : mSwapChainsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroySwapchainKHR(vkDevice, swapChain, nullptr);
    }
    mSwapChainsToDelete.ClearUpTo(completedSerial);
    for (VkSurfaceKHR surface : mSurfacesToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroySurfaceKHR(instance, surface, nullptr);
    }
    mSurfacesToDelete.ClearUpTo(completedSerial);

    for (VkSemaphore semaphore : mSemaphoresToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroySemaphore(vkDevice, semaphore, nullptr);
    }
    mSemaphoresToDelete.ClearUpTo(completedSerial);

    for (VkDescriptorPool pool : mDescriptorPoolsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyDescriptorPool(vkDevice, pool, nullptr);
    }
    mDescriptorPoolsToDelete.ClearUpTo(completedSerial);

    for (VkQueryPool pool : mQueryPoolsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroyQueryPool(vkDevice, pool, nullptr);
    }
    mQueryPoolsToDelete.ClearUpTo(completedSerial);

    for (VkSamplerYcbcrConversion samplerYcbcrConversion :
         mSamplerYcbcrConversionsToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroySamplerYcbcrConversion(vkDevice, samplerYcbcrConversion, nullptr);
    }
    mSamplerYcbcrConversionsToDelete.ClearUpTo(completedSerial);

    for (VkSampler sampler : mSamplersToDelete.IterateUpTo(completedSerial)) {
        mDevice->fn.DestroySampler(vkDevice, sampler, nullptr);
    }
    mSamplersToDelete.ClearUpTo(completedSerial);
}

void FencedDeleter::ThreadLoop() {
    while(1) {
        std::unique_lock<std::mutex> lock(mMutex);
        mHasWorkCondition.wait(lock, [this]() { return mThreadShouldExit || mHasDeletions; });

        if (mThreadShouldExit) {
            break;
        }

        ExecutionSerial completedSerial = GetCurrentDeletionSerial();
        ProcessDeletions(completedSerial)
    }
}

}  // namespace dawn::native::vulkan
