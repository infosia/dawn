// Copyright 2024 The Dawn & Tint Authors
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

#ifndef DAWN_NATIVE_VULKAN_DESCRIPTORSETALLOCATORMANAGER_H_
#define DAWN_NATIVE_VULKAN_DESCRIPTORSETALLOCATORMANAGER_H_

#include "absl/container/flat_hash_map.h"
#include "dawn/common/Mutex.h"
#include "dawn/common/SerialQueue.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/vulkan/DescriptorSetAllocator.h"

namespace dawn::native::vulkan {

class Device;

class DescriptorSetAllocatorManager {
  public:
    explicit DescriptorSetAllocatorManager(Device* device);
    ~DescriptorSetAllocatorManager();

    Ref<DescriptorSetAllocator> Create(
        absl::flat_hash_map<VkDescriptorType, uint32_t> descriptorCountPerType);

    // Member functions after this point are only expected to be called by DescriptorSetAllocator.
    void EnqueueDeferredDeallocation(DescriptorSetAllocator* allocator, ExecutionSerial serial);
    void FinishDeallocationUpTo(ExecutionSerial serial);
    Mutex* GetMutex() { return &mMutex; }

  private:
    Device* const mDevice;

    // Mutex is used guard both mutable member variables in this class and access to mutable member
    // variables in all DescriptorSetAllocator created here.
    Mutex mMutex;

    SerialQueue<ExecutionSerial, Ref<DescriptorSetAllocator>> mPendingDeallocation;
};

}  // namespace  dawn::native::vulkan

#endif  // DAWN_NATIVE_VULKAN_DESCRIPTORSETALLOCATORMANAGER_H_
