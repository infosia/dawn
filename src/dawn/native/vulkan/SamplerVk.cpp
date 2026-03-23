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

#include "dawn/native/vulkan/SamplerVk.h"

#include <algorithm>

#include "dawn/common/Log.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/FencedDeleter.h"
#include "dawn/native/vulkan/TextureVk.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"
#include "vulkan/vulkan_core.h"

namespace dawn::native::vulkan {

namespace {
VkSamplerAddressMode VulkanSamplerAddressMode(wgpu::AddressMode mode) {
    switch (mode) {
        case wgpu::AddressMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case wgpu::AddressMode::MirrorRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case wgpu::AddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case wgpu::AddressMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

VkSamplerMipmapMode VulkanMipMapMode(wgpu::MipmapFilterMode filter) {
    switch (filter) {
        case wgpu::MipmapFilterMode::Linear:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        case wgpu::MipmapFilterMode::Nearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case wgpu::MipmapFilterMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}
}  // anonymous namespace

// static
ResultOrError<Ref<Sampler>> Sampler::Create(Device* device, const SamplerDescriptor* descriptor) {
    Ref<Sampler> sampler = AcquireRef(new Sampler(device, descriptor));
    DAWN_TRY(sampler->Initialize(descriptor));
    return sampler;
}

MaybeError Sampler::Initialize(const SamplerDescriptor* descriptor) {
    DAWN_DEBUG() << "Sampler creation";

    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.magFilter = ToVulkanSamplerFilter(descriptor->magFilter);
    createInfo.minFilter = ToVulkanSamplerFilter(descriptor->minFilter);
    createInfo.mipmapMode = VulkanMipMapMode(descriptor->mipmapFilter);
    createInfo.addressModeU = VulkanSamplerAddressMode(descriptor->addressModeU);
    createInfo.addressModeV = VulkanSamplerAddressMode(descriptor->addressModeV);
    createInfo.addressModeW = VulkanSamplerAddressMode(descriptor->addressModeW);
    createInfo.mipLodBias = 0.0f;
    if (descriptor->compare != wgpu::CompareFunction::Undefined) {
        createInfo.compareOp = ToVulkanCompareOp(descriptor->compare);
        createInfo.compareEnable = VK_TRUE;
    } else {
        // Still set the compareOp so it's not garbage.
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.compareEnable = VK_FALSE;
    }
    createInfo.minLod = descriptor->lodMinClamp;
    createInfo.maxLod = descriptor->lodMaxClamp;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    Device* device = ToBackend(GetDevice());
    uint16_t maxAnisotropy = GetMaxAnisotropy();
    if (device->GetDeviceInfo().features.samplerAnisotropy == VK_TRUE && maxAnisotropy > 1) {
        createInfo.anisotropyEnable = VK_TRUE;
        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSamplerCreateInfo.html
        createInfo.maxAnisotropy =
            std::min(static_cast<float>(maxAnisotropy),
                     device->GetDeviceInfo().properties.limits.maxSamplerAnisotropy);
    } else {
        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.maxAnisotropy = 1;
    }

    DAWN_DEBUG() << "Sampler creation";
    DAWN_DEBUG() << "sType=" << createInfo.sType;
    DAWN_DEBUG() << "flags=" << createInfo.flags;
    DAWN_DEBUG() << "magFilter=" << createInfo.magFilter;
    DAWN_DEBUG() << "minFilter=" << createInfo.minFilter;
    DAWN_DEBUG() << "mipmapMode=" << createInfo.mipmapMode;
    DAWN_DEBUG() << "addressModeU=" << createInfo.addressModeU;
    DAWN_DEBUG() << "addressModeV=" << createInfo.addressModeV;
    DAWN_DEBUG() << "addressModeW=" << createInfo.addressModeW;
    DAWN_DEBUG() << "mipLodBias=" << createInfo.mipLodBias;
    DAWN_DEBUG() << "compareOp=" << createInfo.compareOp;
    DAWN_DEBUG() << "compareEnable=" << createInfo.compareEnable;
    DAWN_DEBUG() << "minLod=" << createInfo.minLod;
    DAWN_DEBUG() << "maxLod=" << createInfo.maxLod;
    DAWN_DEBUG() << "unnormalizedCoordinates=" << createInfo.unnormalizedCoordinates;
    DAWN_DEBUG() << "anisotropyEnable=" << createInfo.anisotropyEnable;
    DAWN_DEBUG() << "maxAnisotropy=" << createInfo.maxAnisotropy;

    VkSamplerYcbcrConversionInfo samplerYCbCrInfo = {};
    if (IsYCbCr()) {
        DAWN_TRY_ASSIGN(mSamplerYCbCrConversion,
                        CreateSamplerYCbCrConversionCreateInfo(GetYCbCrVkDescriptor(), device));

        samplerYCbCrInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
        samplerYCbCrInfo.pNext = nullptr;
        samplerYCbCrInfo.conversion = mSamplerYCbCrConversion;

        DAWN_DEBUG() << "samplerYCbCrInfo.sType=" << samplerYCbCrInfo.sType;
        DAWN_DEBUG() << "samplerYCbCrInfo.pNext=" << samplerYCbCrInfo.pNext;
        DAWN_DEBUG() << "samplerYCbCrInfo.conversion=" << samplerYCbCrInfo.conversion;
        createInfo.pNext = &samplerYCbCrInfo;
    }

    DAWN_DEBUG() << "pNext=" << createInfo.pNext;
    DAWN_TRY(CheckVkSuccess(
        device->fn.CreateSampler(device->GetVkDevice(), &createInfo, nullptr, &*mHandle),
        "CreateSampler"));

    DAWN_DEBUG() << "Made sampler " << *mHandle;

    SetLabelImpl();

    return {};
}

Sampler::~Sampler() = default;

void Sampler::DestroyImpl(DestroyReason reason) {
    SamplerBase::DestroyImpl(reason);
    DAWN_DEBUG() << "*88888888888*******88***888*8*8*8*8*8*8******888********888*****";
    if (mSamplerYCbCrConversion != VK_NULL_HANDLE) {
        ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mSamplerYCbCrConversion);
        mSamplerYCbCrConversion = VK_NULL_HANDLE;
    }
    if (mHandle != VK_NULL_HANDLE) {
        ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mHandle);
        mHandle = VK_NULL_HANDLE;
    }
}

const VkSampler& Sampler::GetHandle() const {
    return mHandle;
}

void Sampler::SetLabelImpl() {
    SetDebugName(ToBackend(GetDevice()), mHandle, "Dawn_Sampler", GetLabel());
}

// static
StaticSamplerSpecialization StaticSamplerSpecialization::From(const TextureView* view,
                                                              const Sampler* sampler) {
    bool isYCbCr =
        view->GetTexture()->GetFormat().format == wgpu::TextureFormat::OpaqueYCbCrAndroid;
    bool filterable = !isYCbCr || view->IsYCbCrFilterable();

    StaticSamplerSpecialization spec = {
        .minFilter = (sampler != nullptr && filterable) ? sampler->GetMinFilter()
                                                        : wgpu::FilterMode::Nearest,
        .magFilter = (sampler != nullptr && filterable) ? sampler->GetMagFilter()
                                                        : wgpu::FilterMode::Nearest,
        .isYCbCr = isYCbCr,
        // Put default values for the YCbCr part.
        .vkFormat = VK_FORMAT_UNDEFINED,
        .androidExternalFormat = 0,
        .model = VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY,
        .range = VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
    };

    if (isYCbCr) {
        auto stm = static_cast<SharedTextureMemoryContentsVk*>(
            view->GetTexture()->GetSharedResourceMemoryContents());
        DAWN_ASSERT(stm != nullptr);

        spec.vkFormat = static_cast<VkFormat>(stm->GetYCbCrVkDesc().vkFormat);
        spec.androidExternalFormat = stm->GetYCbCrVkDesc().externalFormat;
        spec.model = VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
        spec.range = VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
        spec.xChromaOffset = static_cast<VkChromaLocation>(stm->GetYCbCrVkDesc().vkXChromaOffset);
        spec.yChromaOffset = static_cast<VkChromaLocation>(stm->GetYCbCrVkDesc().vkYChromaOffset);
        spec.forceReconstruction = stm->GetYCbCrVkDesc().forceExplicitReconstruction;
    }

    return spec;
}

ResultOrError<Ref<SamplerBase>> StaticSamplerSpecialization::CreateSampler(
    DeviceBase* device) const {
    SamplerDescriptor samplerDesc;
    samplerDesc.magFilter = this->magFilter;
    samplerDesc.minFilter = this->minFilter;

    YCbCrVkDescriptor yCbCrDesc;
    if (isYCbCr) {
        yCbCrDesc.vkFormat = this->vkFormat;
        yCbCrDesc.externalFormat = this->androidExternalFormat;
        yCbCrDesc.vkYCbCrModel = this->model;
        yCbCrDesc.vkYCbCrRange = this->range;
        yCbCrDesc.vkComponentSwizzleRed = VK_COMPONENT_SWIZZLE_R;
        yCbCrDesc.vkComponentSwizzleGreen = VK_COMPONENT_SWIZZLE_G;
        yCbCrDesc.vkComponentSwizzleBlue = VK_COMPONENT_SWIZZLE_B;
        yCbCrDesc.vkComponentSwizzleAlpha = VK_COMPONENT_SWIZZLE_A;
        yCbCrDesc.vkXChromaOffset = this->xChromaOffset;
        yCbCrDesc.vkYChromaOffset = this->yChromaOffset;
        yCbCrDesc.vkChromaFilter = wgpu::FilterMode::Nearest;
        yCbCrDesc.forceExplicitReconstruction = this->forceReconstruction;

        samplerDesc.nextInChain = &yCbCrDesc;
    }

    return device->CreateSampler(&samplerDesc);
}

}  // namespace dawn::native::vulkan
