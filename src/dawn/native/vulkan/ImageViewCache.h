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

#ifndef SRC_DAWN_NATIVE_VULKAN_IMAGEVIEWCACHE_H_
#define SRC_DAWN_NATIVE_VULKAN_IMAGEVIEWCACHE_H_

#include <array>
#include <list>
#include <mutex>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "dawn/common/Constants.h"
#include "dawn/common/vulkan_platform.h"
#include "dawn/native/Error.h"
#include "dawn/native/Texture.h"
#include "dawn/native/dawn_platform.h"
#include "dawn/native/vulkan/RefCountedVkHandle.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native::vulkan {

class Device;
class Texture;

// This is a key to query the ImageViewCache.
struct ImageViewCacheQuery {
    wgpu::TextureFormat format;
    wgpu::TextureViewDimension dimension;
    VkImageUsageFlags usage;
    uint32_t depthSlice = 0u;
    SubresourceRange subresources;
    bool isYCbCr = false;
    VkSamplerYcbcrConversion samplerYCbCrConversion;
};

// A LRU Cache of VkImageViews. The cache is only valid for ImageViews from a single VkImage.
// All the operations on ImageViewCache are guaranteed to be thread-safe.
class ImageViewCache {
    // How many elements the LRU cache will retain before it begins evicting them.
    // Uses a fairly low capacity because it's expected that only a few views will be re-created
    // multiple times.
    static const size_t kDefaultCacheCapacity = 4;

  public:
    using ImageViewRef = Ref<RefCountedVkHandle<VkImageView>>;

    explicit ImageViewCache(Texture* texture, size_t capacity = kDefaultCacheCapacity);
    ~ImageViewCache();

    ResultOrError<ImageViewRef> GetImageView(ImageViewCacheQuery& query);

    VkImageViewCreateInfo GetCreateInfo(ImageViewCacheQuery& query) const;

  private:
    // Does the actual VkImageVIew creation on a cache miss.
    ResultOrError<ImageViewRef> CreateImageViewForQuery(ImageViewCacheQuery& query) const;

    // Implements the functors necessary for to use ImageViewCacheQuery as absl::flat_hash_map
    // keys.
    struct CacheFuncs {
        size_t operator()(const ImageViewCacheQuery& query) const;
        bool operator()(const ImageViewCacheQuery& a, const ImageViewCacheQuery& b) const;
    };

    using RecentList = std::list<std::pair<ImageViewCacheQuery, ImageViewRef>>;
    using Cache =
        absl::flat_hash_map<ImageViewCacheQuery, RecentList::iterator, CacheFuncs, CacheFuncs>;

    raw_ptr<Texture> mTexture = nullptr;
    size_t mCapacity;

    std::mutex mMutex;
    RecentList mRecentList;
    Cache mCache;
};

}  // namespace dawn::native::vulkan

#endif  // SRC_DAWN_NATIVE_VULKAN_IMAGEVIEWCACHE_H_
