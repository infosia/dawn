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
#include <type_traits>
#include <concepts>
#include <utility>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/Range.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/UtilsVulkan.h"
#include "dawn/native/vulkan/VulkanError.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace MinimalAlgebra {

template <size_t Size, typename Scalar = float>
struct Vector {
    using Self = Vector<Size, Scalar>;

    template <typename... Args>
        requires(std::convertible_to<Args, Scalar> && ...)
    constexpr Vector(Args&&... args)
        requires(sizeof...(args) == Size)
    {
        data = {static_cast<Scalar>(args)...};
    }
    constexpr Vector() {data.fill(Scalar());}

    constexpr explicit Vector(std::array<Scalar, Size> data) : data(data) {}

    constexpr Scalar& operator[](size_t i) { return data[i]; }
    constexpr const Scalar& operator[](size_t i) const { return data[i]; }

    constexpr Self operator+(const Self& other) const {
        Self result;
        for (size_t i = 0; i < Size; i++) {
            result[i] = data[i] + other[i];
        }
        return result;
    }
    constexpr Self& operator+=(const Self& other) {
        *this = *this + other;
        return *this;
    }

    constexpr Self operator*(const Scalar& s) const {
        Self result;
        for (size_t i = 0; i < Size; i++) {
            result[i] = data[i] * s;
        }
        return result;
    }
    constexpr Self& operator*=(const Scalar& s) {
        *this = *this * s;
        return *this;
    }

    std::array<Scalar, Size> data = {};
};

// Column-major to match WGSL
template <size_t Cols, size_t Rows, typename Scalar = float>
struct Matrix {
    using Column = Vector<Rows, Scalar>;
    using Self = Matrix<Cols, Rows, Scalar>;

    template <typename... Args>
        requires(std::convertible_to<Args, Column> && ...)
    constexpr Matrix(Args&&... columns)
        requires(sizeof...(columns) == Rows)
    {
        data = {static_cast<Column>(columns)...};
    }

    constexpr Matrix() {data.fill(Column());}

    constexpr static Self Identity()
        requires(Cols == Rows)
    {
        std::array<Scalar, Rows> scales;
        scales.fill(1);
        return Scale({scales});
    }

    template <typename... Args>
        requires(std::is_convertible_v<Args, Scalar> && ...)
    constexpr static Self Scale(Args&&... scales)
        requires(sizeof...(scales) == Rows)
         && (Cols == Rows)
    {
        return Scale(Column(std::forward<Args>(scales)...));
    }

    constexpr static Self Scale(Column scale) requires (Cols == Rows) {
        Self mat;
        for (size_t i = 0; i < Rows; i++) {
            mat[i][i] = scale[i];
        }
        return mat;
    }

    constexpr Column& operator[](size_t i) { return data[i]; }
    constexpr const Column& operator[](size_t i) const { return data[i]; }

    std::array<Column, Cols> data = {};
};

template<size_t M, size_t N, typename Scalar>
constexpr Vector<N, Scalar> Mul(const Matrix<M, N, Scalar>& A, const Vector<M, Scalar>& V) {
    Vector<N, Scalar> result;
    for (size_t i = 0; i < M; i++) {
        result += A[i] * V[i];
    }
    return result;
}

template<size_t M, size_t N, size_t K, typename Scalar>
Matrix<M, N, Scalar> Mul(const Matrix<K, N, Scalar>& A, const Matrix<M, K, Scalar>& B) {
    Matrix<M, N, Scalar> result;
    for (size_t i = 0; i < M; i++) {
        result[i] = Mul(A, B[i]);
    }
    return result;
}

using Vec2f = Vector<2, float>;
using Vec3f = Vector<3, float>;
using Vec4f = Vector<4, float>;
using Mat3x3f = Matrix<3, 3, float>;
using Mat4x3f = Matrix<4, 3, float>;

