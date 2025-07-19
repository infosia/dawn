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

#include <vector>

#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class TexelBufferTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();
        DAWN_TEST_UNSUPPORTED_IF(!HasToggleEnabled("allow_unsafe_apis"));
        DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::TexelBuffers));
    }

    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        if (SupportsFeatures({wgpu::FeatureName::TexelBuffers})) {
            return {wgpu::FeatureName::TexelBuffers};
        }
        return {};
    }
};

// Verify a texel buffer view can be created successfully.
TEST_P(TexelBufferTests, CreateView) {
    constexpr uint64_t kSize = 256;

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = kSize;
    bufferDesc.usage =
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::TexelBuffer;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = 16;

    wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);
    EXPECT_NE(view, nullptr);
}

// Verify compute shader can load and store through a texel buffer view.
TEST_P(TexelBufferTests, LoadStore) {
    constexpr uint64_t kSize = 256;
    std::array<uint8_t, 16> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    wgpu::Buffer buffer = utils::CreateBufferFromData(
        device, data.data(), kSize,
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = 16;

    wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);

    const char* shaderSrc = R"(
        @group(0) @binding(0) var t : texel_buffer<rgba8uint, read_write>;

        @compute @workgroup_size(1)
        fn main() {
            let value = textureLoad(t, 0u);
            textureStore(t, 1u, value + vec4u(1u));
        }
    )";

    wgpu::ShaderModule module = utils::CreateShaderModule(device, shaderSrc);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = module;
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    wgpu::BindGroup bg = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, view}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.DispatchWorkgroups(1);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    std::array<uint8_t, 16> expected = {0, 1, 2, 3, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15};
    EXPECT_BUFFER_U8_RANGE_EQ(expected.data(), buffer, 0, expected.size());
}

// Verify a texel buffer view with a non-zero offset works correctly.
TEST_P(TexelBufferTests, LoadStoreWithOffset) {
    constexpr uint64_t kSize = 512;
    std::array<uint8_t, 32> data = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

    wgpu::Buffer buffer = utils::CreateBufferFromData(
        device, data.data(), kSize,
        wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 256;
    viewDesc.size = 16;

    wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);

    const char* shaderSrc = R"(
        @group(0) @binding(0) var t : texel_buffer<rgba8uint, read_write>;

        @compute @workgroup_size(1)
        fn main() {
            let value = textureLoad(t, 0u);
            textureStore(t, 1u, value + vec4u(1u));
        }
    )";

    wgpu::ShaderModule module = utils::CreateShaderModule(device, shaderSrc);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = module;
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    wgpu::BindGroup bg = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, view}});

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.DispatchWorkgroups(1);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    std::array<uint8_t, 32> expected = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                        11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22,
                                        23, 24, 24, 25, 26, 27, 28, 29, 30, 31};
    EXPECT_BUFFER_U8_RANGE_EQ(expected.data(), buffer, 0, expected.size());
}

// Verify creating a view with an unaligned offset fails validation.
TEST_P(TexelBufferTests, UnalignedOffsetFails) {
    DAWN_TEST_UNSUPPORTED_IF(HasToggleEnabled("skip_validation"));

    constexpr uint64_t kSize = 256;
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = kSize;
    bufferDesc.usage = wgpu::BufferUsage::TexelBuffer;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 1;  // Not 256-byte aligned
    viewDesc.size = 16;

    ASSERT_DEVICE_ERROR(buffer.CreateView(&viewDesc));
}

// Verify creating shader that writes to a read-only texel buffer fails.
TEST_P(TexelBufferTests, WriteToReadOnlyFails) {
    DAWN_TEST_UNSUPPORTED_IF(HasToggleEnabled("skip_validation"));

    const char* shaderSrc = R"(
        @group(0) @binding(0) var t : texel_buffer<rgba8uint, read>;

        @compute @workgroup_size(1)
        fn main() {
            textureStore(t, 0u, vec4u(0u));
        }
    )";

    ASSERT_DEVICE_ERROR(utils::CreateShaderModule(device, shaderSrc));
}

