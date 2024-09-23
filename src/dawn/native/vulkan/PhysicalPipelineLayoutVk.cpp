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

#include "dawn/native/vulkan/PhysicalPipelineLayoutVk.h"

#include "dawn/common/vulkan_platform.h"
#include "dawn/native/Device.h"
#include "dawn/native/Error.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/vulkan/BindGroupLayoutVk.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/FencedDeleter.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"

namespace dawn::native::vulkan {
ResultOrError<Ref<PhysicalPipelineLayout>> PhysicalPipelineLayout::Create(
    DeviceBase* device,
    PipelineLayoutBase* layout,
    uint32_t internalImmediateDataRangeByteSize) {
    Ref<PhysicalPipelineLayout> physicalPipelineLayout =
        AcquireRef(new PhysicalPipelineLayout(device, internalImmediateDataRangeByteSize));

    DAWN_TRY(physicalPipelineLayout->Initialize(layout));
    return physicalPipelineLayout;
}

MaybeError PhysicalPipelineLayout::Initialize(PipelineLayoutBase* layout) {
    uint32_t numSetLayouts = 0;
    std::array<VkDescriptorSetLayout, kMaxBindGroups> setLayouts;
    std::array<const CachedObject*, kMaxBindGroups> cachedObjects;
    for (BindGroupIndex setIndex : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
        const BindGroupLayoutInternalBase* bindGroupLayout = layout->GetBindGroupLayout(setIndex);
        setLayouts[numSetLayouts] = ToBackend(bindGroupLayout)->GetHandle();
        cachedObjects[numSetLayouts] = bindGroupLayout;
        numSetLayouts++;
    }

    VkPipelineLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.setLayoutCount = numSetLayouts;
    createInfo.pSetLayouts = AsVkArray(setLayouts.data());

    // Set push constants range
    VkPushConstantRange vkPushConstantRange;
    const uint32_t pushConstantRangeSize =
        layout->GetImmediateDataRangeByteSize() + GetInternalImmediateDataRangeByteSize();

    // Vulkan validation layer requires non-zero push constant range size.
    if (pushConstantRangeSize > 0) {
        vkPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // TODO(crbug.com/366291600): Offset here should be calculated instead of assuming clamp
        // depth offset always.
        vkPushConstantRange.offset = kClampFragDepthArgsOffset;
        vkPushConstantRange.size = pushConstantRangeSize;

        createInfo.pushConstantRangeCount = 1;
        createInfo.pPushConstantRanges = &vkPushConstantRange;
    } else {
        createInfo.pushConstantRangeCount = 0;
    }

    // Record cache key information now since the createInfo is not stored.
    StreamIn(&mCacheKey, stream::Iterable(cachedObjects.data(), numSetLayouts), createInfo);

    Device* device = ToBackend(GetDevice());
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreatePipelineLayout(device->GetVkDevice(), &createInfo, nullptr, &*mHandle),
        "CreatePipelineLayout"));

    SetLabelFromPipelineLayout(layout->GetLabel());

    return {};
}

VkPipelineLayout PhysicalPipelineLayout::GetHandle() const {
    return mHandle;
}

const CacheKey& PhysicalPipelineLayout::GetCacheKey() const {
    return mCacheKey;
}

uint32_t PhysicalPipelineLayout::GetInternalImmediateDataRangeByteSize() const {
    return mInternalImmediateDataRangeByteSize;
}

void PhysicalPipelineLayout::Destroy() {
    if (mHandle != VK_NULL_HANDLE) {
        ToBackend(mDevice)->GetFencedDeleter()->DeleteWhenUnused(mHandle);
        mHandle = VK_NULL_HANDLE;
    }
}

void PhysicalPipelineLayout::SetLabelFromPipelineLayout(std::string label) {
    SetDebugName(ToBackend(mDevice), mHandle, "Dawn_PipelineLayout", label);
}

}  // namespace dawn::native::vulkan
