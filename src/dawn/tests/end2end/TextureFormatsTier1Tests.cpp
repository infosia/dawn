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

#include <cmath>
#include <string>
#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

int8_t ConvertFloatToSnorm8(float value) {
    float roundedValue = (value >= 0) ? (value + 0.5f) : (value - 0.5f);
    float clampedValue = std::clamp(roundedValue, -128.0f, 127.0f);
    return static_cast<int8_t>(clampedValue);
}

int16_t ConvertFloatToSnorm16(float value) {
    float roundedValue = (value >= 0) ? (value + 0.5f) : (value - 0.5f);
    float clampedValue = std::clamp(roundedValue, -32768.0f, 32767.0f);
    return static_cast<int16_t>(clampedValue);
}

uint16_t ConvertFloatToUnorm16(float value) {
    float roundedValue = value + 0.5f;
    float clampedValue = std::clamp(roundedValue, 0.0f, 65535.0f);
    return static_cast<uint16_t>(clampedValue);
}

enum class FormatType { kSnorm8, kSnorm16, kUnorm16 };
FormatType GetFormatType(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::R8Snorm:
        case wgpu::TextureFormat::RG8Snorm:
        case wgpu::TextureFormat::RGBA8Snorm:
            return FormatType::kSnorm8;
        case wgpu::TextureFormat::R16Snorm:
        case wgpu::TextureFormat::RG16Snorm:
        case wgpu::TextureFormat::RGBA16Snorm:
            return FormatType::kSnorm16;
        case wgpu::TextureFormat::R16Unorm:
        case wgpu::TextureFormat::RG16Unorm:
        case wgpu::TextureFormat::RGBA16Unorm:
            return FormatType::kUnorm16;
        default:
            return FormatType::kSnorm8;
    }
}

class TextureFormatsTier1Test : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures = {};
        if (SupportsFeatures({wgpu::FeatureName::TextureFormatsTier1})) {
            requiredFeatures.push_back(wgpu::FeatureName::TextureFormatsTier1);
        }
        return requiredFeatures;
    }

    const char* GetFullScreenQuadVS() {
        return R"(
                @vertex
                fn vs_main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
                    var positions = array<vec2<f32>, 3>(
                        vec2<f32>(-1.0, -1.0),
                        vec2<f32>( 3.0, -1.0),
                        vec2<f32>( -1.0, 3.0));
                    return vec4<f32>(positions[VertexIndex], 0.0, 1.0);
                }
            )";
    }

    std::string GenerateFragmentShader(const std::vector<float>& srcColorFloats) {
        std::ostringstream fsCodeStream;
        fsCodeStream << R"(
    @fragment
    fn fs_main() -> @location(0) vec4<f32> {
        return vec4<f32>()";

        for (size_t i = 0; i < 4; ++i) {
            if (i < srcColorFloats.size()) {
                fsCodeStream << srcColorFloats[i];
            } else {
                fsCodeStream << (i == 3 ? "1.0" : "0.0");
            }
            if (i < 3) {
                fsCodeStream << ", ";
            }
        }

        fsCodeStream << ");\n"
                     << "}\n";

        return fsCodeStream.str();
    }
};

