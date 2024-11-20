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

#include <vulkan/vulkan.h>
#include <vector>

#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

constexpr uint32_t kWidth = 32u;
constexpr uint32_t kHeight = 32u;
constexpr uint32_t kDefaultMipLevels = 1u;
constexpr uint32_t kDefaultLayerCount = 1u;
constexpr uint32_t kDefaultSampleCount = 1u;
constexpr wgpu::TextureFormat kDefaultTextureFormat = wgpu::TextureFormat::External;

wgpu::Texture Create2DTexture(wgpu::Device& device,
                              wgpu::TextureFormat format = kDefaultTextureFormat) {
    wgpu::TextureDescriptor descriptor;
    descriptor.dimension = wgpu::TextureDimension::e2D;
    descriptor.size.width = kWidth;
    descriptor.size.height = kHeight;
    descriptor.size.depthOrArrayLayers = kDefaultLayerCount;
    descriptor.sampleCount = kDefaultSampleCount;
    descriptor.format = format;
    descriptor.mipLevelCount = kDefaultMipLevels;
    descriptor.usage = wgpu::TextureUsage::TextureBinding;
    return device.CreateTexture(&descriptor);
}

wgpu::TextureViewDescriptor CreateDefaultViewDescriptor(wgpu::TextureViewDimension dimension) {
    wgpu::TextureViewDescriptor descriptor;
    descriptor.format = kDefaultTextureFormat;
    descriptor.dimension = dimension;
    descriptor.baseMipLevel = 0;
    if (dimension != wgpu::TextureViewDimension::e1D) {
        descriptor.mipLevelCount = kDefaultMipLevels;
    }
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = kDefaultLayerCount;
    return descriptor;
}

wgpu::TextureView Create2DTextureView(wgpu::Texture texture, wgpu::YCbCrVkDescriptor yCbCrDesc) {
    wgpu::TextureViewDescriptor textureDesc =
        CreateDefaultViewDescriptor(wgpu::TextureViewDimension::e2D);
    textureDesc.arrayLayerCount = 1;
    textureDesc.nextInChain = &yCbCrDesc;

    return texture.CreateView(&textureDesc);
}

wgpu::Sampler CreateYCbCrSampler(wgpu::Device device, wgpu::YCbCrVkDescriptor* yCbCrDesc) {
    wgpu::SamplerDescriptor samplerDesc = {};
    samplerDesc.nextInChain = yCbCrDesc;
    return device.CreateSampler(&samplerDesc);
}

class YCbCrInfoWithoutFeatureValidationTest : public ValidationTest {
    void SetUp() override {
        ValidationTest::SetUp();
        DAWN_SKIP_TEST_IF(UsesWire());
    }
};

// Tests that creating a sampler with a valid ycbcr vulkan descriptor raises an error
// if the required feature is not enabled.
TEST_F(YCbCrInfoWithoutFeatureValidationTest, YCbCrSamplerNotSupported) {
    wgpu::SamplerDescriptor samplerDesc = {};
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    samplerDesc.nextInChain = &yCbCrDesc;

    ASSERT_DEVICE_ERROR(device.CreateSampler(&samplerDesc));
}

// Tests that creating a texture with External format raises an error if the required feature is not
// enabled.
TEST_F(YCbCrInfoWithoutFeatureValidationTest, ExternalTextureNotSupported) {
    ASSERT_DEVICE_ERROR(Create2DTexture(device));
}

// Tests that creating a texture view with a valid ycbcr vulkan descriptor raises an error
// if the required feature is not enabled.
TEST_F(YCbCrInfoWithoutFeatureValidationTest, YCbCrTextureViewNotSupported) {
    wgpu::Texture texture = Create2DTexture(device, wgpu::TextureFormat::RGBA8Unorm);

    wgpu::TextureViewDescriptor descriptor =
        CreateDefaultViewDescriptor(wgpu::TextureViewDimension::e2D);
    descriptor.arrayLayerCount = 1;
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    descriptor.nextInChain = &yCbCrDesc;

    ASSERT_DEVICE_ERROR(texture.CreateView(&descriptor));
}

class YCbCrInfoWithFeatureValidationTest : public YCbCrInfoWithoutFeatureValidationTest {
    void SetUp() override {
        ValidationTest::SetUp();
        DAWN_SKIP_TEST_IF(!DAWN_PLATFORM_IS(ANDROID));
        DAWN_SKIP_TEST_IF(UsesWire());
    }

    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::StaticSamplers, wgpu::FeatureName::YCbCrVulkanSamplers};
    }
};

