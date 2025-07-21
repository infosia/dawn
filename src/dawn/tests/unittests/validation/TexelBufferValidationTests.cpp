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

#include "dawn/tests/unittests/validation/ValidationTest.h"

namespace dawn {
namespace {

class TexelBufferValidationTest : public ValidationTest {
  protected:
    void SetUp() override {
        ValidationTest::SetUp();
        DAWN_SKIP_TEST_IF(UsesWire());
    }

    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::TexelBuffers};
    }
};

// Creation succeeds when the feature is enabled.
TEST_F(TexelBufferValidationTest, CreationSuccess) {
    wgpu::BufferDescriptor desc;
    desc.size = 4;
    desc.usage = wgpu::BufferUsage::TexelBuffer;
    device.CreateBuffer(&desc);
}

// Mappable usages require BufferMapExtendedUsages when combined with TexelBuffer.
TEST_F(TexelBufferValidationTest, MappableUsageRequiresExtendedFeature) {
    wgpu::BufferDescriptor desc;
    desc.size = 4;
    desc.usage = wgpu::BufferUsage::TexelBuffer | wgpu::BufferUsage::MapRead;
    ASSERT_DEVICE_ERROR(device.CreateBuffer(&desc));

    desc.usage = wgpu::BufferUsage::TexelBuffer | wgpu::BufferUsage::MapWrite;
    ASSERT_DEVICE_ERROR(device.CreateBuffer(&desc));
}

class TexelBufferFeatureDisabledTest : public TexelBufferValidationTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override { return {}; }
};

// Creating a TexelBuffer buffer requires the TexelBuffers feature to be enabled.
TEST_F(TexelBufferFeatureDisabledTest, RequiresFeature) {
    wgpu::BufferDescriptor desc;
    desc.size = 4;
    desc.usage = wgpu::BufferUsage::TexelBuffer;

    ASSERT_DEVICE_ERROR(device.CreateBuffer(&desc));
}

}  // anonymous namespace
}  // namespace dawn