class RenderAttachmentFormatsTest : public TextureFormatsTier1Test {
  protected:
    void RunRenderTest(wgpu::TextureFormat format, const std::vector<float>& originData) {
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureFormatsTier1));
        wgpu::Extent3D textureSize = {16, 16, 1};
        wgpu::TextureDescriptor textureDesc;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = textureSize;
        textureDesc.format = format;

        wgpu::Texture texture = device.CreateTexture(&textureDesc);

        std::string fsCode = GenerateFragmentShader(originData);

        std::string combinedShaderCode = GetFullScreenQuadVS() + fsCode;
        wgpu::ShaderModule shaderModule =
            utils::CreateShaderModule(device, combinedShaderCode.c_str());

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.cFragment.module = shaderModule;

        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.cFragment.targetCount = 1;
        pipelineDesc.cTargets[0].format = format;
        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

        wgpu::TextureView textureView = texture.CreateView();

        utils::ComboRenderPassDescriptor renderPass;
        renderPass.cColorAttachments[0].view = textureView;
        renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
        renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
        renderPass.colorAttachmentCount = 1;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);

        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        std::vector<int8_t> expectedLowerBounds8;
        std::vector<int8_t> expectedUpperBounds8;
        std::vector<int16_t> expectedLowerBounds16;
        std::vector<int16_t> expectedUpperBounds16;
        std::vector<uint16_t> expectedLowerBoundsUnorm16;
        std::vector<uint16_t> expectedUpperBoundsUnorm16;

        FormatType formatType = GetFormatType(format);
        uint32_t componentCount = utils::GetTextureComponentCount(format);

        for (uint32_t i = 0; i < componentCount; ++i) {
            float floatComponent = originData[i];
            float tolerance = 0.6f;

            switch (formatType) {
                case FormatType::kSnorm8: {
                    float scaledComponent = floatComponent * 127.0f;
                    int8_t lowerExpectation = ConvertFloatToSnorm8(scaledComponent - tolerance);
                    int8_t upperExpectation = ConvertFloatToSnorm8(scaledComponent + tolerance);
                    expectedLowerBounds8.push_back(lowerExpectation);
                    expectedUpperBounds8.push_back(upperExpectation);
                    break;
                }
                case FormatType::kSnorm16: {
                    float scaledComponent = floatComponent * 32767.0f;
                    int16_t lowerExpectation = ConvertFloatToSnorm16(scaledComponent - tolerance);
                    int16_t upperExpectation = ConvertFloatToSnorm16(scaledComponent + tolerance);
                    expectedLowerBounds16.push_back(lowerExpectation);
                    expectedUpperBounds16.push_back(upperExpectation);
                    break;
                }
                case FormatType::kUnorm16: {
                    float clampedComponent = floatComponent * 65535.0f;
                    uint16_t lowerExpectation = ConvertFloatToUnorm16(clampedComponent - tolerance);
                    uint16_t upperExpectation = ConvertFloatToUnorm16(clampedComponent + tolerance);
                    expectedLowerBoundsUnorm16.push_back(lowerExpectation);
                    expectedUpperBoundsUnorm16.push_back(upperExpectation);
                    break;
                }
            }
        }

        switch (formatType) {
            case FormatType::kSnorm8:
                EXPECT_TEXTURE_SNORM8_BETWEEN(expectedLowerBounds8, expectedUpperBounds8, texture,
                                              {0, 0}, {1, 1}, format);
                break;
            case FormatType::kSnorm16:
                EXPECT_TEXTURE_SNORM16_BETWEEN(expectedLowerBounds16, expectedUpperBounds16,
                                               texture, {0, 0}, {1, 1}, format);
                break;
            case FormatType::kUnorm16:
                EXPECT_TEXTURE_UNORM16_BETWEEN(expectedLowerBoundsUnorm16,
                                               expectedUpperBoundsUnorm16, texture, {0, 0}, {1, 1},
                                               format);
                break;
        }
    }
};

// Test that r8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, R8SnormRenderAttachment) {
    std::vector<float> originData = {-0.5f};  // R
    RunRenderTest(wgpu::TextureFormat::R8Snorm, originData);
}

// Test that rg8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RG8SnormRenderAttachment) {
    std::vector<float> originData = {-0.5f, 0.25f};  // RG
    RunRenderTest(wgpu::TextureFormat::RG8Snorm, originData);
}

// Test that r8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RGBA8SnormRenderAttachment) {
    std::vector<float> originData = {-0.5f, 0.25f, -1.0f, 1.0f};  // RGBA
    RunRenderTest(wgpu::TextureFormat::RGBA8Snorm, originData);
}

