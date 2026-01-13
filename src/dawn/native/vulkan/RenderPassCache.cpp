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

#include "dawn/native/vulkan/RenderPassCache.h"

#include <vector>

#include "absl/container/inlined_vector.h"
#include "dawn/common/Enumerator.h"
#include "dawn/common/HashUtils.h"
#include "dawn/common/Log.h"
#include "dawn/common/Range.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/TextureVk.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"

namespace dawn::native::vulkan {

namespace {

// Contains the attachment description that will be chained in the create info
// The order of all attachments in attachmentDescs is "color-depthstencil-resolve".
constexpr uint8_t kMaxAttachmentCount = kMaxColorAttachments * 2 + 1;

struct RenderPassInfo {
    PerColorAttachment<VkAttachmentReference> colorAttachmentRefs;
    PerColorAttachment<VkAttachmentReference> resolveAttachmentRefs;
    PerColorAttachment<VkAttachmentReference> inputAttachmentRefs;
    VkAttachmentReference depthStencilAttachmentRef;

    std::array<VkAttachmentDescription, kMaxAttachmentCount> attachmentDescs;
    std::array<VkSubpassDescription, 2> subpassDescs;
    std::array<VkSubpassDependency, 2> subpassDependencies;

    VkRenderPassCreateInfo createInfo;
}

struct RenderPassInfo2 {
    PerColorAttachment<VkAttachmentReference2> colorAttachmentRefs;
    PerColorAttachment<VkAttachmentReference2> resolveAttachmentRefs;
    PerColorAttachment<VkAttachmentReference2> inputAttachmentRefs;
    VkAttachmentReference2 depthStencilAttachmentRef;

    std::array<VkAttachmentDescription2, kMaxAttachmentCount> attachmentDescs;
    std::array<VkSubpassDescription2, 2> subpassDescs;
    std::array<VkSubpassDependency2, 2> subpassDependencies;

