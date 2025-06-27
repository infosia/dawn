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
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureComponentSwizzle));
    }

    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures = {};
        if (SupportsFeatures({wgpu::FeatureName::TextureComponentSwizzle})) {
            requiredFeatures.push_back(wgpu::FeatureName::TextureComponentSwizzle);
        }
        return requiredFeatures;
    }

    struct TestParams {
        wgpu::TextureFormat format;
        // Input data for the source texture. Max 4 bytes for convenience.
        std::array<uint8_t, 4> inputData;
        // Actual bytes written to the source texture.
        uint8_t bytesToWrite;
        // The expected values (R, G, B, A) that `textureLoad` would produce
        // for this format *before* any swizzling. This accounts for default values
        // for missing channels (e.g., G=0, B=0, A=255 for R8Unorm).
        std::array<uint8_t, 4> baseLoadValues;
    };

    // Calculates the expected float value after swizzling and normalization.
    // Uses a switch statement for clarity and ensures floating-point division.
    float GetExpectedValue(wgpu::ComponentSwizzle swizzle, const std::array<uint8_t, 4>& rgbaData) {
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
            case wgpu::ComponentSwizzle::Undefined:
            default:
                DAWN_UNREACHABLE();
                return 0.0f;
        }
    }

    // Define all component swizzle options to test.
    std::array<wgpu::ComponentSwizzle, 6> componentSwizzles = {
        wgpu::ComponentSwizzle::Zero, wgpu::ComponentSwizzle::One, wgpu::ComponentSwizzle::R,
        wgpu::ComponentSwizzle::G,    wgpu::ComponentSwizzle::B,   wgpu::ComponentSwizzle::A};

    void RunTest(TestParams params) {
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

        auto RunSwizzleTest = [this, params, pipeline](wgpu::ComponentSwizzle swizzleRed,
                                                       wgpu::ComponentSwizzle swizzleGreen,
                                                       wgpu::ComponentSwizzle swizzleBlue,
                                                       wgpu::ComponentSwizzle swizzleAlpha) {
            // Create source texture.
            wgpu::TextureDescriptor textureDesc = {};
            textureDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
            textureDesc.size = {1, 1, 1};
            textureDesc.format = params.format;
            wgpu::Texture texture = device.CreateTexture(&textureDesc);

            wgpu::TexelCopyTextureInfo dest = {.texture = texture};
            wgpu::TexelCopyBufferLayout dataLayout = {.bytesPerRow = 256, .rowsPerImage = 1};
            wgpu::Extent3D writeSize = {1, 1, 1};
            device.GetQueue().WriteTexture(&dest, params.inputData.data(), params.bytesToWrite,
                                           &dataLayout, &writeSize);

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
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.DispatchWorkgroups(1);
            pass.End();

            // Submit commands to the queue.
            wgpu::CommandBuffer commands = encoder.Finish();
            queue.Submit(1, &commands);

            float expectedRed = GetExpectedValue(swizzleRed, params.baseLoadValues);
            float expectedGreen = GetExpectedValue(swizzleGreen, params.baseLoadValues);
            float expectedBlue = GetExpectedValue(swizzleBlue, params.baseLoadValues);
            float expectedAlpha = GetExpectedValue(swizzleAlpha, params.baseLoadValues);

            EXPECT_BUFFER_FLOAT_EQ(expectedRed, buffer, 0);
            EXPECT_BUFFER_FLOAT_EQ(expectedGreen, buffer, 4);
            EXPECT_BUFFER_FLOAT_EQ(expectedBlue, buffer, 8);
            EXPECT_BUFFER_FLOAT_EQ(expectedAlpha, buffer, 12);
        };

        for (auto swizzleRed : componentSwizzles) {
            RunSwizzleTest(swizzleRed, wgpu::ComponentSwizzle::G, wgpu::ComponentSwizzle::B,
                           wgpu::ComponentSwizzle::A);
        }

        for (auto swizzleGreen : componentSwizzles) {
            RunSwizzleTest(wgpu::ComponentSwizzle::R, swizzleGreen, wgpu::ComponentSwizzle::B,
                           wgpu::ComponentSwizzle::A);
        }

        for (auto swizzleBlue : componentSwizzles) {
            RunSwizzleTest(wgpu::ComponentSwizzle::R, wgpu::ComponentSwizzle::G, swizzleBlue,
                           wgpu::ComponentSwizzle::A);
        }

        for (auto swizzleAlpha : componentSwizzles) {
            RunSwizzleTest(wgpu::ComponentSwizzle::R, wgpu::ComponentSwizzle::G,
                           wgpu::ComponentSwizzle::B, swizzleAlpha);
        }
    }
};

// Test that texture component swizzle works as expected when the 'texture-component-swizzle'
// feature is enabled. This test verifies each component's swizzle mapping independently
// by using a compute shader to read from a swizzled texture and write to a storage buffer.
// It covers formats with all channels, and formats where some channels are implicitly
// generated (0.0 or 1.0) by `textureLoad` due to the base format.
TEST_P(TextureComponentSwizzleTest, ComponentByComponentSwizzleComputeToBuffer) {
    {
        // All channels present and used.
        TestParams params;
        params.format = wgpu::TextureFormat::RGBA8Unorm;
        params.inputData = {255, 128, 64, 0};
        params.bytesToWrite = 4;
        params.baseLoadValues = {255, 128, 64, 0};
        RunTest(params);
    }
    {
        // Only R channel present. G, B default to 0, A defaults to 255.
        TestParams params;
        params.format = wgpu::TextureFormat::R8Unorm;
        params.inputData = {128, 0, 0, 0};
        params.bytesToWrite = 1;
        params.baseLoadValues = {128, 0, 0, 255};
        RunTest(params);
    }
    {
        // Only R and G channel present. B defaults to 0, A defaults to 255.
        TestParams params;
        params.format = wgpu::TextureFormat::RG8Unorm;
        params.inputData = {128, 64, 0, 0};
        params.bytesToWrite = 2;
        params.baseLoadValues = {128, 64, 0, 255};
        RunTest(params);
    }
}

DAWN_INSTANTIATE_TEST(TextureComponentSwizzleTest, MetalBackend());

}  // anonymous namespace
}  // namespace dawn
