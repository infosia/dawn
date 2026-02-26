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

#include <android/hardware_buffer.h>
#include <webgpu/webgpu_cpp.h>

#include <span>
#include <utility>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/Range.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

AHardwareBuffer* MakeYUVAHB(uint32_t width,
                            uint32_t height,
                            std::span<const uint8_t> yData,
                            std::span<const uint8_t> uData,
                            std::span<const uint8_t> vData) {
    DAWN_ASSERT(width % 2 == 0 && height % 2 == 0);

    AHardwareBuffer_Desc ahbDesc = {
        .width = width,
        .height = height,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420,
        .usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY | AHARDWAREBUFFER_USAGE_CPU_READ_NEVER |
                 AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
    };

    AHardwareBuffer* ahb = nullptr;
    EXPECT_EQ(AHardwareBuffer_allocate(&ahbDesc, &ahb), 0);

    AHardwareBuffer_Planes ahbPlanes;
    EXPECT_EQ(AHardwareBuffer_lockPlanes(ahb, AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY, -1, nullptr,
                                         &ahbPlanes),
              0);

    auto CopyPlane = [](std::span<const uint8_t> src, uint32_t srcWidth, uint32_t srcHeight,
                        const AHardwareBuffer_Plane& dst) {
        uint8_t* dstData = static_cast<uint8_t*>(dst.data);
        for (uint32_t x : Range(srcWidth)) {
            for (uint32_t y : Range(srcHeight)) {
                dstData[y * dst.rowStride + x * dst.pixelStride] = src[y * srcWidth + x];
            }
        }
    };

    CopyPlane(yData, width, height, ahbPlanes.planes[0]);
    CopyPlane(uData, width / 2, height / 2, ahbPlanes.planes[0]);
    CopyPlane(vData, width / 2, height / 2, ahbPlanes.planes[0]);

    EXPECT_EQ(AHardwareBuffer_unlock(ahb, nullptr), 0);

    return ahb;
}

class SharedTextureMemoryYCbCrVulkanSamplersTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::SharedTextureMemoryAHardwareBuffer,
                wgpu::FeatureName::SharedFenceSyncFD, wgpu::FeatureName::StaticSamplers, wgpu::FeatureName::YCbCrVulkanSamplers};
    }
};

// Test validation of an incorrectly-configured SharedTextureMemoryAHardwareBufferProperties
// instance.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests,
       InvalidSharedTextureMemoryAHardwareBufferProperties) {
    // TODO(crbug.com/40238674): Fails on Pixel 10 vulkan.
    DAWN_SUPPRESS_TEST_IF(IsImgTec() && IsVulkan());
    // TODO(crbug.com/444741058): Fails on Intel-based brya devices running Android Desktop.
    DAWN_SUPPRESS_TEST_IF(IsVulkan() && IsIntel() && IsAndroid());

    AHardwareBuffer_Desc aHardwareBufferDesc = {
        .width = 4,
        .height = 4,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
    };
    AHardwareBuffer* aHardwareBuffer;
    EXPECT_EQ(AHardwareBuffer_allocate(&aHardwareBufferDesc, &aHardwareBuffer), 0);

    wgpu::SharedTextureMemoryAHardwareBufferDescriptor stmAHardwareBufferDesc;
    stmAHardwareBufferDesc.handle = aHardwareBuffer;

    wgpu::SharedTextureMemoryDescriptor desc;
    desc.nextInChain = &stmAHardwareBufferDesc;

    wgpu::SharedTextureMemory memory = device.ImportSharedTextureMemory(&desc);

    wgpu::SharedTextureMemoryProperties properties;
    wgpu::SharedTextureMemoryAHardwareBufferProperties ahbProperties = {};
    wgpu::YCbCrVkDescriptor yCbCrDesc;

    // Chaining anything onto the passed-in YCbCrVkDescriptor is invalid.
    yCbCrDesc.nextInChain = &stmAHardwareBufferDesc;
    ahbProperties.yCbCrInfo = yCbCrDesc;
    properties.nextInChain = &ahbProperties;

    ASSERT_DEVICE_ERROR(memory.GetProperties(&properties));
}