    VkRenderPassCreateInfo2 createInfo;
}

// TODO: Initialize with the right Vulkan struct enums

template InitializeRenderPassCreateInfo<T>(Device* device, const RenderPassCacheQuery& query, T& passInfo) {
    // The Vulkan subpasses want to know the layout of the attachments with VkAttachmentRef.
    // Precompute them as they must be pointer-chained in VkSubpassDescription.
    // Note that both colorAttachmentRefs and resolveAttachmentRefs can be sparse with holes
    // filled with VK_ATTACHMENT_UNUSED.

    for (auto i : Range(kMaxColorAttachmentsTyped)) {
        passInfo.colorAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        passInfo.resolveAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        passInfo.inputAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        // The Khronos Vulkan validation layer will complain if not set
        passInfo.colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        passInfo.resolveAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        passInfo.inputAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkSampleCountFlagBits vkSampleCount = VulkanSampleCount(query.sampleCount);

    uint32_t attachmentCount = 0;
    ColorAttachmentIndex highestColorAttachmentIndexPlusOne(static_cast<uint8_t>(0));
    for (auto i : query.colorMask) {
        auto& attachmentRef = passInfo.colorAttachmentRefs[i];
        auto& attachmentDesc = passInfo.attachmentDescs[attachmentCount];

        attachmentRef.attachment = attachmentCount;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentDesc.flags = 0;
        attachmentDesc.format = VulkanImageFormat(device, query.colorFormats[i]);
        attachmentDesc.samples = vkSampleCount;
        attachmentDesc.loadOp = VulkanAttachmentLoadOp(query.colorLoadOp[i]);
        attachmentDesc.storeOp = VulkanAttachmentStoreOp(query.colorStoreOp[i]);
        attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentCount++;
        highestColorAttachmentIndexPlusOne =
            ColorAttachmentIndex(static_cast<uint8_t>(static_cast<uint8_t>(i) + 1u));
    }

    VkAttachmentReference* depthStencilAttachment = nullptr;
    if (query.hasDepthStencil) {
        const Format& dsFormat = device->GetValidInternalFormat(query.depthStencilFormat);

        depthStencilAttachment = &passInfo.depthStencilAttachmentRef;
        passInfo.depthStencilAttachmentRef.attachment = attachmentCount;
        passInfo.depthStencilAttachmentRef.layout = VulkanImageLayoutForDepthStencilAttachment(
            dsFormat, query.depthReadOnly, query.stencilReadOnly);

        // Build the attachment descriptor.
        auto& attachmentDesc = passInfo.attachmentDescs[attachmentCount];
        attachmentDesc.flags = 0;
        attachmentDesc.format = VulkanImageFormat(device, dsFormat.format);
        attachmentDesc.samples = vkSampleCount;

        attachmentDesc.loadOp = VulkanAttachmentLoadOp(query.depthLoadOp);
        attachmentDesc.storeOp = VulkanAttachmentStoreOp(query.depthStoreOp);
        attachmentDesc.stencilLoadOp = VulkanAttachmentLoadOp(query.stencilLoadOp);
        attachmentDesc.stencilStoreOp = VulkanAttachmentStoreOp(query.stencilStoreOp);

        // There is only one subpass, so it is safe to set both initialLayout and finalLayout to
        // the only subpass's layout.
        attachmentDesc.initialLayout = passInfo.depthStencilAttachmentRef.layout;
        attachmentDesc.finalLayout = passInfo.depthStencilAttachmentRef.layout;

        attachmentCount++;
    }

    uint32_t resolveAttachmentCount = 0;
    ColorAttachmentIndex highestInputAttachmentIndex(static_cast<uint8_t>(0));

    for (auto i : query.resolveTargetMask) {
        auto& resolveAttachmentRef = passInfo.resolveAttachmentRefs[i];
        auto& resolveAttachmentDesc = passInfo.attachmentDescs[attachmentCount];

        resolveAttachmentRef.attachment = attachmentCount;
        resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        resolveAttachmentDesc.flags = 0;
        resolveAttachmentDesc.format = VulkanImageFormat(mDevice, query.colorFormats[i]);
        resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (query.expandResolveMask.test(i)) {
            resolveAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            resolveAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            inputAttachmentRefs[i].attachment = resolveAttachmentRef.attachment;

            highestInputAttachmentIndex = i;
        } else {
            resolveAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        resolveAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentCount++;
        resolveAttachmentCount++;
    }

    if (query.expandResolveMask.any()) {
        // To simulate ExpandResolveTexture, we use two subpasses. The first subpass will read the
        // resolve texture as input attachment.
        passInfo.subpassDescs.push_back({});
        VkSubpassDescription& subpassDesc = passInfo.subpassDescs.back();
        subpassDesc.flags = 0;
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.inputAttachmentCount = static_cast<uint8_t>(highestInputAttachmentIndex) + 1;
        subpassDesc.pInputAttachments = passInfo.inputAttachmentRefs.data();
        subpassDesc.colorAttachmentCount = static_cast<uint8_t>(highestColorAttachmentIndexPlusOne);
        subpassDesc.pColorAttachments = passInfo.colorAttachmentRefs.data();
        subpassDesc.pDepthStencilAttachment = passInfo.depthStencilAttachment;

        InitializeLoadResolveSubpassDependencies(&subpassDependencies);
    }

    // Create the VkSubpassDescription that will be chained in the VkRenderPasspassInfo
    passInfo.subpassDescs.push_back({});
    VkSubpassDescription& subpassDesc = passInfo.subpassDescs.back();
    subpassDesc.flags = 0;
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = nullptr;
    subpassDesc.colorAttachmentCount = static_cast<uint8_t>(highestColorAttachmentIndexPlusOne);
    subpassDesc.pColorAttachments = passInfo.colorAttachmentRefs.data();

    // Qualcomm GPUs have a driver bug on some devices where passing a zero-length array to the
    // resolveAttachments causes a VK_ERROR_OUT_OF_HOST_MEMORY. nullptr must be passed instead.
    if (resolveAttachmentCount) {
        subpassDesc.pResolveAttachments = passInfo.resolveAttachmentRefs.data();
    } else {
        subpassDesc.pResolveAttachments = nullptr;
    }

    subpassDesc.pDepthStencilAttachment = passInfo.depthStencilAttachment;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = nullptr;

    // Chain everything in VkRenderPassCreateInfo
    passInfo.createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.createInfo.pNext = nullptr;
    passInfo.createInfo.flags = 0;
    passInfo.createInfo.attachmentCount = attachmentCount;
    passInfo.createInfo.pAttachments = passInfo.attachmentDescs.data();
    passInfo.createInfo.subpassCount = passInfo.subpassDescs.size();
    passInfo.createInfo.pSubpasses = passInfo.subpassDescs.data();
    passInfo.createInfo.dependencyCount = passInfo.subpassDependencies.size();
    passInfo.createInfo.pDependencies = passInfo.subpassDependencies.data();
}

void InitializeLoadResolveSubpassDependencies(
    absl::InlinedVector<VkSubpassDependency, 2>* subpassDependenciesOut) {
    VkSubpassDependency dependencies[2];
    // Dependency for resolve texture's read -> resolve texture's write.
    dependencies[0].srcSubpass = 0;
    dependencies[0].dstSubpass = 1;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Dependency for color write in subpass 0 -> color write in subpass 1
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    subpassDependenciesOut->insert(subpassDependenciesOut->end(), std::begin(dependencies),
                                   std::end(dependencies));
}
}  // anonymous namespace

// RenderPassCacheQuery

void RenderPassCacheQuery::SetColor(ColorAttachmentIndex index,
                                    wgpu::TextureFormat format,
                                    wgpu::LoadOp loadOp,
                                    wgpu::StoreOp storeOp,
                                    bool hasResolveTarget) {
    colorMask.set(index);
    colorFormats[index] = format;
    colorLoadOp[index] = loadOp;
    colorStoreOp[index] = storeOp;
    resolveTargetMask[index] = hasResolveTarget;
    expandResolveMask.set(index, loadOp == wgpu::LoadOp::ExpandResolveTexture);
}

void RenderPassCacheQuery::SetDepthStencil(wgpu::TextureFormat format,
                                           wgpu::LoadOp depthLoadOpIn,
                                           wgpu::StoreOp depthStoreOpIn,
                                           bool depthReadOnlyIn,
                                           wgpu::LoadOp stencilLoadOpIn,
                                           wgpu::StoreOp stencilStoreOpIn,
                                           bool stencilReadOnlyIn) {
    hasDepthStencil = true;
    depthStencilFormat = format;
    depthLoadOp = depthLoadOpIn;
    depthStoreOp = depthStoreOpIn;
    depthReadOnly = depthReadOnlyIn;
    stencilLoadOp = stencilLoadOpIn;
    stencilStoreOp = stencilStoreOpIn;
    stencilReadOnly = stencilReadOnlyIn;
}

void RenderPassCacheQuery::SetSampleCount(uint32_t sampleCountIn) {
    sampleCount = sampleCountIn;
}

// RenderPassCache

RenderPassCache::RenderPassCache(Device* device) : mDevice(device) {}

RenderPassCache::~RenderPassCache() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto [_, renderPassInfo] : mCache) {
        mDevice->fn.DestroyRenderPass(mDevice->GetVkDevice(), renderPassInfo.renderPass, nullptr);
    }

