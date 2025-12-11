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

#include "absl/container/inlined_vector.h"
#include "dawn/common/Enumerator.h"
#include "dawn/common/HashUtils.h"
#include "dawn/common/Log.h"
#include "dawn/common/Range.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/TextureVk.h"
#include "dawn/native/vulkan/VulkanError.h"

namespace dawn::native::vulkan {

namespace {
VkAttachmentLoadOp VulkanAttachmentLoadOp(wgpu::LoadOp op) {
    switch (op) {
        case wgpu::LoadOp::Load:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case wgpu::LoadOp::Clear:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case wgpu::LoadOp::ExpandResolveTexture:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case wgpu::LoadOp::Undefined:
            DAWN_UNREACHABLE();
            break;
    }
    DAWN_UNREACHABLE();
}

VkAttachmentStoreOp VulkanAttachmentStoreOp(wgpu::StoreOp op) {
    // TODO(crbug.com/dawn/485): return STORE_OP_STORE_NONE_QCOM if the device has required
    // extension.
    switch (op) {
        case wgpu::StoreOp::Store:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case wgpu::StoreOp::Discard:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case wgpu::StoreOp::Undefined:
            DAWN_UNREACHABLE();
            break;
    }
    DAWN_UNREACHABLE();
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
    // The Vulkan subpasses want to know the layout of the attachments with VkAttachmentRef.
    // Precompute them as they must be pointer-chained in VkSubpassDescription.
    // Note that both colorAttachmentRefs and resolveAttachmentRefs can be sparse with holes
    // filled with VK_ATTACHMENT_UNUSED.
    PerColorAttachment<VkAttachmentReference> colorAttachmentRefs;
    PerColorAttachment<VkAttachmentReference> resolveAttachmentRefs;
    PerColorAttachment<VkAttachmentReference> inputAttachmentRefs;
    VkAttachmentReference depthStencilAttachmentRef;

    for (auto i : Range(kMaxColorAttachmentsTyped)) {
        colorAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        resolveAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        inputAttachmentRefs[i].attachment = VK_ATTACHMENT_UNUSED;
        // The Khronos Vulkan validation layer will complain if not set
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        resolveAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        inputAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Contains the attachment description that will be chained in the create info
    // The order of all attachments in attachmentDescs is "color-depthstencil-resolve".
    constexpr uint8_t kMaxAttachmentCount = kMaxColorAttachments * 2 + 1;
    std::array<VkAttachmentDescription, kMaxAttachmentCount> attachmentDescs = {};

    VkSampleCountFlagBits vkSampleCount =
        query.renderToSingleSampled ? VK_SAMPLE_COUNT_1_BIT : VulkanSampleCount(query.sampleCount);

    uint32_t attachmentCount = 0;
    ColorAttachmentIndex highestColorAttachmentIndexPlusOne(static_cast<uint8_t>(0));
    for (auto i : query.colorMask) {
        auto& attachmentRef = colorAttachmentRefs[i];
        auto& attachmentDesc = attachmentDescs[attachmentCount];

        attachmentRef.attachment = attachmentCount;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentDesc.flags = 0;
        attachmentDesc.format = VulkanImageFormat(mDevice, query.colorFormats[i]);
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
        const Format& dsFormat = mDevice->GetValidInternalFormat(query.depthStencilFormat);

        depthStencilAttachment = &depthStencilAttachmentRef;
        depthStencilAttachmentRef.attachment = attachmentCount;
        depthStencilAttachmentRef.layout = VulkanImageLayoutForDepthStencilAttachment(
            dsFormat, query.depthReadOnly, query.stencilReadOnly);

        // Build the attachment descriptor.
        auto& attachmentDesc = attachmentDescs[attachmentCount];
        attachmentDesc.flags = 0;
        attachmentDesc.format = VulkanImageFormat(mDevice, dsFormat.format);
        attachmentDesc.samples = vkSampleCount;

        attachmentDesc.loadOp = VulkanAttachmentLoadOp(query.depthLoadOp);
        attachmentDesc.storeOp = VulkanAttachmentStoreOp(query.depthStoreOp);
        attachmentDesc.stencilLoadOp = VulkanAttachmentLoadOp(query.stencilLoadOp);
        attachmentDesc.stencilStoreOp = VulkanAttachmentStoreOp(query.stencilStoreOp);

        // There is only one subpass, so it is safe to set both initialLayout and finalLayout to
        // the only subpass's layout.
        attachmentDesc.initialLayout = depthStencilAttachmentRef.layout;
        attachmentDesc.finalLayout = depthStencilAttachmentRef.layout;

        attachmentCount++;
    }

    uint32_t resolveAttachmentCount = 0;
    ColorAttachmentIndex highestInputAttachmentIndex(static_cast<uint8_t>(0));

    for (auto i : query.resolveTargetMask) {
        auto& resolveAttachmentRef = resolveAttachmentRefs[i];
        auto& resolveAttachmentDesc = attachmentDescs[attachmentCount];

        resolveAttachmentRef.attachment = attachmentCount;
        resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        resolveAttachmentDesc.flags = 0;
        resolveAttachmentDesc.format = VulkanImageFormat(mDevice, query.colorFormats[i]);
        resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (query.expandResolveMask.test(i)) {
            resolveAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            resolveAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            inputAttachmentRefs[i].attachment = resolveAttachmentRefs[i].attachment;

            highestInputAttachmentIndex = i;
        } else {
            resolveAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        resolveAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentCount++;
        resolveAttachmentCount++;
    }

    absl::InlinedVector<VkSubpassDescription, 2> subpassDescs;
    absl::InlinedVector<VkSubpassDependency, 2> subpassDependencies;
    if (query.expandResolveMask.any()) {
        // To simulate ExpandResolveTexture, we use two subpasses. The first subpass will read the
        // resolve texture as input attachment.
        subpassDescs.push_back({});
        VkSubpassDescription& subpassDesc = subpassDescs.back();
        subpassDesc.flags = 0;
        subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDesc.inputAttachmentCount = static_cast<uint8_t>(highestInputAttachmentIndex) + 1;
        subpassDesc.pInputAttachments = inputAttachmentRefs.data();
        subpassDesc.colorAttachmentCount = static_cast<uint8_t>(highestColorAttachmentIndexPlusOne);
        subpassDesc.pColorAttachments = colorAttachmentRefs.data();
        subpassDesc.pDepthStencilAttachment = depthStencilAttachment;

        InitializeLoadResolveSubpassDependencies(&subpassDependencies);
    }

    // Create the VkSubpassDescription that will be chained in the VkRenderPassCreateInfo
    subpassDescs.push_back({});
    VkSubpassDescription& subpassDesc = subpassDescs.back();
    subpassDesc.flags = 0;
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = nullptr;
    subpassDesc.colorAttachmentCount = static_cast<uint8_t>(highestColorAttachmentIndexPlusOne);
    subpassDesc.pColorAttachments = colorAttachmentRefs.data();

    // Qualcomm GPUs have a driver bug on some devices where passing a zero-length array to the
    // resolveAttachments causes a VK_ERROR_OUT_OF_HOST_MEMORY. nullptr must be passed instead.
    if (resolveAttachmentCount) {
        subpassDesc.pResolveAttachments = resolveAttachmentRefs.data();
    } else {
        subpassDesc.pResolveAttachments = nullptr;
    }

    subpassDesc.pDepthStencilAttachment = depthStencilAttachment;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = nullptr;

    // Chain everything in VkRenderPassCreateInfo
    VkRenderPassCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments = attachmentDescs.data();
    createInfo.subpassCount = subpassDescs.size();
    createInfo.pSubpasses = subpassDescs.data();
    createInfo.dependencyCount = subpassDependencies.size();
    createInfo.pDependencies = subpassDependencies.data();

    if (query.renderToSingleSampled) {
        dawn::ErrorLog() << "renderToSingleSampled";
        absl::InlinedVector<VkAttachmentDescription2, kMaxAttachmentCount> attachmentDescs2;
        attachmentDescs2.reserve(attachmentCount);
        for (uint32_t i = 0; i < attachmentCount; ++i) {
            const auto& desc = attachmentDescs[i];
            VkAttachmentDescription2 desc2 = {};
            desc2.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
            desc2.flags = desc.flags;
            desc2.format = desc.format;
            desc2.samples = desc.samples;
            desc2.loadOp = desc.loadOp;
            desc2.storeOp = desc.storeOp;
            desc2.stencilLoadOp = desc.stencilLoadOp;
            desc2.stencilStoreOp = desc.stencilStoreOp;
            desc2.initialLayout = desc.initialLayout;
            desc2.finalLayout = desc.finalLayout;
            attachmentDescs2.push_back(desc2);
        }

        // We need to persist the vector of references until CreateRenderPass2 call.
        std::vector<std::vector<VkAttachmentReference2>> inputAttachments2(subpassDescs.size());
        std::vector<std::vector<VkAttachmentReference2>> colorAttachments2(subpassDescs.size());
        std::vector<std::vector<VkAttachmentReference2>> resolveAttachments2(subpassDescs.size());
        std::vector<VkAttachmentReference2> depthStencilAttachments2(subpassDescs.size());

        absl::InlinedVector<VkSubpassDescription2, 2> subpassDescs2;
        subpassDescs2.reserve(subpassDescs.size());

        for (size_t i = 0; i < subpassDescs.size(); ++i) {
            const auto& desc = subpassDescs[i];
            VkSubpassDescription2 desc2 = {};
            desc2.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
            desc2.flags = desc.flags;
            desc2.pipelineBindPoint = desc.pipelineBindPoint;
            desc2.viewMask = 0;

            if (desc.inputAttachmentCount > 0) {
                inputAttachments2[i].resize(desc.inputAttachmentCount);
                for (uint32_t j = 0; j < desc.inputAttachmentCount; ++j) {
                    inputAttachments2[i][j].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    inputAttachments2[i][j].attachment = desc.pInputAttachments[j].attachment;
                    inputAttachments2[i][j].layout = desc.pInputAttachments[j].layout;
                }
                desc2.inputAttachmentCount = desc.inputAttachmentCount;
                desc2.pInputAttachments = inputAttachments2[i].data();
            }

            if (desc.colorAttachmentCount > 0) {
                colorAttachments2[i].resize(desc.colorAttachmentCount);
                for (uint32_t j = 0; j < desc.colorAttachmentCount; ++j) {
                    colorAttachments2[i][j].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    colorAttachments2[i][j].attachment = desc.pColorAttachments[j].attachment;
                    colorAttachments2[i][j].layout = desc.pColorAttachments[j].layout;
                }
                desc2.colorAttachmentCount = desc.colorAttachmentCount;
                desc2.pColorAttachments = colorAttachments2[i].data();
            }

            if (desc.pResolveAttachments) {
                resolveAttachments2[i].resize(desc.colorAttachmentCount);
                for (uint32_t j = 0; j < desc.colorAttachmentCount; ++j) {
                    resolveAttachments2[i][j].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                    resolveAttachments2[i][j].attachment = desc.pResolveAttachments[j].attachment;
                    resolveAttachments2[i][j].layout = desc.pResolveAttachments[j].layout;
                }
                desc2.pResolveAttachments = resolveAttachments2[i].data();
            }

            if (desc.pDepthStencilAttachment) {
                depthStencilAttachments2[i].sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
                depthStencilAttachments2[i].attachment = desc.pDepthStencilAttachment->attachment;
                depthStencilAttachments2[i].layout = desc.pDepthStencilAttachment->layout;
                desc2.pDepthStencilAttachment = &depthStencilAttachments2[i];
            }

            subpassDescs2.push_back(desc2);
        }

        absl::InlinedVector<VkSubpassDependency2, 2> subpassDependencies2;
        subpassDependencies2.reserve(subpassDependencies.size());
        for (const auto& dep : subpassDependencies) {
            VkSubpassDependency2 dep2 = {};
            dep2.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
            dep2.srcSubpass = dep.srcSubpass;
            dep2.dstSubpass = dep.dstSubpass;
            dep2.srcStageMask = dep.srcStageMask;
            dep2.dstStageMask = dep.dstStageMask;
            dep2.srcAccessMask = dep.srcAccessMask;
            dep2.dstAccessMask = dep.dstAccessMask;
            dep2.dependencyFlags = dep.dependencyFlags;
            subpassDependencies2.push_back(dep2);
        }

        VkMultisampledRenderToSingleSampledInfoEXT msrtss = {};
        msrtss.sType = VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT;
        msrtss.multisampledRenderToSingleSampledEnable = VK_TRUE;
        msrtss.rasterizationSamples = VulkanSampleCount(query.sampleCount);

        subpassDescs2.back().pNext = &msrtss;

        VkRenderPassCreateInfo2 createInfo2 = {};
        createInfo2.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
        createInfo2.flags = createInfo.flags;
        createInfo2.attachmentCount = attachmentCount;
        createInfo2.pAttachments = attachmentDescs2.data();
        createInfo2.subpassCount = subpassDescs2.size();
        createInfo2.pSubpasses = subpassDescs2.data();
        createInfo2.dependencyCount = subpassDependencies2.size();
        createInfo2.pDependencies = subpassDependencies2.data();

        RenderPassInfo renderPassInfo;
        renderPassInfo.mainSubpass = subpassDescs2.size() - 1;
        renderPassInfo.uniqueId = nextRenderPassId++;
        DAWN_TRY(CheckVkSuccess(mDevice->fn.CreateRenderPass2(mDevice->GetVkDevice(), &createInfo2,
                                                              nullptr, &*renderPassInfo.renderPass),
                                "CreateRenderPass2"));
        return renderPassInfo;
    }

    // Create the render pass from the zillion parameters
    RenderPassInfo renderPassInfo;
    renderPassInfo.mainSubpass = subpassDescs.size() - 1;
    renderPassInfo.uniqueId = nextRenderPassId++;
    DAWN_TRY(CheckVkSuccess(mDevice->fn.CreateRenderPass(mDevice->GetVkDevice(), &createInfo,
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
    HashCombine(&hash, query.sampleCount, query.renderToSingleSampled);

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

    if (a.renderToSingleSampled != b.renderToSingleSampled) {
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