void f() {
    constexpr Mat3x3f kYCbCrToRGB_Rev709 = {
        Vec3f{1.0f, 1.0f, 1.0f},
        Vec3f{0.0f, -(0.13397432f / 0.7152f), 1.8556f},
        Vec3f{1.5748f, -(0.33480248f / 0.7152f), 0.0f},
    };

    //constexpr Mat3x3f kYCbCrToRGB_Rev7092 = {
    //    {1.0f, 1.0f, 1.0f},
    //    {0.0f, -(0.13397432f / 0.7152f), 1.8556f},
    //    {1.5748f, -(0.33480248f / 0.7152f), 0.0f},
    //};
    constexpr Vec3f YCbCrRangeBias{};
    constexpr Vec3f RGBRangeBias = Mul(kYCbCrToRGB_Rev709, YCbCrRangeBias);

    //Mat4x3f YCbCrToRGBAndBias{kYCbCrToRGB};
}

// Note actually R'G'B' and Y'Cb'Cr as the values are all computed before applying the transfer
// function.
//
// RGB = YCbCr2RGB * RangeYCbCr
// RangeYCbCr = RangeScale * YCbCr + YCbCrRangeBias
// RGB = YCbCr2RGB * RangeScale * YCbCr + RGBRangeBias
// RGBRangeBias = YCbCr2RGB * YCbCrRangeBias

}  // namespace MinimalAlgebra

namespace MA = MinimalAlgebra;

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
    CopyPlane(uData, width / 2, height / 2, ahbPlanes.planes[1]);
    CopyPlane(vData, width / 2, height / 2, ahbPlanes.planes[2]);

    EXPECT_EQ(AHardwareBuffer_unlock(ahb, nullptr), 0);

    return ahb;
}

class SharedTextureMemoryYCbCrVulkanSamplersTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::SharedTextureMemoryAHardwareBuffer,
                wgpu::FeatureName::SharedFenceSyncFD, wgpu::FeatureName::StaticSamplers,
                wgpu::FeatureName::YCbCrVulkanSamplers};
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

    // Make this a parametrized test on the mode, range, and forceExplicitReconstruction

    yCbCrDesc.vkYCbCrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020_KHR;
    yCbCrDesc.vkYCbCrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL_KHR;
    yCbCrDesc.vkComponentSwizzleRed = VK_COMPONENT_SWIZZLE_R;
    yCbCrDesc.vkComponentSwizzleGreen = VK_COMPONENT_SWIZZLE_G;
    yCbCrDesc.vkComponentSwizzleBlue = VK_COMPONENT_SWIZZLE_B;
    yCbCrDesc.vkComponentSwizzleAlpha = VK_COMPONENT_SWIZZLE_A;
    yCbCrDesc.vkChromaFilter = wgpu::FilterMode::Nearest;

    // Create the BindGroupLayout and BindGroup.
    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.nextInChain = &yCbCrDesc;

    wgpu::StaticSamplerBindingLayout staticSamplerBinding{};
    staticSamplerBinding.sampler = device.CreateSampler(&samplerDesc);
    staticSamplerBinding.sampledTextureBinding = 1;

    std::array<wgpu::BindGroupLayoutEntry, 2> bglEntries;
    bglEntries[0].binding = 0;
    bglEntries[0].visibility = wgpu::ShaderStage::Fragment;
    bglEntries[0].nextInChain = &staticSamplerBinding;
    bglEntries[1].binding = 1;
    bglEntries[1].visibility = wgpu::ShaderStage::Fragment;
    bglEntries[1].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;

    wgpu::BindGroupLayoutDescriptor bglDesc{
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

    utils::ComboRenderPipelineDescriptor pDesc;
    pDesc.vertex.module = module;
    pDesc.cFragment.module = module;
    pDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
    pDesc.layout = utils::MakePipelineLayout(device, {bgl});
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pDesc);

    // Do the copy.
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    utils::BasicRenderPass renderPass =
        utils::CreateBasicRenderPass(device, 2, 2, wgpu::TextureFormat::RGBA8Unorm);
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass.renderPassInfo);
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.Draw(3);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8(0, 0, 0, 0), renderPass.color, 0, 0);
    EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8(0, 0, 0, 0), renderPass.color, 0, 1);
    EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8(0, 0, 0, 0), renderPass.color, 1, 0);
    EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8(0, 0, 0, 0), renderPass.color, 1, 1);
}

DAWN_INSTANTIATE_TEST(SharedTextureMemoryYCbCrVulkanSamplersTests, VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