// Test querying YCbCr info from the Device.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests, QueryYCbCrInfoFromDevice) {
    // TODO(crbug.com/40238674): Fails on Pixel 10 vulkan.
    DAWN_SUPPRESS_TEST_IF(IsImgTec() && IsVulkan());

    // TODO(crbug.com/444741058): Fails on Intel-based brya devices running Android Desktop.
    DAWN_SUPPRESS_TEST_IF(IsVulkan() && IsIntel() && IsAndroid());

    AHardwareBuffer_Desc aHardwareBufferDesc = {
        .width = 4,
        .height = 4,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
    };
    AHardwareBuffer* aHardwareBuffer;
    EXPECT_EQ(AHardwareBuffer_allocate(&aHardwareBufferDesc, &aHardwareBuffer), 0);

    // Query the YCbCr properties of the AHardwareBuffer.
    auto deviceVk = native::vulkan::ToBackend(native::FromAPI(device.Get()));

    VkAndroidHardwareBufferPropertiesANDROID bufferProperties = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
    };

    VkAndroidHardwareBufferFormatPropertiesANDROID bufferFormatProperties;
    native::vulkan::PNextChainBuilder bufferPropertiesChain(&bufferProperties);
    bufferPropertiesChain.Add(&bufferFormatProperties,
                              VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID);

    VkDevice vkDevice = deviceVk->GetVkDevice();
    EXPECT_EQ(deviceVk->fn.GetAndroidHardwareBufferPropertiesANDROID(vkDevice, aHardwareBuffer,
                                                                     &bufferProperties),
              VK_SUCCESS);

    // Query the YCbCr properties of this AHB via the Device.
    wgpu::AHardwareBufferProperties ahbProperties;
    device.GetAHardwareBufferProperties(aHardwareBuffer, &ahbProperties);
    auto yCbCrInfo = ahbProperties.yCbCrInfo;
    uint32_t formatFeatures = bufferFormatProperties.formatFeatures;

    // Verify that the YCbCr properties match.
    EXPECT_EQ(bufferFormatProperties.format, yCbCrInfo.vkFormat);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrModel, yCbCrInfo.vkYCbCrModel);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrRange, yCbCrInfo.vkYCbCrRange);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.r,
              yCbCrInfo.vkComponentSwizzleRed);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.g,
              yCbCrInfo.vkComponentSwizzleGreen);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.b,
              yCbCrInfo.vkComponentSwizzleBlue);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.a,
              yCbCrInfo.vkComponentSwizzleAlpha);
    EXPECT_EQ(bufferFormatProperties.suggestedXChromaOffset, yCbCrInfo.vkXChromaOffset);
    EXPECT_EQ(bufferFormatProperties.suggestedYChromaOffset, yCbCrInfo.vkYChromaOffset);

    wgpu::FilterMode expectedFilter =
        (formatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT)
            ? wgpu::FilterMode::Linear
            : wgpu::FilterMode::Nearest;
    EXPECT_EQ(expectedFilter, yCbCrInfo.vkChromaFilter);
    EXPECT_EQ(
        formatFeatures &
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT,
        yCbCrInfo.forceExplicitReconstruction);
    EXPECT_EQ(bufferFormatProperties.externalFormat, yCbCrInfo.externalFormat);
}