// Verify that a bind group layout requiring a particular format cannot be used
// with a texel buffer view created with a different format.
TEST_P(TexelBufferTests, BindGroupFormatMismatchFails) {
    DAWN_TEST_UNSUPPORTED_IF(HasToggleEnabled("skip_validation"));

    constexpr uint64_t kSize = 256;
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = kSize;
    bufferDesc.usage = wgpu::BufferUsage::TexelBuffer;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = 16;
    wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);

    wgpu::TexelBufferBindingLayout texelLayout = {};
    texelLayout.format = wgpu::TextureFormat::R32Float;
    texelLayout.access = wgpu::TexelBufferAccess::ReadWrite;

    wgpu::BindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = wgpu::ShaderStage::Compute;
    entry.nextInChain = &texelLayout;

    wgpu::BindGroupLayoutDescriptor bglDesc;
    bglDesc.entryCount = 1;
    bglDesc.entries = &entry;
    wgpu::BindGroupLayout bgl = device.CreateBindGroupLayout(&bglDesc);

    wgpu::TexelBufferBindingEntry tbEntry = {};
    tbEntry.texelBufferView = view;

    wgpu::BindGroupEntry bgEntry = {};
    bgEntry.binding = 0;
    bgEntry.nextInChain = &tbEntry;

    wgpu::BindGroupDescriptor bgDesc;
    bgDesc.layout = bgl;
    bgDesc.entryCount = 1;
    bgDesc.entries = &bgEntry;

    ASSERT_DEVICE_ERROR(device.CreateBindGroup(&bgDesc));
}

// Verify a pipeline layout can include a texel buffer binding.
TEST_P(TexelBufferTests, PipelineLayoutWithTexelBuffer) {
    constexpr uint64_t kSize = 256;

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = kSize;
    bufferDesc.usage = wgpu::BufferUsage::TexelBuffer;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = 16;
    wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);

    wgpu::TexelBufferBindingLayout texelLayout = {};
    texelLayout.format = wgpu::TextureFormat::RGBA8Uint;
    texelLayout.access = wgpu::TexelBufferAccess::ReadWrite;

    wgpu::BindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = wgpu::ShaderStage::Compute;
    entry.nextInChain = &texelLayout;

    wgpu::BindGroupLayoutDescriptor bglDesc;
    bglDesc.entryCount = 1;
    bglDesc.entries = &entry;
    wgpu::BindGroupLayout bgl = device.CreateBindGroupLayout(&bglDesc);

    wgpu::PipelineLayoutDescriptor plDesc;
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bgl;
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&plDesc);

    const char* shaderSrc = R"(
        @group(0) @binding(0) var t : texel_buffer<rgba8uint, read_write>;

        @compute @workgroup_size(1)
        fn main() {
            let value = textureLoad(t, 0u);
            textureStore(t, 0u, value);
        }
    )";

    wgpu::ShaderModule module = utils::CreateShaderModule(device, shaderSrc);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = module;
    csDesc.layout = pipelineLayout;
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    wgpu::TexelBufferBindingEntry tbEntry = {};
    tbEntry.texelBufferView = view;

    wgpu::BindGroupEntry bgEntry = {};
    bgEntry.binding = 0;
    bgEntry.nextInChain = &tbEntry;

    wgpu::BindGroupDescriptor bgDesc;
    bgDesc.layout = bgl;
    bgDesc.entryCount = 1;
    bgDesc.entries = &bgEntry;
    wgpu::BindGroup bg = device.CreateBindGroup(&bgDesc);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bg);
    pass.DispatchWorkgroups(1);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);
}

// Verify that texel buffer views can be created using the plain color formats
// listed in the proposal.
TEST_P(TexelBufferTests, PlainColorFormatViews) {
    constexpr uint64_t kSize = 256;

    std::vector<wgpu::TextureFormat> formats = {
        wgpu::TextureFormat::RGBA8Uint,   wgpu::TextureFormat::RGBA8Sint,
        wgpu::TextureFormat::RGBA16Uint,  wgpu::TextureFormat::RGBA16Sint,
        wgpu::TextureFormat::RGBA16Float, wgpu::TextureFormat::R32Uint,
        wgpu::TextureFormat::R32Sint,     wgpu::TextureFormat::R32Float,
        wgpu::TextureFormat::RGBA32Uint,  wgpu::TextureFormat::RGBA32Sint,
        wgpu::TextureFormat::RGBA32Float,
    };

    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.size = kSize;
    bufferDesc.usage = wgpu::BufferUsage::TexelBuffer;
    wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);

    for (wgpu::TextureFormat format : formats) {
        wgpu::TexelBufferViewDescriptor viewDesc;
        viewDesc.format = format;
        viewDesc.offset = 0;
        viewDesc.size = 16;

        wgpu::TexelBufferView view = buffer.CreateView(&viewDesc);
        EXPECT_NE(view, nullptr);
    }
}

DAWN_INSTANTIATE_TEST(TexelBufferTests,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
