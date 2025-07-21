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

namespace dawn {
namespace {

class TexelBufferLayoutValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();
        // TODO(crbug/382544164): Prototype texel buffer feature
        // Implement texel buffers for wire.
        DAWN_SKIP_TEST_IF(UsesWire());
    }
};

wgpu::BindGroupLayoutDescriptor CreateDescriptor(wgpu::TexelBufferAccess access,
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

// Creation succeeds with valid parameters.
TEST_F(TexelBufferLayoutValidationTest, CreationSuccess) {
    wgpu::TexelBufferBindingLayout layout = {};
    layout.access = wgpu::TexelBufferAccess::ReadOnly;
    layout.format = wgpu::TextureFormat::RGBA8Uint;

    wgpu::BindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = wgpu::ShaderStage::Fragment;
    entry.nextInChain = &layout;

    wgpu::BindGroupLayoutDescriptor desc = {};
    desc.entryCount = 1;
    desc.entries = &entry;

    device.CreateBindGroupLayout(&desc);
}

// bindingArraySize > 1 is invalid.
TEST_F(TexelBufferLayoutValidationTest, BindingArraySize) {
    auto desc = CreateDescriptor(wgpu::TexelBufferAccess::ReadOnly, wgpu::ShaderStage::Fragment,
                                 wgpu::TextureFormat::RGBA8Uint, 2);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Vertex visibility requires read-only access.
TEST_F(TexelBufferLayoutValidationTest, VertexVisibilityRequiresReadOnly) {
    auto desc = CreateDescriptor(wgpu::TexelBufferAccess::ReadWrite, wgpu::ShaderStage::Vertex,
                                 wgpu::TextureFormat::RGBA8Uint);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

// Format must not be undefined.
TEST_F(TexelBufferLayoutValidationTest, UndefinedFormat) {
    auto desc = CreateDescriptor(wgpu::TexelBufferAccess::ReadOnly, wgpu::ShaderStage::Fragment,
                                 wgpu::TextureFormat::Undefined);
    ASSERT_DEVICE_ERROR(device.CreateBindGroupLayout(&desc));
}

}  // anonymous namespace
}  // namespace dawn
