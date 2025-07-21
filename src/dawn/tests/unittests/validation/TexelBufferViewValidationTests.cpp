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

#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/WGPUHelpers.h"
#include "webgpu/webgpu_cpp.h"

namespace dawn {
namespace {

class TexelBufferViewValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();
        // TODO(crbug/382544164): Prototype texel buffer feature
        // Implement texel buffers for wire.
        DAWN_SKIP_TEST_IF(UsesWire());
    }

    wgpu::Buffer CreateTexelBuffer(uint64_t size, wgpu::BufferUsage usage) {
        wgpu::BufferDescriptor desc;
        desc.size = size;
        desc.usage = usage;
        return device.CreateBuffer(&desc);
    }

    wgpu::BindGroupLayoutDescriptor CreateTexelBufferLayoutDescriptor(
        wgpu::TexelBufferAccess access,
        wgpu::ShaderStage visibility,
        wgpu::TextureFormat format,
        uint32_t bindingArraySize = 0) {
        wgpu::TexelBufferBindingLayout layout = {};
        layout.access = access;
        layout.format = format;

        wgpu::BindGroupLayoutEntry entry = {};
        entry.binding = 0;
        entry.visibility = visibility;
        entry.bindingArraySize = bindingArraySize;
        entry.nextInChain = &layout;

        wgpu::BindGroupLayoutDescriptor desc = {};
        desc.entryCount = 1;
        desc.entries = &entry;
        return desc;
    }
};

// Valid texel buffer view creation
TEST_F(TexelBufferViewValidationTest, CreationSuccess) {
    constexpr uint64_t kSize = 4 * 4;  // 4 texels of RGBA8Uint
    wgpu::Buffer buffer =
        CreateTexelBuffer(kSize, wgpu::BufferUsage::TexelBuffer | wgpu::BufferUsage::CopySrc);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = kSize;

    buffer.CreateTexelView(&viewDesc);
}

// Format must not be undefined
TEST_F(TexelBufferViewValidationTest, UndefinedFormat) {
    wgpu::Buffer buffer = CreateTexelBuffer(256, wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::Undefined;
    viewDesc.offset = 0;
    viewDesc.size = 256;

    ASSERT_DEVICE_ERROR(buffer.CreateTexelView(&viewDesc));
}

// Offset must be aligned to the texel size
TEST_F(TexelBufferViewValidationTest, OffsetAlignment) {
    wgpu::Buffer buffer = CreateTexelBuffer(512, wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 2;  // Not aligned to 4-byte texel
    viewDesc.size = 256;

    ASSERT_DEVICE_ERROR(buffer.CreateTexelView(&viewDesc));
}

// Size must be multiple of texel size
TEST_F(TexelBufferViewValidationTest, SizeMustBeMultipleOfTexel) {
    wgpu::Buffer buffer = CreateTexelBuffer(512, wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 256;
    viewDesc.size = 3;  // Not multiple of 4-byte texel

    ASSERT_DEVICE_ERROR(buffer.CreateTexelView(&viewDesc));
}

// Range may not exceed buffer size
TEST_F(TexelBufferViewValidationTest, RangeExceedsBuffer) {
    wgpu::Buffer buffer = CreateTexelBuffer(256, wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = 512;  // Larger than buffer

    ASSERT_DEVICE_ERROR(buffer.CreateTexelView(&viewDesc));
}

// bindingArraySize > 1 is invalid.
TEST_F(TexelBufferViewValidationTest, BindingArraySize) {
    auto desc = CreateTexelBufferLayoutDescriptor(wgpu::TexelBufferAccess::ReadOnly,
                                                  wgpu::ShaderStage::Fragment,
                                                  wgpu::TextureFormat::RGBA8Uint, 2);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Vertex visibility requires read-only access.
TEST_F(TexelBufferViewValidationTest, VertexVisibilityRequiresReadOnly) {
    auto desc = CreateTexelBufferLayoutDescriptor(wgpu::TexelBufferAccess::ReadWrite,
                                                  wgpu::ShaderStage::Vertex,
                                                  wgpu::TextureFormat::RGBA8Uint);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Format must not be undefined.
TEST_F(TexelBufferViewValidationTest, UndefinedLayoutFormat) {
    auto desc = CreateTexelBufferLayoutDescriptor(wgpu::TexelBufferAccess::ReadOnly,
                                                  wgpu::ShaderStage::Fragment,
                                                  wgpu::TextureFormat::Undefined);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// BindingInitializationHelper should chain a TexelBufferBindingEntry when initialized
// with a TexelBufferView.
TEST_F(TexelBufferViewValidationTest, BindingHelperChainsTexelBufferBindingEntry) {
    constexpr uint64_t kSize = 4 * 4;
    wgpu::Buffer buffer = CreateTexelBuffer(kSize, wgpu::BufferUsage::TexelBuffer);

    wgpu::TexelBufferViewDescriptor viewDesc;
    viewDesc.format = wgpu::TextureFormat::RGBA8Uint;
    viewDesc.offset = 0;
    viewDesc.size = kSize;
    wgpu::TexelBufferView view = buffer.CreateTexelView(&viewDesc);

    utils::BindingInitializationHelper helper(0, view);
    wgpu::BindGroupEntry entry = helper.GetAsBinding();

    EXPECT_EQ(entry.buffer, nullptr);
    EXPECT_EQ(entry.textureView, nullptr);

    ASSERT_NE(entry.nextInChain, nullptr);
    EXPECT_EQ(entry.nextInChain->sType, wgpu::SType::TexelBufferBindingEntry);

    const auto* texelEntry =
        reinterpret_cast<const wgpu::TexelBufferBindingEntry*>(entry.nextInChain);
    EXPECT_EQ(texelEntry->texelBufferView.Get(), view.Get());
}

}  // anonymous namespace
}  // namespace dawn