    mCache.clear();
}

ResultOrError<RenderPassCache::RenderPassInfo> RenderPassCache::GetRenderPass(
    const RenderPassCacheQuery& query) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mCache.find(query);
    if (it != mCache.end()) {
        return RenderPassInfo(it->second);
    }

    RenderPassInfo renderPass;
    DAWN_TRY_ASSIGN(renderPass, CreateRenderPassForQuery(query));
    mCache.emplace(query, renderPass);
    return renderPass;
}

ResultOrError<RenderPassCache::RenderPassInfo> RenderPassCache::CreateRenderPassForQuery(
    const RenderPassCacheQuery& query) {

    if (mDevice->IsToggleEnabled(Toggle::VulkanUseCreateRenderPass2)) {
        dawn::ErrorLog() << "VulkanUseCreateRenderPass2";

        RenderPassInfo2 passInfo2;
        InitializeRenderPassCreateInfo(mDevice, query, passInfo2);

        /*VkMultisampledRenderToSingleSampledInfoEXT msrtss = {};
        if (query.renderToSingleSampled) {
            msrtss.sType = VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT;
            msrtss.pNext = nullptr;
            msrtss.multisampledRenderToSingleSampledEnable = VK_TRUE;
            msrtss.rasterizationSamples = VulkanSampleCount(query.sampleCount);

            passInfo2.subpassDescs.back().pNext = &msrtss;
        }*/

        RenderPassInfo renderPassInfo;
        renderPassInfo.mainSubpass = createInfo2.subpassDescs.size() - 1;
        renderPassInfo.uniqueId = nextRenderPassId++;
        DAWN_TRY(
            CheckVkSuccess(mDevice->fn.CreateRenderPass2KHR(mDevice->GetVkDevice(), &passInfo2.createInfo,
                                                            nullptr, &*renderPassInfo.renderPass),
                           "CreateRenderPass2KHR"));
        return renderPassInfo;
    }

    RenderPassInfo passInfo;
    InitializeRenderPassCreateInfo(mDevice, query, passInfo);

    // Create the render pass from the zillion parameters
    RenderPassInfo renderPassInfo;
    renderPassInfo.mainSubpass = passInfo.subpassDescs.size() - 1;
    renderPassInfo.uniqueId = nextRenderPassId++;
    DAWN_TRY(CheckVkSuccess(mDevice->fn.CreateRenderPass(mDevice->GetVkDevice(), &passInfo.createInfo,
                                                         nullptr, &*renderPassInfo.renderPass),
                            "CreateRenderPass"));
    return renderPassInfo;
}

