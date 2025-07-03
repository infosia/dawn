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

#include <array>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class TextureComponentSwizzleTest : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures = {};
        if (SupportsFeatures({wgpu::FeatureName::TextureComponentSwizzle})) {
            requiredFeatures.push_back(wgpu::FeatureName::TextureComponentSwizzle);
        }
        return requiredFeatures;
    }

    // Calculates the expected float value after swizzling and normalization.
    // Uses a switch statement for clarity and ensures floating-point division.
    float getExpectedValue(wgpu::ComponentSwizzle swizzle, std::array<uint8_t, 4> rgbaData) {
        switch (swizzle) {
            case wgpu::ComponentSwizzle::Zero:
                return 0.0f;
            case wgpu::ComponentSwizzle::One:
                return 1.0f;
            case wgpu::ComponentSwizzle::R:
                return static_cast<float>(rgbaData[0]) / 255.0f;
            case wgpu::ComponentSwizzle::G:
                return static_cast<float>(rgbaData[1]) / 255.0f;
            case wgpu::ComponentSwizzle::B:
                return static_cast<float>(rgbaData[2]) / 255.0f;
            case wgpu::ComponentSwizzle::A:
                return static_cast<float>(rgbaData[3]) / 255.0f;
            // Undefined is not tested, but a default case is good practice for enums.
            case wgpu::ComponentSwizzle::Undefined:
            default:
                DAWN_UNREACHABLE();
                return 0.0f;  // Should not be reached
        }
    }
};

// Test that texture component swizzle works as expected if
// 'texture-component-swizzle' is enabled.
TEST_P(TextureComponentSwizzleTest, Works) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureComponentSwizzle));

    // Define all component swizzle options to test.
    std::array<wgpu::ComponentSwizzle, 6> componentSwizzles = {
        wgpu::ComponentSwizzle::Zero, wgpu::ComponentSwizzle::One, wgpu::ComponentSwizzle::R,
        wgpu::ComponentSwizzle::G,    wgpu::ComponentSwizzle::B,   wgpu::ComponentSwizzle::A};

    // The shader module and pipeline can be created once as they don't change per iteration.
    wgpu::ShaderModule module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var texture : texture_2d<f32>;
        @group(0) @binding(1) var<storage, read_write> buffer: vec4f;

        @compute @workgroup_size(1) fn main() {
          // textureLoad samples at an integer coordinate and mip level 0.
          buffer = textureLoad(texture, vec2i(0, 0), 0);
        })");

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = module;
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Input color values for the RGBA8Unorm texture.
    // These values will be normalized when read by the shader.
    std::array<uint8_t, 4> rgbaData = {255, 128, 64,
                                       0};  // R=255 (1.0f), G=128 (0.5f), B=64 (~0.25f), A=0 (0.0f)

    // Loop through all combinations of component swizzles for R, G, B, and A.
    for (auto swizzleRed : componentSwizzles) {
        for (auto swizzleGreen : componentSwizzles) {
            for (auto swizzleBlue : componentSwizzles) {
                for (auto swizzleAlpha : componentSwizzles) {
                    // Create source texture.
                    wgpu::TextureDescriptor textureDesc = {};
                    textureDesc.usage =
                        wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
                    textureDesc.size = {1, 1, 1};
                    textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
                    wgpu::Texture texture = device.CreateTexture(&textureDesc);

                    // Create buffer to hold the initial texture data.
                    uint64_t rgbaDataSize =
                        static_cast<uint64_t>(rgbaData.size() * sizeof(uint8_t));
                    wgpu::Buffer textureData = utils::CreateBufferFromData(
                        device, rgbaData.data(), rgbaDataSize, wgpu::BufferUsage::CopySrc);

                    wgpu::TexelCopyBufferInfo texelCopyBufferInfo = {};
                    texelCopyBufferInfo.buffer = textureData;
                    texelCopyBufferInfo.layout.bytesPerRow = 256;

                    wgpu::TexelCopyTextureInfo texelCopyTextureInfo = {};
                    texelCopyTextureInfo.texture = texture;

                    wgpu::Extent3D copySize = {1, 1, 1};

                    // Encode command to copy data to the texture.
                    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                    encoder.CopyBufferToTexture(&texelCopyBufferInfo, &texelCopyTextureInfo,
                                                &copySize);

                    // Create a storage buffer that will store the result of textureLoad.
                    // Initialize with arbitrary values (42) to ensure they are overwritten.
                    std::array<float, 4> data = {42.0f, 42.0f, 42.0f, 42.0f};
                    uint64_t bufferSize = static_cast<uint64_t>(data.size() * sizeof(float));
                    wgpu::Buffer buffer = utils::CreateBufferFromData(
                        device, data.data(), bufferSize,
                        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);

                    // Create the TextureView with the specified swizzle.
                    wgpu::TextureViewDescriptor viewDesc = {};
                    wgpu::TextureComponentSwizzleDescriptor swizzleDesc = {};
                    swizzleDesc.swizzle.r = swizzleRed;
                    swizzleDesc.swizzle.g = swizzleGreen;
                    swizzleDesc.swizzle.b = swizzleBlue;
                    swizzleDesc.swizzle.a = swizzleAlpha;
                    viewDesc.nextInChain = &swizzleDesc;
                    wgpu::TextureView textureView = texture.CreateView(&viewDesc);

                    // Set up bind group using the swizzled texture view and the result buffer.
                    wgpu::BindGroup bindGroup =
                        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                             {{0, textureView}, {1, buffer, 0, bufferSize}});

                    // Issue compute dispatch to run the shader.
                    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
                    pass.SetPipeline(pipeline);
                    pass.SetBindGroup(0, bindGroup);
                    pass.DispatchWorkgroups(1);
                    pass.End();

                    // Submit commands to the queue.
                    wgpu::CommandBuffer commands = encoder.Finish();
                    queue.Submit(1, &commands);

                    float expectedRed = getExpectedValue(swizzleRed, rgbaData);
                    float expectedGreen = getExpectedValue(swizzleGreen, rgbaData);
                    float expectedBlue = getExpectedValue(swizzleBlue, rgbaData);
                    float expectedAlpha = getExpectedValue(swizzleAlpha, rgbaData);

                    EXPECT_BUFFER_FLOAT_EQ(expectedRed, buffer, 0);
                    EXPECT_BUFFER_FLOAT_EQ(expectedGreen, buffer, 4);
                    EXPECT_BUFFER_FLOAT_EQ(expectedBlue, buffer, 8);
                    EXPECT_BUFFER_FLOAT_EQ(expectedAlpha, buffer, 12);
                }
            }
        }
    }
}

DAWN_INSTANTIATE_TEST(TextureComponentSwizzleTest, MetalBackend(), VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