// Test that r16unorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, R16UnormRenderAttachment) {
    std::vector<float> originData = {0.375f};
    RunRenderTest(wgpu::TextureFormat::R16Unorm, originData);
}

// Test that r16snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, R16SnormRenderAttachment) {
    std::vector<float> originData = {-0.375f};
    RunRenderTest(wgpu::TextureFormat::R16Snorm, originData);
}

// Test that rg16unorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RG16UnormRenderAttachment) {
    std::vector<float> originData = {0.375f, 0.75f};
    RunRenderTest(wgpu::TextureFormat::RG16Unorm, originData);
}

// Test that rg16snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RG16SnormRenderAttachment) {
    std::vector<float> originData = {-0.375f, 0.625f};
    RunRenderTest(wgpu::TextureFormat::RG16Snorm, originData);
}

// Test that rgba16unorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RGBA16UnormRenderAttachment) {
    std::vector<float> originData = {0.375f, 0.75f, 0.125f, 0.875f};
    RunRenderTest(wgpu::TextureFormat::RGBA16Unorm, originData);
}

// Test that rgba16snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentFormatsTest, RGBA16SnormRenderAttachment) {
    std::vector<float> originData = {-0.375f, 0.625f, -0.875f, 0.5f};
    RunRenderTest(wgpu::TextureFormat::RGBA16Snorm, originData);
}

DAWN_INSTANTIATE_TEST(RenderAttachmentFormatsTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend(),
                      OpenGLBackend());