// Tests that creating a sampler with a valid ycbcr vulkan descriptor succeeds if the
// required feature is enabled.
TEST_F(YCbCrInfoWithFeatureValidationTest, YCbCrSamplerSupported) {
    wgpu::SamplerDescriptor samplerDesc = {};
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    samplerDesc.nextInChain = &yCbCrDesc;

    device.CreateSampler(&samplerDesc);
}

// Tests that creating a bind group layout with a valid static sampler succeeds if the
// required feature is enabled.
TEST_F(YCbCrInfoWithFeatureValidationTest, CreateBindGroupWithYCbCrSamplerSupported) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;

    wgpu::YCbCrVkDescriptor yCbCrDesc = {};

    wgpu::BindGroupLayoutEntry& binding0 = entries.emplace_back();
    binding0.binding = 0;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};
    staticSamplerBinding.sampler = CreateYCbCrSampler(device, &yCbCrDesc);
    staticSamplerBinding.sampledTextureBinding = 1;
    binding0.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutEntry& binding1 = entries.emplace_back();
    binding1.binding = 1;
    binding1.texture.sampleType = wgpu::TextureSampleType::Float;
    binding1.texture.viewDimension = wgpu::TextureViewDimension::e2D;
    binding1.texture.multisampled = false;

    wgpu::BindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = entries.size();
    layoutDesc.entries = entries.data();

    wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&layoutDesc);

    wgpu::Texture texture = Create2DTexture(device);
    wgpu::TextureView textureView = Create2DTextureView(texture, yCbCrDesc);

    utils::MakeBindGroup(device, layout, {{1, textureView}});
}

// Verify that creating a bind group layout fails with a static sampler binding that has no texture
// binding.
TEST_F(YCbCrInfoWithFeatureValidationTest, CreateBindGroupWithYCbCrSamplerNoTextureBinding) {
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};

    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};
    staticSamplerBinding.sampler = CreateYCbCrSampler(device, &yCbCrDesc);
    staticSamplerBinding.sampledTextureBinding = 1;
    binding.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Verifies that creating a bind group layout fails with a static sampler binding that refers to an
// invalid binding index for texture binding.
TEST_F(YCbCrInfoWithFeatureValidationTest,
       CreateBindGroupWithYCbCrSamplerInvalidTextureBindingIndex) {
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};

    wgpu::BindGroupLayoutEntry binding = {};
    binding.binding = 0;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};
    staticSamplerBinding.sampler = CreateYCbCrSampler(device, &yCbCrDesc);
    staticSamplerBinding.sampledTextureBinding = 1;
    binding.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &binding;

    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Verifies that creating a bind group layout fails with a static sampler binding that refers to a
// binding that is not a texture binding.
TEST_F(YCbCrInfoWithFeatureValidationTest,
       CreateBindGroupWithYCbCrSamplerInvalidTextureBindingType) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};

    wgpu::BindGroupLayoutEntry& binding0 = entries.emplace_back();
    binding0.binding = 0;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};
    staticSamplerBinding.sampler = CreateYCbCrSampler(device, &yCbCrDesc);
    staticSamplerBinding.sampledTextureBinding = 1;
    binding0.nextInChain = &staticSamplerBinding;

    // This should be a texture binding but it's not.
    wgpu::BindGroupLayoutEntry& binding1 = entries.emplace_back();
    binding1.binding = 1;
    binding1.sampler.type = wgpu::SamplerBindingType::Filtering;

    wgpu::BindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = entries.size();
    layoutDesc.entries = entries.data();

    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&layoutDesc));
}

// Verify that creating a bind group layout fails when two static samplers have the same
// sampled texture binding.
TEST_F(YCbCrInfoWithFeatureValidationTest,
       CreateBindGroupWithYCbCrSamplerDuplicateSampledTextures) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;

    wgpu::YCbCrVkDescriptor yCbCrDesc = {};

    auto& binding0 = entries.emplace_back();
    binding0.binding = 0;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};
    staticSamplerBinding.sampler = CreateYCbCrSampler(device, &yCbCrDesc);
    staticSamplerBinding.sampledTextureBinding = 1;
    binding0.nextInChain = &staticSamplerBinding;

    auto& binding1 = entries.emplace_back();
    binding1.binding = 1;
    binding1.texture.sampleType = wgpu::TextureSampleType::Float;
    binding1.texture.viewDimension = wgpu::TextureViewDimension::e2D;
    binding1.texture.multisampled = false;

    // This static sampler has the same `sampledTextureBinding` as binding 0.
    auto& binding2 = entries.emplace_back();
    binding2.binding = 2;
    binding2.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = entries.size();
    layoutDesc.entries = entries.data();

    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&layoutDesc));
}