// Test querying YCbCr info from the SharedTextureMemory without external format.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests, QueryYCbCrInfoWithoutYCbCrFormat) {
    // TODO(crbug.com/40238674): Fails on Pixel 10 vulkan.
    DAWN_SUPPRESS_TEST_IF(IsImgTec() && IsVulkan());

    // TODO(crbug.com/444741058): Fails on Intel-based brya devices running Android Desktop.
    DAWN_SUPPRESS_TEST_IF(IsVulkan() && IsIntel() && IsAndroid());

    AHardwareBuffer_Desc aHardwareBufferDesc = {
        .width = 4,
        .height = 4,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
    };
    AHardwareBuffer* aHardwareBuffer;
    EXPECT_EQ(AHardwareBuffer_allocate(&aHardwareBufferDesc, &aHardwareBuffer), 0);

    // Query the YCbCr properties of the AHardwareBuffer.
    auto deviceVk = native::vulkan::ToBackend(native::FromAPI(device.Get()));

    VkAndroidHardwareBufferPropertiesANDROID bufferProperties = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
    };

    VkAndroidHardwareBufferFormatPropertiesANDROID bufferFormatProperties;
    native::vulkan::PNextChainBuilder bufferPropertiesChain(&bufferProperties);
    bufferPropertiesChain.Add(&bufferFormatProperties,
                              VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID);

    VkDevice vkDevice = deviceVk->GetVkDevice();
    EXPECT_EQ(deviceVk->fn.GetAndroidHardwareBufferPropertiesANDROID(vkDevice, aHardwareBuffer,
                                                                     &bufferProperties),
              VK_SUCCESS);

    // Query the YCbCr properties of a SharedTextureMemory created from this
    // AHB.
    wgpu::SharedTextureMemoryAHardwareBufferDescriptor stmAHardwareBufferDesc;
    stmAHardwareBufferDesc.handle = aHardwareBuffer;

    wgpu::SharedTextureMemoryDescriptor desc;
    desc.nextInChain = &stmAHardwareBufferDesc;

    wgpu::SharedTextureMemory memory = device.ImportSharedTextureMemory(&desc);

    wgpu::SharedTextureMemoryProperties properties;
    wgpu::SharedTextureMemoryAHardwareBufferProperties ahbProperties = {};
    properties.nextInChain = &ahbProperties;
    memory.GetProperties(&properties);
    auto yCbCrInfo = ahbProperties.yCbCrInfo;
    uint32_t formatFeatures = bufferFormatProperties.formatFeatures;

    // Verify that the YCbCr properties match.
    EXPECT_EQ(bufferFormatProperties.format, yCbCrInfo.vkFormat);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrModel, yCbCrInfo.vkYCbCrModel);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrRange, yCbCrInfo.vkYCbCrRange);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.r,
              yCbCrInfo.vkComponentSwizzleRed);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.g,
              yCbCrInfo.vkComponentSwizzleGreen);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.b,
              yCbCrInfo.vkComponentSwizzleBlue);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.a,
              yCbCrInfo.vkComponentSwizzleAlpha);
    EXPECT_EQ(bufferFormatProperties.suggestedXChromaOffset, yCbCrInfo.vkXChromaOffset);
    EXPECT_EQ(bufferFormatProperties.suggestedYChromaOffset, yCbCrInfo.vkYChromaOffset);

    wgpu::FilterMode expectedFilter =
        (formatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT)
            ? wgpu::FilterMode::Linear
            : wgpu::FilterMode::Nearest;
    EXPECT_EQ(expectedFilter, yCbCrInfo.vkChromaFilter);
    EXPECT_EQ(
        formatFeatures &
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT,
        yCbCrInfo.forceExplicitReconstruction);
    uint64_t expectedExternalFormat = 0u;
    EXPECT_EQ(expectedExternalFormat, yCbCrInfo.externalFormat);
}

