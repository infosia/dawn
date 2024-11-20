// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dawn/native/vulkan/DescriptorSetAllocatorManager.h"

#include <utility>

#include "dawn/common/Ref.h"
#include "dawn/native/vulkan/DeviceVk.h"

namespace dawn::native::vulkan {

DescriptorSetAllocatorManager::DescriptorSetAllocatorManager(Device* device) : mDevice(device) {}

DescriptorSetAllocatorManager::~DescriptorSetAllocatorManager() = default;

Ref<DescriptorSetAllocator> DescriptorSetAllocatorManager::Create(
    absl::flat_hash_map<VkDescriptorType, uint32_t> descriptorCountPerType) {
    return AcquireRef(new DescriptorSetAllocator(mDevice, this, std::move(descriptorCountPerType)));
}

void DescriptorSetAllocatorManager::EnqueueDeferredDeallocation(DescriptorSetAllocator* allocator,
                                                                ExecutionSerial serial) {
    DAWN_ASSERT(mMutex.IsLockedByCurrentThread());
    mPendingDeallocation.Enqueue(allocator, serial);
}

void DescriptorSetAllocatorManager::FinishDeallocationUpTo(ExecutionSerial serial) {
    Mutex::AutoLock lock(&mMutex);
    for (Ref<DescriptorSetAllocator>& allocator : mPendingDeallocation.IterateUpTo(serial)) {
        allocator->FinishDeallocation(serial);
    }
    mPendingDeallocation.ClearUpTo(serial);
}

}  // namespace dawn::native::vulkan
