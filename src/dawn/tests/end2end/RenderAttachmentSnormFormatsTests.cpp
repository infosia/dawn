#include <webgpu/webgpu_cpp.h>
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

#include <vector>

namespace dawn {
namespace {

class RenderAttachmentSnormFormatsTest : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::TextureFormatsTier1};
    }

    const char* GetSinglePointVS() {
        return R"(
            @vertex
            fn main() -> @builtin(position) vec4<f32> {
                return vec4<f32>(0.0, 0.0, 0.0, 1.0); 
            }
        )";
    }
    void RunSingleFormatTest(wgpu::TextureFormat format,
                             const char* fsCode,
                             const std::vector<int8_t>& expectedData) {
        wgpu::Extent3D textureSize = {1, 1, 1};
        wgpu::TextureDescriptor textureDesc;
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = textureSize;
        textureDesc.format = format;

        wgpu::Texture texture = device.CreateTexture(&textureDesc);

        wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, GetSinglePointVS());
        wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, fsCode);

        utils::ComboRenderPipelineDescriptor pipelineDesc;
        pipelineDesc.vertex.module = vsModule;
        pipelineDesc.cFragment.module = fsModule;
        pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::PointList;
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
        pass.Draw(1);
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        EXPECT_PIXEL_SNORM_EQ(expectedData.data(), texture, {0, 0}, {1, 1}, format);
    }
};

// Test that r8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentSnormFormatsTest, R8SnormRenderAttachment) {
    const char* fs_r8snorm = R"(
        @fragment
        fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(-0.5, 0.0, 0.0, 1.0); 
        }
    )";

    std::vector<int8_t> expectedData = {static_cast<int8_t>(0xC0)};  // -64

    RunSingleFormatTest(wgpu::TextureFormat::R8Snorm, fs_r8snorm, expectedData);
}

// Test that rg8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentSnormFormatsTest, RG8SnormRenderAttachment) {
    const char* fs_rg8snorm = R"(
        @fragment
        fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(-0.5, 0.25, 0.0, 1.0); 
        }
    )";

    std::vector<int8_t> expectedData = {static_cast<int8_t>(0xC0), static_cast<int8_t>(0x20)};

    RunSingleFormatTest(wgpu::TextureFormat::RG8Snorm, fs_rg8snorm, expectedData);
}

// Test that r8snorm format is valid as renderable texture if
// 'texture-formats-tier1' is enabled.
TEST_P(RenderAttachmentSnormFormatsTest, RGBA8SnormRenderAttachment) {
    const char* fs_rgba8snorm = R"(
        @fragment
        fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(-0.5, 0.25, -0.75, 1.0); 
        }
    )";
    std::vector<int8_t> expectedData = {static_cast<int8_t>(0xC0), static_cast<int8_t>(0x20),
                                        static_cast<int8_t>(0xA1), static_cast<int8_t>(0x7F)};
    RunSingleFormatTest(wgpu::TextureFormat::RGBA8Snorm, fs_rgba8snorm, expectedData);
}

DAWN_INSTANTIATE_TEST(RenderAttachmentSnormFormatsTest, D3D12Backend(), MetalBackend());

}  // anonymous namespace
}  // namespace dawn