// Verifies that creation of a correctly-specified bind group for a layout that
// has a sampler and a static sampler succeeds.
TEST_F(YCbCrInfoWithFeatureValidationTest, CreateBindGroupWithSamplerAndStaticSamplerSupported) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;

    wgpu::BindGroupLayoutEntry& binding0 = entries.emplace_back();
    binding0.binding = 0;
    binding0.sampler.type = wgpu::SamplerBindingType::Filtering;

    wgpu::BindGroupLayoutEntry& binding1 = entries.emplace_back();
    binding1.binding = 1;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};

    wgpu::SamplerDescriptor samplerDesc = {};
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    samplerDesc.nextInChain = &yCbCrDesc;

    staticSamplerBinding.sampler = device.CreateSampler(&samplerDesc);
    staticSamplerBinding.sampledTextureBinding = 2;
    binding1.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutEntry& binding2 = entries.emplace_back();
    binding2.binding = 2;
    binding2.texture.sampleType = wgpu::TextureSampleType::Float;
    binding2.texture.viewDimension = wgpu::TextureViewDimension::e2D;
    binding2.texture.multisampled = false;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = entries.size();
    desc.entries = entries.data();

    wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&desc);

    wgpu::SamplerDescriptor dynamicSamplerDesc;
    dynamicSamplerDesc.minFilter = wgpu::FilterMode::Linear;

    wgpu::Texture texture = Create2DTexture(device);
    wgpu::TextureView textureView = Create2DTextureView(texture, yCbCrDesc);

    utils::MakeBindGroup(device, layout,
                         {{0, device.CreateSampler(&dynamicSamplerDesc)}, {2, textureView}});
}

// Verifies that creation of a bind group with the correct number of entries for a layout that has a
// sampler and a static sampler raises an error if the entry is specified at the index of the static
// sampler rather than that of the sampler.
TEST_F(YCbCrInfoWithFeatureValidationTest, BindGroupCreationForSamplerBindingTypeCausesError) {
    std::vector<wgpu::BindGroupLayoutEntry> entries;

    wgpu::BindGroupLayoutEntry& binding0 = entries.emplace_back();
    binding0.binding = 0;
    binding0.sampler.type = wgpu::SamplerBindingType::Filtering;

    wgpu::BindGroupLayoutEntry& binding1 = entries.emplace_back();
    binding1.binding = 1;
    wgpu::StaticSamplerBindingLayout staticSamplerBinding = {};

    wgpu::SamplerDescriptor samplerDesc = {};
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    samplerDesc.nextInChain = &yCbCrDesc;

    staticSamplerBinding.sampler = device.CreateSampler(&samplerDesc);
    staticSamplerBinding.sampledTextureBinding = 2;
    binding1.nextInChain = &staticSamplerBinding;

    wgpu::BindGroupLayoutEntry& binding2 = entries.emplace_back();
    binding2.binding = 2;
    binding2.texture.sampleType = wgpu::TextureSampleType::Float;
    binding2.texture.viewDimension = wgpu::TextureViewDimension::e2D;
    binding2.texture.multisampled = false;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = entries.size();
    desc.entries = entries.data();

    wgpu::BindGroupLayout layout = device.CreateBindGroupLayout(&desc);

    wgpu::SamplerDescriptor samplerDesc0;
    samplerDesc0.minFilter = wgpu::FilterMode::Linear;

    wgpu::Texture texture = Create2DTexture(device);
    wgpu::TextureView textureView = Create2DTextureView(texture, yCbCrDesc);

    ASSERT_DEVICE_ERROR(utils::MakeBindGroup(
        device, layout, {{1, device.CreateSampler(&samplerDesc0)}, {2, textureView}}));
}

// Tests that creating a texture view with a valid ycbcr vulkan descriptor succeeds if the
// required feature is enabled.
TEST_F(YCbCrInfoWithFeatureValidationTest, YCbCrTextureViewSupported) {
    wgpu::Texture texture = Create2DTexture(device);

    wgpu::TextureViewDescriptor descriptor =
        CreateDefaultViewDescriptor(wgpu::TextureViewDimension::e2D);
    descriptor.arrayLayerCount = 1;
    wgpu::YCbCrVkDescriptor yCbCrDesc = {};
    descriptor.nextInChain = &yCbCrDesc;

    texture.CreateView(&descriptor);
}

// TODO(crbug.com/dawn/2476): Add test validating binding fails if texture view ycbcr info is
// different from that on sampler
// TODO(crbug.com/dawn/2476): Add test validating binding passes if texture view ycbcr info is same
// as that on sampler
// TODO(crbug.com/dawn/2476): Add validation that mipLevel, arrayLayers are always 1 along with 2D
// view dimension (see
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageCreateInfo.html) with
// YCbCr and tests for it.

}  // anonymous namespace
}  // namespace dawn