// Test querying YCbCr info from the SharedTextureMemory with external format.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests, QueryYCbCrInfoWithExternalFormat) {
    AHardwareBuffer_Desc aHardwareBufferDesc = {
        .width = 4,
        .height = 4,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420,
        .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
    };
    AHardwareBuffer* aHardwareBuffer;
    EXPECT_EQ(AHardwareBuffer_allocate(&aHardwareBufferDesc, &aHardwareBuffer), 0);

    // Query the YCbCr properties of the AHardwareBuffer.
    auto deviceVk = native::vulkan::ToBackend(native::FromAPI(device.Get()));

    VkAndroidHardwareBufferPropertiesANDROID bufferProperties = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
    };

    VkAndroidHardwareBufferFormatPropertiesANDROID bufferFormatProperties;
    native::vulkan::PNextChainBuilder bufferPropertiesChain(&bufferProperties);
    bufferPropertiesChain.Add(&bufferFormatProperties,
                              VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID);

    VkDevice vkDevice = deviceVk->GetVkDevice();
    EXPECT_EQ(deviceVk->fn.GetAndroidHardwareBufferPropertiesANDROID(vkDevice, aHardwareBuffer,
                                                                     &bufferProperties),
              VK_SUCCESS);

    // Query the YCbCr properties of a SharedTextureMemory created from this
    // AHB.
    wgpu::SharedTextureMemoryAHardwareBufferDescriptor stmAHardwareBufferDesc;
    stmAHardwareBufferDesc.handle = aHardwareBuffer;

    wgpu::SharedTextureMemoryDescriptor desc;
    desc.nextInChain = &stmAHardwareBufferDesc;

    wgpu::SharedTextureMemory memory = device.ImportSharedTextureMemory(&desc);

    wgpu::SharedTextureMemoryProperties properties;
    wgpu::SharedTextureMemoryAHardwareBufferProperties ahbProperties = {};
    properties.nextInChain = &ahbProperties;
    memory.GetProperties(&properties);
    auto yCbCrInfo = ahbProperties.yCbCrInfo;
    uint32_t formatFeatures = bufferFormatProperties.formatFeatures;

    // Verify that the YCbCr properties match.
    VkFormat expectedVkFormat = VK_FORMAT_UNDEFINED;
    EXPECT_EQ(expectedVkFormat, yCbCrInfo.vkFormat);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrModel, yCbCrInfo.vkYCbCrModel);
    EXPECT_EQ(bufferFormatProperties.suggestedYcbcrRange, yCbCrInfo.vkYCbCrRange);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.r,
              yCbCrInfo.vkComponentSwizzleRed);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.g,
              yCbCrInfo.vkComponentSwizzleGreen);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.b,
              yCbCrInfo.vkComponentSwizzleBlue);
    EXPECT_EQ(bufferFormatProperties.samplerYcbcrConversionComponents.a,
              yCbCrInfo.vkComponentSwizzleAlpha);
    EXPECT_EQ(bufferFormatProperties.suggestedXChromaOffset, yCbCrInfo.vkXChromaOffset);
    EXPECT_EQ(bufferFormatProperties.suggestedYChromaOffset, yCbCrInfo.vkYChromaOffset);

    wgpu::FilterMode expectedFilter =
        (formatFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT)
            ? wgpu::FilterMode::Linear
            : wgpu::FilterMode::Nearest;
    EXPECT_EQ(expectedFilter, yCbCrInfo.vkChromaFilter);
    EXPECT_EQ(
        formatFeatures &
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT,
        yCbCrInfo.forceExplicitReconstruction);
    EXPECT_EQ(bufferFormatProperties.externalFormat, yCbCrInfo.externalFormat);
}

// Test BeginAccess on an uninitialized texture with external format fails.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests,
       GPUReadForUninitializedTextureWithExternalFormatFails) {
    const AHardwareBuffer_Desc aHardwareBufferDesc = {
        .width = 4,
        .height = 4,
        .layers = 1,
        .format = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420,
        .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE,
    };
    AHardwareBuffer* aHardwareBuffer;
    EXPECT_EQ(AHardwareBuffer_allocate(&aHardwareBufferDesc, &aHardwareBuffer), 0);

    wgpu::SharedTextureMemoryAHardwareBufferDescriptor stmAHardwareBufferDesc;
    stmAHardwareBufferDesc.handle = aHardwareBuffer;

    wgpu::SharedTextureMemoryDescriptor desc;
    desc.nextInChain = &stmAHardwareBufferDesc;

    const wgpu::SharedTextureMemory memory = device.ImportSharedTextureMemory(&desc);

    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = 4;
    descriptor.size.height = 4;
    descriptor.size.depthOrArrayLayers = 1u;
    descriptor.sampleCount = 1u;
    descriptor.format = wgpu::TextureFormat::OpaqueYCbCrAndroid;
    descriptor.mipLevelCount = 1u;
    descriptor.usage = wgpu::TextureUsage::TextureBinding;
    auto texture = memory.CreateTexture(&descriptor);
    AHardwareBuffer_release(aHardwareBuffer);

    wgpu::SharedTextureMemoryBeginAccessDescriptor beginDesc = {};
    beginDesc.initialized = false;
    wgpu::SharedTextureMemoryVkImageLayoutBeginState beginLayout{};
    beginDesc.nextInChain = &beginLayout;

    ASSERT_DEVICE_ERROR(memory.BeginAccess(texture, &beginDesc));
}