class BlendableFormatsTest : public TextureFormatsTier1Test {
  protected:
    void RunBlendTest(wgpu::TextureFormat format,
                      const std::vector<float>& srcColorFloats,
                      const std::vector<float>& clearColorFloats) {
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureFormatsTier1));

        std::string fsCode = GenerateFragmentShader(srcColorFloats);

        wgpu::Extent3D textureSize = {16, 16, 1};
        wgpu::TextureDescriptor textureDesc;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = textureSize;
        textureDesc.format = format;
        wgpu::Texture renderTarget = device.CreateTexture(&textureDesc);
        wgpu::TextureView renderTargetView = renderTarget.CreateView();

        std::string combinedShaderCode = GetFullScreenQuadVS() + fsCode;
        wgpu::ShaderModule shaderModule =
            utils::CreateShaderModule(device, combinedShaderCode.c_str());

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.cFragment.module = shaderModule;
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.cFragment.targetCount = 1;

        pipelineDesc.cTargets[0].format = format;
        pipelineDesc.cTargets[0].blend = &pipelineDesc.cBlends[0];

        pipelineDesc.cBlends[0].color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        pipelineDesc.cBlends[0].color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        pipelineDesc.cBlends[0].color.operation = wgpu::BlendOperation::Add;
        pipelineDesc.cBlends[0].alpha.srcFactor = wgpu::BlendFactor::One;
        pipelineDesc.cBlends[0].alpha.dstFactor = wgpu::BlendFactor::Zero;
        pipelineDesc.cBlends[0].alpha.operation = wgpu::BlendOperation::Add;
        pipelineDesc.cTargets[0].writeMask = wgpu::ColorWriteMask::All;

        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        utils::ComboRenderPassDescriptor renderPass;
        renderPass.cColorAttachments[0].view = renderTargetView;
        renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;

        renderPass.cColorAttachments[0].clearValue = {clearColorFloats[0], clearColorFloats[1],
                                                      clearColorFloats[2], clearColorFloats[3]};
        renderPass.colorAttachmentCount = 1;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        std::vector<float> expectedBlendedFloats(4);
        float srcR = (srcColorFloats.size() > 0) ? srcColorFloats[0] : 0.0f;
        float srcG = (srcColorFloats.size() > 1) ? srcColorFloats[1] : 0.0f;
        float srcB = (srcColorFloats.size() > 2) ? srcColorFloats[2] : 0.0f;
        float srcA = (srcColorFloats.size() > 3) ? srcColorFloats[3] : 1.0f;

        float dstR = clearColorFloats[0];
        float dstG = clearColorFloats[1];
        float dstB = clearColorFloats[2];

        expectedBlendedFloats[0] = srcR * srcA + dstR * (1.0f - srcA);
        expectedBlendedFloats[1] = srcG * srcA + dstG * (1.0f - srcA);
        expectedBlendedFloats[2] = srcB * srcA + dstB * (1.0f - srcA);
        expectedBlendedFloats[3] = srcA;

        std::vector<int8_t> expectedLowerBounds8;
        std::vector<int8_t> expectedUpperBounds8;
        std::vector<int16_t> expectedLowerBounds16;
        std::vector<int16_t> expectedUpperBounds16;
        std::vector<uint16_t> expectedLowerBoundsUnorm16;
        std::vector<uint16_t> expectedUpperBoundsUnorm16;

        FormatType formatType = GetFormatType(format);
        uint32_t componentCount = utils::GetTextureComponentCount(format);

        for (uint32_t i = 0; i < componentCount; ++i) {
            float floatComponent = expectedBlendedFloats[i];
            float tolerance = 0.6f;

            switch (formatType) {
                case FormatType::kSnorm8: {
                    float scaledComponent = floatComponent * 127.0f;
                    int8_t lowerExpectation = ConvertFloatToSnorm8(scaledComponent - tolerance);
                    int8_t upperExpectation = ConvertFloatToSnorm8(scaledComponent + tolerance);
                    expectedLowerBounds8.push_back(lowerExpectation);
                    expectedUpperBounds8.push_back(upperExpectation);
                    break;
                }
                case FormatType::kSnorm16: {
                    float scaledComponent = floatComponent * 32767.0f;
                    int16_t lowerExpectation = ConvertFloatToSnorm16(scaledComponent - tolerance);
                    int16_t upperExpectation = ConvertFloatToSnorm16(scaledComponent + tolerance);
                    expectedLowerBounds16.push_back(lowerExpectation);
                    expectedUpperBounds16.push_back(upperExpectation);
                    break;
                }
                case FormatType::kUnorm16: {
                    float clampedComponent = floatComponent * 65535.0f;
                    uint16_t lowerExpectation = ConvertFloatToUnorm16(clampedComponent - tolerance);
                    uint16_t upperExpectation = ConvertFloatToUnorm16(clampedComponent + tolerance);
                    expectedLowerBoundsUnorm16.push_back(lowerExpectation);
                    expectedUpperBoundsUnorm16.push_back(upperExpectation);
                    break;
                }
            }
        }

        switch (formatType) {
            case FormatType::kSnorm8:
                EXPECT_TEXTURE_SNORM8_BETWEEN(expectedLowerBounds8, expectedUpperBounds8,
                                              renderTarget, {0, 0}, {1, 1}, format);
                break;
            case FormatType::kSnorm16:
                EXPECT_TEXTURE_SNORM16_BETWEEN(expectedLowerBounds16, expectedUpperBounds16,
                                               renderTarget, {0, 0}, {1, 1}, format);
                break;
            case FormatType::kUnorm16:
                EXPECT_TEXTURE_UNORM16_BETWEEN(expectedLowerBoundsUnorm16,
                                               expectedUpperBoundsUnorm16, renderTarget, {0, 0},
                                               {1, 1}, format);
                break;
        }
    }
};

// Test that r8snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, R8SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::R8Snorm, srcColor, clearColor);
}

// Test that rg8snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RG8SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RG8Snorm, srcColor, clearColor);
}

// Test that rgba8snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RGBA8SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RGBA8Snorm, srcColor, clearColor);
}

// Test that r16unorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, R16UnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::R16Unorm, srcColor, clearColor);
}

// Test that r16snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, R16SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::R16Snorm, srcColor, clearColor);
}

// Test that rg16unorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RG16UnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RG16Unorm, srcColor, clearColor);
}

