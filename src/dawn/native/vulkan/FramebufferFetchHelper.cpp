// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/native/vulkan/FramebufferFetchHelper.h"

#include <utility>

#include "dawn/common/Log.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/ShaderModuleVk.h"
#include "dawn/native/vulkan/TextureVk.h"
#include "dawn/native/vulkan/VulkanError.h"
#include "vulkan/vulkan_core.h"

namespace dawn::native::vulkan {
namespace {

ResultOrError<VkDescriptorSetLayout> MakeFramebufferFetchLayout(Device* device) {
    std::array<VkDescriptorSetLayoutBinding, kMaxColorAttachments> bindings;
    for (uint32_t i = 0; i < kMaxColorAttachments; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreateDescriptorSetLayout(device->GetVkDevice(), &createInfo, nullptr, &*layout),
        "CreateDescriptorSetLayout"));
    return layout;
}

}  // namespace

MaybeError FramebufferFetchHelper::Initialize(Device* device) {
    DAWN_ASSERT(!mDevice);

    DAWN_TRY_ASSIGN(mLayout, MakeFramebufferFetchLayout(device));

    absl::flat_hash_map<VkDescriptorType, uint32_t> descriptorCountPerType;
    descriptorCountPerType[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT] = kMaxColorAttachments;
    mDescriptorAllocator =
        DescriptorSetAllocator::Create(device, std::move(descriptorCountPerType));

    mDevice = device;
    return {};
}

void FramebufferFetchHelper::Destroy() {
    if (mLayout != VK_NULL_HANDLE) {
        mDevice->fn.DestroyDescriptorSetLayout(mDevice->GetVkDevice(), mLayout, nullptr);
        mLayout = VK_NULL_HANDLE;
    }

    mDescriptorAllocator = nullptr;
    mDevice = nullptr;
}

const VkDescriptorSetLayout& FramebufferFetchHelper::GetLayout() const {
    DAWN_ASSERT(mLayout != VK_NULL_HANDLE);
    return mLayout;
}

ResultOrError<VkDescriptorSet> FramebufferFetchHelper::CreateDescriptorSet(
    const BeginRenderPassCmd* cmd) {
    DAWN_ASSERT(mLayout != VK_NULL_HANDLE);

    // Needed to work around some quirks introduced by vulkan_platform.h which causes some platforms
    // to hit compiler errors if Vulkan struct members are assigned to VK_NULL_HANDLE directly.
    static const VkSampler nullSampler = VK_NULL_HANDLE;

    DescriptorSetAllocation allocation;
    DAWN_TRY_ASSIGN(allocation, mDescriptorAllocator->Allocate(mLayout));
    VkDescriptorSet set = allocation.set;

    std::array<VkWriteDescriptorSet, kMaxColorAttachments> writes;
    std::array<VkDescriptorImageInfo, kMaxColorAttachments> imageInfos;
    uint32_t writeCount = 0;

    for (auto i : cmd->attachmentState->GetColorAttachmentsMask()) {
        auto& attachmentInfo = cmd->colorAttachments[i];
        TextureView* textureView = ToBackend(attachmentInfo.view.Get());

        VkImageView imageView;
        if (textureView->GetDimension() == wgpu::TextureViewDimension::e3D) {
            DAWN_TRY_ASSIGN(imageView,
                            textureView->GetOrCreate2DViewOn3D(attachmentInfo.depthSlice));
        } else {
            imageView = textureView->GetHandle();
        }

        imageInfos[writeCount].sampler = nullSampler;
        imageInfos[writeCount].imageView = imageView;
        imageInfos[writeCount].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writes[writeCount].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[writeCount].pNext = nullptr;
        writes[writeCount].dstSet = set;
        writes[writeCount].dstBinding = static_cast<uint32_t>(i);
        writes[writeCount].dstArrayElement = 0;
        writes[writeCount].descriptorCount = 1;
        writes[writeCount].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[writeCount].pImageInfo = &imageInfos[writeCount];
        writes[writeCount].pBufferInfo = nullptr;
        writes[writeCount].pTexelBufferView = nullptr;

        writeCount++;
    }

    if (writeCount > 0) {
        mDevice->fn.UpdateDescriptorSets(mDevice->GetVkDevice(), writeCount, writes.data(), 0,
                                         nullptr);
    }

    // This will delete the VkDescriptorSet after the pending command serial has completed.
    mDescriptorAllocator->Deallocate(&allocation);

    return set;
}

}  // namespace dawn::native::vulkan