// Test sampling of a YCbCr 420 AHB using a static YCbCr sampler.
TEST_P(SharedTextureMemoryYCbCrVulkanSamplersTests, SampleYUVAHB) {
    // Make the AHB and import it as an STM and wgpu::Texture.
    const std::array<uint8_t, 4> yData = {50, 100, 150, 200};
    const std::array<uint8_t, 1> uData = {75};
    const std::array<uint8_t, 1> vData = {125};

    AHardwareBuffer* ahb = MakeYUVAHB(2, 2, yData, uData, vData);

    wgpu::SharedTextureMemoryAHardwareBufferDescriptor stmAHardwareBufferDesc{};
    stmAHardwareBufferDesc.handle = ahb;
    wgpu::SharedTextureMemoryDescriptor stmDesc{};
    stmDesc.nextInChain = &stmAHardwareBufferDesc;
    wgpu::SharedTextureMemory stm = device.ImportSharedTextureMemory(&stmDesc);
    wgpu::Texture ycbcrTex = stm.CreateTexture();

    AHardwareBuffer_release(ahb);

    wgpu::SharedTextureMemoryBeginAccessDescriptor beginDesc{};
    beginDesc.initialized = true;
    wgpu::SharedTextureMemoryVkImageLayoutBeginState beginLayout{};
    beginDesc.nextInChain = &beginLayout;
    EXPECT_EQ(stm.BeginAccess(ycbcrTex, &beginDesc), wgpu::Status::Success);

    // Get the YCbCrVkDescriptor
    wgpu::SharedTextureMemoryProperties stmProperties{};
    wgpu::SharedTextureMemoryAHardwareBufferProperties ahbProperties{};
    stmProperties.nextInChain = &ahbProperties;
    stm.GetProperties(&stmProperties);
    wgpu::YCbCrVkDescriptor yCbCrDesc = ahbProperties.yCbCrInfo;

    // Create the BindGroupLayout and BindGroup.
    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.nextInChain = &yCbCrDesc;

    wgpu::StaticSamplerBindingLayout staticSamplerBinding{};
    staticSamplerBinding.sampler = device.CreateSampler(&samplerDesc);
    staticSamplerBinding.sampledTextureBinding = 1;

    std::array<wgpu::BindGroupLayoutEntry, 2> bglEntries;
    bglEntries[0].binding = 0;
    bglEntries[0].nextInChain = &staticSamplerBinding;
    bglEntries[1].binding = 1;
    bglEntries[1].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;

    wgpu::BindGroupLayoutDescriptor bglDesc {
        .entryCount = bglEntries.size(),
        .entries = bglEntries.data(),
    };
    wgpu::BindGroupLayout bgl = device.CreateBindGroupLayout(&bglDesc);

    wgpu::TextureViewDescriptor viewDesc;
    viewDesc.nextInChain = &yCbCrDesc;
    wgpu::BindGroup bg = utils::MakeBindGroup(device, bgl, {{1, ycbcrTex.CreateView(&viewDesc)}});

    // Create the pipeline that copies from the YCbCr texture to an RGBA one.
    wgpu::ShaderModule module = utils::CreateShaderModule(device, R"(
        @vertex fn quad(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
            const pos = array(
                vec2f(-1.0, -1.0),
                vec2f( 3.0, -1.0),
                vec2f(-1.0,  3.0));
            return vec4f(pos[i], 0.0, 1.0);
        }

        @group(0) @binding(0) var s : sampler;
        @group(0) @binding(1) var t : texture_2d<f32>;
        @fragment fn fs(@builtin(position) pos : vec4f) -> @location(0) vec4f { 
            return textureSample(t, s, pos.xy / 2);
        }
    )");

}

DAWN_INSTANTIATE_TEST(SharedTextureMemoryYCbCrVulkanSamplersTests, VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