// Test that rg16snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RG16SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RG16Snorm, srcColor, clearColor);
}

// Test that rgba16unorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RGBA16UnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RGBA16Unorm, srcColor, clearColor);
}

// Test that rgba16snorm format is blendable when 'texture-formats-tier1' is enabled.
TEST_P(BlendableFormatsTest, RGBA16SnormBlendable) {
    std::vector<float> srcColor = {1.0f, 0.0f, 0.0f, 0.5f};
    std::vector<float> clearColor = {0.0f, 0.0f, 1.0f, 1.0f};
    RunBlendTest(wgpu::TextureFormat::RGBA16Snorm, srcColor, clearColor);
}

DAWN_INSTANTIATE_TEST(BlendableFormatsTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend(),
                      OpenGLBackend());

class MultisampleResolveFromatsTest : public TextureFormatsTier1Test {
  protected:
    static constexpr uint32_t kSize = 16;
    static constexpr uint32_t kMultisampleCount = 4;
    static constexpr uint32_t kSingleSampleCount = 1;
    void RunMultisampleResolveFromatsTest(wgpu::TextureFormat format,
                                          const std::vector<float>& expectedDrawColorFloats) {
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureFormatsTier1));
        std::string fsCode = GenerateFragmentShader(expectedDrawColorFloats);

        wgpu::TextureDescriptor multisampleColorDesc;
        multisampleColorDesc.usage = wgpu::TextureUsage::RenderAttachment;
        multisampleColorDesc.dimension = wgpu::TextureDimension::e2D;
        multisampleColorDesc.size = {kSize, kSize, 1};
        multisampleColorDesc.format = format;
        multisampleColorDesc.sampleCount = kMultisampleCount;

        wgpu::Texture multisampleColorTexture = device.CreateTexture(&multisampleColorDesc);
        wgpu::TextureView multisampleColorView = multisampleColorTexture.CreateView();

        wgpu::TextureDescriptor resolveTargetDesc;
        resolveTargetDesc.usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        resolveTargetDesc.dimension = wgpu::TextureDimension::e2D;
        resolveTargetDesc.size = {kSize, kSize, 1};
        resolveTargetDesc.format = format;
        resolveTargetDesc.sampleCount = kSingleSampleCount;

        wgpu::Texture resolveTargetTexture = device.CreateTexture(&resolveTargetDesc);
        wgpu::TextureView resolveTargetView = resolveTargetTexture.CreateView();

        std::string combinedShaderCode = GetFullScreenQuadVS() + fsCode;
        wgpu::ShaderModule shaderModule =
            utils::CreateShaderModule(device, combinedShaderCode.c_str());

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.cFragment.module = shaderModule;
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        pipelineDesc.cFragment.targetCount = 1;
        pipelineDesc.cTargets[0].format = format;
        pipelineDesc.multisample.count = kMultisampleCount;

        pipelineDesc.cTargets[0].writeMask = wgpu::ColorWriteMask::All;

        wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&pipelineDesc);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        utils::ComboRenderPassDescriptor renderPass;
        renderPass.cColorAttachments[0].view = multisampleColorView;
        renderPass.cColorAttachments[0].resolveTarget = resolveTargetView;
        renderPass.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
        renderPass.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
        renderPass.colorAttachmentCount = 1;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        uint32_t componentCount = utils::GetTextureComponentCount(format);

        std::vector<int8_t> expectedLowerBounds;
        std::vector<int8_t> expectedUpperBounds;

        for (uint32_t i = 0; i < componentCount; ++i) {
            float floatComponent = expectedDrawColorFloats[i];
            int8_t lowerSnormExpectation = ConvertFloatToSnorm8(floatComponent * 127.0f - 0.6f);
            int8_t upperSnormExpectation = ConvertFloatToSnorm8(floatComponent * 127.0f + 0.6f);

            expectedLowerBounds.push_back(lowerSnormExpectation);
            expectedUpperBounds.push_back(upperSnormExpectation);
        }

        EXPECT_TEXTURE_SNORM8_BETWEEN(expectedLowerBounds, expectedUpperBounds,
                                      resolveTargetTexture, {0, 0}, {1, 1}, format);
    }
};