// RenderPassCache

// If you change these, remember to also update StreamImplVk.cpp

size_t RenderPassCache::CacheFuncs::operator()(const RenderPassCacheQuery& query) const {
    size_t hash = Hash(query.colorMask);

    HashCombine(&hash, Hash(query.resolveTargetMask));

    for (auto i : query.colorMask) {
        HashCombine(&hash, query.colorFormats[i], query.colorLoadOp[i], query.colorStoreOp[i]);
    }
    HashCombine(&hash, query.expandResolveMask);

    HashCombine(&hash, query.hasDepthStencil);
    if (query.hasDepthStencil) {
        HashCombine(&hash, query.depthStencilFormat, query.depthLoadOp, query.depthStoreOp,
                    query.depthReadOnly, query.stencilLoadOp, query.stencilStoreOp,
                    query.stencilReadOnly);
    }
    HashCombine(&hash, query.sampleCount);

    return hash;
}

bool RenderPassCache::CacheFuncs::operator()(const RenderPassCacheQuery& a,
                                             const RenderPassCacheQuery& b) const {
    if (a.colorMask != b.colorMask) {
        return false;
    }

    if (a.resolveTargetMask != b.resolveTargetMask) {
        return false;
    }

    if (a.sampleCount != b.sampleCount) {
        return false;
    }

    for (auto i : a.colorMask) {
        if ((a.colorFormats[i] != b.colorFormats[i]) || (a.colorLoadOp[i] != b.colorLoadOp[i]) ||
            (a.colorStoreOp[i] != b.colorStoreOp[i])) {
            return false;
        }
    }

    if (a.expandResolveMask != b.expandResolveMask) {
        return false;
    }

    if (a.hasDepthStencil != b.hasDepthStencil) {
        return false;
    }

    if (a.hasDepthStencil) {
        if ((a.depthStencilFormat != b.depthStencilFormat) || (a.depthLoadOp != b.depthLoadOp) ||
            (a.stencilLoadOp != b.stencilLoadOp) || (a.depthStoreOp != b.depthStoreOp) ||
            (a.depthReadOnly != b.depthReadOnly) || (a.stencilStoreOp != b.stencilStoreOp) ||
            (a.stencilReadOnly != b.stencilReadOnly)) {
            return false;
        }
    }

    return true;
}
}  // namespace dawn::native::vulkan
