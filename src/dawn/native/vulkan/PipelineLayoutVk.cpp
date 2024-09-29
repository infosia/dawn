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

#include "dawn/native/vulkan/PipelineLayoutVk.h"

#include <string>
#include <utility>

#include "dawn/common/BitSetIterator.h"
#include "dawn/native/vulkan/BindGroupLayoutVk.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/FencedDeleter.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"

namespace dawn::native::vulkan {

// static
ResultOrError<Ref<PipelineLayout>> PipelineLayout::Create(
    Device* device,
    const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) {
    Ref<PipelineLayout> layout = AcquireRef(new PipelineLayout(device, descriptor));
    DAWN_TRY(layout->Initialize());
    return layout;
}

MaybeError PipelineLayout::Initialize() {
    // Compute the array of VkDescriptorSetLayouts that will be chained in the create info.
    // TODO(crbug.com/dawn/277) Vulkan doesn't allow holes in this array, should we expose
    // this constraints at the Dawn level?
    uint32_t numSetLayouts = 0;
    auto& setLayouts = mPipelineLayoutInfo.setLayouts;
    std::array<const CachedObject*, kMaxBindGroups> cachedObjects;
    for (BindGroupIndex setIndex : IterateBitSet(GetBindGroupLayoutsMask())) {
        const BindGroupLayoutInternalBase* bindGroupLayout = GetBindGroupLayout(setIndex);
        setLayouts[numSetLayouts] = ToBackend(bindGroupLayout)->GetHandle();
        cachedObjects[numSetLayouts] = bindGroupLayout;
        numSetLayouts++;
    }

    mPipelineLayoutInfo.vkCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    mPipelineLayoutInfo.vkCreateInfo.pNext = nullptr;
    mPipelineLayoutInfo.vkCreateInfo.flags = 0;
    mPipelineLayoutInfo.vkCreateInfo.setLayoutCount = numSetLayouts;
    mPipelineLayoutInfo.vkCreateInfo.pSetLayouts = AsVkArray(setLayouts.data());
    mPipelineLayoutInfo.vkCreateInfo.pushConstantRangeCount = 0;

    uint32_t immediateDataRangeByteSize = GetImmediateDataRangeByteSize();

    // Initialize VkPushConstantRange for non-zero immediate data range size.
    if (immediateDataRangeByteSize > 0) {
        // Customer immediate data range is unique among shader stages
        mPipelineLayoutInfo.InitializePushConstantRange(immediateDataRangeByteSize);
    }

    // Record cache key information now since the cachedObjects are not stored.
    StreamIn(&mCacheKey, stream::Iterable(cachedObjects.data(), numSetLayouts));  //, createInfo);

    std::unique_ptr<VkPipelineLayoutObject> vkLayoutObject =
        std::make_unique<VkPipelineLayoutObject>();

    VkPipelineLayout vkPipelineLayout;
    Device* device = ToBackend(GetDevice());
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreatePipelineLayout(device->GetVkDevice(), &mPipelineLayoutInfo.vkCreateInfo,
                                        nullptr, &*vkPipelineLayout),
        "CreatePipelineLayout"));

    vkLayoutObject->layout =
        AcquireRef(new RefCountedVkHandle<VkPipelineLayout>(device, vkPipelineLayout));

    StreamIn(&vkLayoutObject->cacheKey, GetCacheKey(), mPipelineLayoutInfo.vkCreateInfo);

    mVkPipelineLayouts.insert({0, std::move(vkLayoutObject)});

    return {};
}

ResultOrError<VkPipelineLayoutObject*> PipelineLayout::GetOrCreateVkLayoutObject(
    uint32_t internalImmediateDataRangeByteSize) {
    // Check cache
    if (mVkPipelineLayouts.contains(internalImmediateDataRangeByteSize)) {
        return mVkPipelineLayouts[internalImmediateDataRangeByteSize].get();
    }

    uint32_t immediateDataRangeByteSize =
        GetImmediateDataRangeByteSize() + internalImmediateDataRangeByteSize;
    if (mPipelineLayoutInfo.vkCreateInfo.pushConstantRangeCount == 0) {
        mPipelineLayoutInfo.InitializePushConstantRange(immediateDataRangeByteSize);
    } else {
        // Update exist push constant range.
        mPipelineLayoutInfo.pushConstantRange.size = immediateDataRangeByteSize;
    }

    std::unique_ptr<VkPipelineLayoutObject> vkLayoutObject =
        std::make_unique<VkPipelineLayoutObject>();
    VkPipelineLayout vkPipelineLayout;

    Device* device = ToBackend(GetDevice());
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreatePipelineLayout(device->GetVkDevice(), &mPipelineLayoutInfo.vkCreateInfo,
                                        nullptr, &*vkPipelineLayout),
        "CreatePipelineLayout"));

    vkLayoutObject->layout =
        AcquireRef(new RefCountedVkHandle<VkPipelineLayout>(device, vkPipelineLayout));

    StreamIn(&vkLayoutObject->cacheKey, GetCacheKey(), mPipelineLayoutInfo.vkCreateInfo);

    mVkPipelineLayouts.insert({internalImmediateDataRangeByteSize, std::move(vkLayoutObject)});

    return mVkPipelineLayouts[internalImmediateDataRangeByteSize].get();
}

PipelineLayout::~PipelineLayout() = default;

void PipelineLayout::DestroyImpl() {
    PipelineLayoutBase::DestroyImpl();
    mVkPipelineLayouts.clear();
}

void PipelineLayout::SetLabelImpl() {
    Device* device = ToBackend(GetDevice());
    const std::string& label = GetLabel();

    for (const auto& pair : mVkPipelineLayouts) {
        SetDebugName(device, pair.second->GetLayout(), "Dawn_PipelineLayout", label);
    }
}

}  // namespace dawn::native::vulkan