// Test that r8snorm format has multisample and resolve capability
// if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleResolveFromatsTest, R8SnormMultisampleResolve) {
    std::vector<float> expectedDrawColor = {1.0f, 0.0f, 0.0f, 1.0f};
    RunMultisampleResolveFromatsTest(wgpu::TextureFormat::R8Snorm, expectedDrawColor);
}

// Test that rg8snorm format has multisample and resolve capability
// if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleResolveFromatsTest, RG8SnormMultisampleResolve) {
    std::vector<float> expectedDrawColor = {1.0f, -0.5f, 0.0f, 1.0f};
    RunMultisampleResolveFromatsTest(wgpu::TextureFormat::RG8Snorm, expectedDrawColor);
}

// Test that rgba8snorm format has multisample and resolve capability
// if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleResolveFromatsTest, RGBA8SnormMultisampleResolve) {
    std::vector<float> expectedDrawColor = {1.0f, -0.5f, -1.0f, 0.5f};
    RunMultisampleResolveFromatsTest(wgpu::TextureFormat::RGBA8Snorm, expectedDrawColor);
}

DAWN_INSTANTIATE_TEST(MultisampleResolveFromatsTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend(),
                      OpenGLBackend());

class MultisampleFormatsTest : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        std::vector<wgpu::FeatureName> requiredFeatures = {};
        if (SupportsFeatures({wgpu::FeatureName::TextureFormatsTier1})) {
            requiredFeatures.push_back(wgpu::FeatureName::TextureFormatsTier1);
        }
        return requiredFeatures;
    }

    const char* GetFullScreenQuadVS() {
        return R"(
                @vertex
                fn vs_main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
                    var positions = array<vec2<f32>, 3>(
                        vec2<f32>(-1.0, -1.0),
                        vec2<f32>( 3.0, -1.0),
                        vec2<f32>(-1.0, 3.0));
                    return vec4<f32>(positions[VertexIndex], 0.0, 1.0);
                }
            )";
    }

    // Fragment shader for the first pass: drawing the expected color to the multisample texture
    std::string GenerateDrawFragmentShader(const std::vector<float>& srcColorFloats) {
        std::ostringstream fsCodeStream;
        fsCodeStream << R"(
                @fragment
                fn fs_main() -> @location(0) vec4<f32> {
                    return vec4<f32>()";

        for (size_t i = 0; i < 4; ++i) {
            if (i < srcColorFloats.size()) {
                fsCodeStream << srcColorFloats[i];
            } else {
                fsCodeStream << (i == 3 ? "1.0" : "0.0");
            }
            if (i < 3) {
                fsCodeStream << ", ";
            }
        }

        fsCodeStream << ");\n"
                     << "}\n";

        return fsCodeStream.str();
    }

    // Fragment shader for the second pass: reading multisample texture and writing verification
    // color
    std::string GetMultisampleValidationFS(wgpu::TextureFormat format) {
        std::ostringstream fsCodeStream;
        std::string componentType = "f32";
        std::string textureType = "texture_multisampled_2d<f32>";

        fsCodeStream << R"(
                @group(0) @binding(0) var myMultisampleTexture: )"
                     << textureType << R"(;
                @group(0) @binding(1) var<uniform> expectedColorUniform: vec4<f32>;

                @fragment
                fn fs_main(@builtin(position) fragCoord: vec4<f32>) -> @location(0) vec4<f32> {
                    let texelCoord = vec2<i32>(fragCoord.xy);
                    let numSamples = textureNumSamples(myMultisampleTexture);

                    var allSamplesMatch = true;
                    // Epsilon for floating point comparison of normalized values
                    // A small epsilon like 1/65535 or 1/32767 is appropriate for 16-bit formats.
                    // We will use a slightly larger one to account for potential precision differences.
                    let epsilon = 0.005;

                    for (var i: u32 = 0; i < numSamples; i = i + 1) {
                        let sampleColor = textureLoad(myMultisampleTexture, texelCoord, i);
                        if (abs(sampleColor.r - expectedColorUniform.r) > epsilon ||
                            abs(sampleColor.g - expectedColorUniform.g) > epsilon ||
                            abs(sampleColor.b - expectedColorUniform.b) > epsilon ||
                            abs(sampleColor.a - expectedColorUniform.a) > epsilon) {
                            allSamplesMatch = false;
                            break;
                        }
                    }

                    if (allSamplesMatch) {
                        return vec4<f32>(1.0, 0.0, 0.0, 1.0);
                    } else {
                        return vec4<f32>(0.0, 1.0, 0.0, 1.0);
                    }
                }
            )";
        return fsCodeStream.str();
    }
    static constexpr uint32_t kSize = 16;
    static constexpr uint32_t kMultisampleCount = 4;

    void RunMultisampleFormatsTest(wgpu::TextureFormat format,
                                   std::vector<float> expectedDrawColorFloats) {
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TextureFormatsTier1));

        wgpu::TextureDescriptor multisampleColorDesc;
        multisampleColorDesc.usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
        multisampleColorDesc.dimension = wgpu::TextureDimension::e2D;
        multisampleColorDesc.size = {kSize, kSize, 1};
        multisampleColorDesc.format = format;
        multisampleColorDesc.sampleCount = kMultisampleCount;

        wgpu::Texture multisampleColorTexture = device.CreateTexture(&multisampleColorDesc);
        wgpu::TextureView multisampleColorView = multisampleColorTexture.CreateView({});

        std::string drawFsCode = GenerateDrawFragmentShader(expectedDrawColorFloats);
        wgpu::ShaderModule drawShaderModule =
            utils::CreateShaderModule(device, (GetFullScreenQuadVS() + drawFsCode).c_str());

        utils::ComboRenderPipelineDescriptor drawPipelineDesc;
        drawPipelineDesc.vertex.module = drawShaderModule;
        drawPipelineDesc.cFragment.module = drawShaderModule;
        drawPipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        drawPipelineDesc.cFragment.targetCount = 1;
        drawPipelineDesc.cTargets[0].format = format;
        drawPipelineDesc.multisample.count = kMultisampleCount;
        drawPipelineDesc.cTargets[0].writeMask = wgpu::ColorWriteMask::All;

        wgpu::RenderPipeline drawPipeline = device.CreateRenderPipeline(&drawPipelineDesc);

        wgpu::CommandEncoder encoder1 = device.CreateCommandEncoder();
        utils::ComboRenderPassDescriptor renderPass1;
        renderPass1.cColorAttachments[0].view = multisampleColorView;
        renderPass1.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass1.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
        renderPass1.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
        renderPass1.colorAttachmentCount = 1;

        wgpu::RenderPassEncoder pass1 = encoder1.BeginRenderPass(&renderPass1);
        pass1.SetPipeline(drawPipeline);
        pass1.Draw(3);
        pass1.End();

        wgpu::TextureDescriptor validationTextureDesc;
        validationTextureDesc.usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        validationTextureDesc.dimension = wgpu::TextureDimension::e2D;
        validationTextureDesc.size = {kSize, kSize, 1};
        validationTextureDesc.format = wgpu::TextureFormat::RGBA8Unorm;
        validationTextureDesc.sampleCount = 1;

        wgpu::Texture validationTexture = device.CreateTexture(&validationTextureDesc);
        wgpu::TextureView validationTextureView = validationTexture.CreateView({});

        std::string validationFsCode = GetMultisampleValidationFS(format);
        wgpu::ShaderModule validationShaderModule =
            utils::CreateShaderModule(device, (GetFullScreenQuadVS() + validationFsCode).c_str());
        while (expectedDrawColorFloats.size() < 3) {
            expectedDrawColorFloats.push_back(0.0f);
        }
        if (expectedDrawColorFloats.size() < 4) {
            expectedDrawColorFloats.push_back(1.0f);
        }

        wgpu::BufferDescriptor descriptor;
        descriptor.size = 16;

        descriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        wgpu::Buffer expectedColorBuffer = device.CreateBuffer(&descriptor);
        queue.WriteBuffer(expectedColorBuffer, 0, expectedDrawColorFloats.data(), 16);

        wgpu::BindGroupLayout validationBindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::UnfilterableFloat,
                      wgpu::TextureViewDimension::e2D, true},
                     {1, wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform}});

        wgpu::BindGroup validationBindGroup =
            utils::MakeBindGroup(device, validationBindGroupLayout,
                                 {{0, multisampleColorView}, {1, expectedColorBuffer}});

        utils::ComboRenderPipelineDescriptor validationPipelineDesc;
        validationPipelineDesc.vertex.module = validationShaderModule;
        validationPipelineDesc.cFragment.module = validationShaderModule;
        validationPipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        validationPipelineDesc.cFragment.targetCount = 1;
        validationPipelineDesc.cTargets[0].format = wgpu::TextureFormat::RGBA8Unorm;
        validationPipelineDesc.multisample.count = 1;
        validationPipelineDesc.layout =
            utils::MakePipelineLayout(device, {validationBindGroupLayout});
        validationPipelineDesc.cTargets[0].writeMask = wgpu::ColorWriteMask::All;

        wgpu::RenderPipeline validationPipeline =
            device.CreateRenderPipeline(&validationPipelineDesc);

        wgpu::CommandEncoder encoder2 = device.CreateCommandEncoder();
        utils::ComboRenderPassDescriptor renderPass2;
        renderPass2.cColorAttachments[0].view = validationTextureView;
        renderPass2.cColorAttachments[0].loadOp = wgpu::LoadOp::Clear;
        renderPass2.cColorAttachments[0].storeOp = wgpu::StoreOp::Store;
        renderPass2.cColorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 0.0f};
        renderPass2.colorAttachmentCount = 1;

        wgpu::RenderPassEncoder pass2 = encoder2.BeginRenderPass(&renderPass2);
        pass2.SetPipeline(validationPipeline);
        pass2.SetBindGroup(0, validationBindGroup);
        pass2.Draw(3);
        pass2.End();

        wgpu::CommandBuffer commands1 = encoder1.Finish();
        wgpu::CommandBuffer commands2 = encoder2.Finish();
        queue.Submit(1, &commands1);
        queue.Submit(1, &commands2);

        EXPECT_PIXEL_RGBA8_EQ(utils::RGBA8::kRed, validationTexture, 0, 0);
    }
};

// Test that r16unorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, R16UnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::R16Unorm, {0.5f});
}

// Test that r16snorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, R16SnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::R16Snorm, {0.25f});
}

// Test that rg16unorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, RG16UnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::RG16Unorm, {0.25f, 0.75f});
}

// Test that rg16snorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, RG16SnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::RG16Snorm, {-0.5f, 0.5f});
}

// Test that rgba16unorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, RGBA16UnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::RGBA16Unorm, {0.1f, 0.4f, 0.7f, 1.0f});
}

// Test that rgba16snorm format has multisample capability if 'texture-formats-tier1' is enabled.
TEST_P(MultisampleFormatsTest, RGBA16SnormMultisample) {
    RunMultisampleFormatsTest(wgpu::TextureFormat::RGBA16Snorm, {-0.8f, -0.2f, 0.3f, 0.9f});
}

DAWN_INSTANTIATE_TEST(MultisampleFormatsTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend(),
                      OpenGLBackend());

}  // anonymous namespace
}  // namespace dawn
