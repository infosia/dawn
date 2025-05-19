// Copyright 2025 The Dawn & Tint Authors
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

#include "dawn/native/vulkan/ImageViewCache.h"

#include "absl/container/inlined_vector.h"
#include "dawn/common/HashUtils.h"
#include "dawn/common/Range.h"
#include "dawn/common/vulkan_platform.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/TextureVk.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"

namespace dawn::native::vulkan {

namespace {
// Converts an Dawn texture dimension to a Vulkan image view type.
// Contrary to image types, image view types include arrayness and cubemapness
VkImageViewType VulkanImageViewType(wgpu::TextureViewDimension dimension) {
    switch (dimension) {
        case wgpu::TextureViewDimension::e1D:
            return VK_IMAGE_VIEW_TYPE_1D;
        case wgpu::TextureViewDimension::e2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case wgpu::TextureViewDimension::e2DArray:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case wgpu::TextureViewDimension::Cube:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case wgpu::TextureViewDimension::CubeArray:
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        case wgpu::TextureViewDimension::e3D:
            return VK_IMAGE_VIEW_TYPE_3D;

        case wgpu::TextureViewDimension::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

}  // namespace

// ImageViewCache

ImageViewCache::ImageViewCache(Texture* texture, size_t capacity)
    : mTexture(texture), mCapacity(capacity) {}

void ImageViewCache::Clear() {
    std::lock_guard<std::mutex> lock(mMutex);
    mRecentList.clear();
    mCache.clear();
}

ResultOrError<ImageViewCache::ImageViewRef> ImageViewCache::GetImageView(
    ImageViewCacheQuery& query) {
    ImageViewRef imageView;

    // Don't cache YCbCr views for now.
    if (query.isYCbCr) {
        DAWN_TRY_ASSIGN(imageView, CreateImageViewForQuery(query));
        return imageView;
    }

    // For everything else, attempt to find a cached ImageView first.
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mCache.find(query);
    if (it != mCache.end()) {
        // Move the queried ImageView to the front of the LRU list.
        // Using the iterator as a stable reference like this works because "Adding, removing and
        // moving the elements within the list or across several lists does not invalidate the
        // iterators or references. An iterator is invalidated only when the corresponding element
        // is deleted." (From https://en.cppreference.com/w/cpp/container/list)
        mRecentList.splice(mRecentList.begin(), mRecentList, it->second);
        return it->second->second;
    }

    // If an ImageView wasn't found in the LRU cache, create a new one and insert it into the cache.
    DAWN_TRY_ASSIGN(imageView, CreateImageViewForQuery(query));

    mRecentList.emplace_front(query, imageView);
    mCache.emplace(query, mRecentList.begin());

    // If the LRU cache has exceeded it's capacity, remove the oldest entry. If no TextureViews
    // still have a reference to it this will cause it to be deleted via DeleteWhenUnused.
    if (mRecentList.size() > mCapacity) {
        auto back = mRecentList.back();
        mCache.erase(back.first);
        mRecentList.pop_back();
    }

    return imageView;
}

ResultOrError<ImageViewCache::ImageViewRef> ImageViewCache::CreateImageViewForQuery(
    ImageViewCacheQuery& query) const {
    Device* device = ToBackend(mTexture->GetDevice());

    VkImageViewCreateInfo createInfo = {};
    PNextChainBuilder createInfoChain(&createInfo);
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.image = mTexture->GetHandle();
    createInfo.viewType = VulkanImageViewType(query.dimension);

    const Format& textureFormat = mTexture->GetFormat();
    if (textureFormat.HasStencil() &&
        (textureFormat.HasDepth() || !device->IsToggleEnabled(Toggle::VulkanUseS8))) {
        // Unlike multi-planar formats, depth-stencil formats have multiple aspects but are not
        // created with VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT.
        // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html#VUID-VkImageViewCreateInfo-image-01762
        // Without, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, the view format must match the texture
        // format.
        createInfo.format = VulkanImageFormat(device, textureFormat.format);
    } else {
        createInfo.format = VulkanImageFormat(device, query.format);
    }

    createInfo.components = VkComponentMapping{VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                               VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

    createInfo.subresourceRange.baseMipLevel = query.subresources.baseMipLevel;
    createInfo.subresourceRange.levelCount = query.subresources.levelCount;
    createInfo.subresourceRange.baseArrayLayer =
        query.subresources.baseArrayLayer + query.depthSlice;
    createInfo.subresourceRange.layerCount = query.subresources.layerCount;
    createInfo.subresourceRange.aspectMask = VulkanAspectMask(query.subresources.aspects);

    VkImageViewUsageCreateInfo usageInfo = {};
    createInfoChain.Add(&usageInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO);
    usageInfo.usage = query.usage;

    VkSamplerYcbcrConversionInfo samplerYCbCrInfo = {};
    if (query.isYCbCr) {
        createInfoChain.Add(&samplerYCbCrInfo, VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO);
        samplerYCbCrInfo.pNext = nullptr;
        samplerYCbCrInfo.conversion = query.samplerYCbCrConversion;
    }

    VkImageView imageView;
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreateImageView(device->GetVkDevice(), &createInfo, nullptr, &*imageView),
        "CreateImageView"));

    return AcquireRef(new RefCountedVkHandle<VkImageView>(device, imageView));
}

size_t ImageViewCache::CacheFuncs::operator()(const ImageViewCacheQuery& query) const {
    size_t hash = Hash(query.format);

    HashCombine(&hash, query.dimension, query.usage, query.depthSlice);
    HashCombine(&hash, query.subresources.baseMipLevel, query.subresources.levelCount,
                query.subresources.baseArrayLayer, query.subresources.layerCount);

    return hash;
}

bool ImageViewCache::CacheFuncs::operator()(const ImageViewCacheQuery& a,
                                            const ImageViewCacheQuery& b) const {
    return a == b;
}

}  // namespace dawn::native::vulkan
