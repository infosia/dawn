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

#include "dawn/common/GPUInfo.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

using FeatureArchInfoTestBase = DawnTestWithParams<>;

TEST_P(FeatureArchInfoTestBase, SubgroupsSupported) {
    const bool subgroupSupportExpected = gpu_info::IsApple(GetParam().adapterProperties.vendorID);

    DAWN_TEST_UNSUPPORTED_IF(!subgroupSupportExpected);
    EXPECT_TRUE(this->SupportsFeatures({wgpu::FeatureName::Subgroups}));
}

class FeatureArchInfoTest_MaxLimits : public FeatureArchInfoTestBase {
  protected:
    void GetRequiredLimits(const dawn::utils::ComboLimits& supported,
                           dawn::utils::ComboLimits& required) override {
        supported.UnlinkedCopyTo(&required);
    }
};

TEST_P(FeatureArchInfoTest_MaxLimits, WorkgroupSizeMin1024Expected) {
    // All apple silicon devices are know to have 1k workgroup size limit.
    // This code will need to be changed if a new apple silicon device is release with a different
    // limit
    const bool isApple = gpu_info::IsApple(GetParam().adapterProperties.vendorID);
    DAWN_TEST_UNSUPPORTED_IF(!isApple);

    EXPECT_GE(GetAdapterLimits().maxComputeInvocationsPerWorkgroup, 1024u);
    // Check that the device was created with the requested limit.
    EXPECT_EQ(GetSupportedLimits().maxComputeInvocationsPerWorkgroup,
              GetAdapterLimits().maxComputeInvocationsPerWorkgroup);
}

class FeatureArchInfoTest_TieredMaxLimits : public FeatureArchInfoTestBase {
  protected:
    bool GetRequireUseTieredLimits() override { return true; }
    void GetRequiredLimits(const dawn::utils::ComboLimits& supported,
                           dawn::utils::ComboLimits& required) override {
        supported.UnlinkedCopyTo(&required);
    }
};

TEST_P(FeatureArchInfoTest_TieredMaxLimits, D3DHighMaxVertexAttributes) {
    const bool isWindowsHighEnd =
        gpu_info::IsNvidia(GetParam().adapterProperties.vendorID) && (IsD3D11() || IsD3D12());
    DAWN_TEST_UNSUPPORTED_IF(!isWindowsHighEnd);

    // High-end windows desktop GPU should report at least 30 even when tiered is enabled
    // See crbug.com/430371785
    EXPECT_GE(GetAdapterLimits().maxVertexAttributes, 30u);
}

DAWN_INSTANTIATE_TEST(FeatureArchInfoTestBase,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

DAWN_INSTANTIATE_TEST(FeatureArchInfoTest_MaxLimits,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

DAWN_INSTANTIATE_TEST(FeatureArchInfoTest_TieredMaxLimits,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
