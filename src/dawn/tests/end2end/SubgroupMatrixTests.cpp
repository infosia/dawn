// Copyright 2023 The Dawn & Tint Authors
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

namespace dawn {
namespace {

class SubgroupMatrixTestBase : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        if (SupportsFeatures({wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix})) {
            return {wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix};
        }
        return {};
    }
};

// Test that it is only valid to request the AdapterPropertiesSubgroupMatrixConfigs structure if the
// feature is available.
TEST_P(SubgroupMatrixTestBase, QueryConfigsOnlyValidWithFeature) {
    auto expected = adapter.HasFeature(wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix)
                        ? wgpu::Status::Success
                        : wgpu::Status::Error;
    {
        wgpu::AdapterInfo info;
        wgpu::AdapterPropertiesSubgroupMatrixConfigs subgroupMatrixConfigs;
        info.nextInChain = &subgroupMatrixConfigs;

        EXPECT_EQ(adapter.GetInfo(&info), expected);
    }
    {
        wgpu::AdapterInfo adapterInfo;
        wgpu::AdapterPropertiesSubgroupMatrixConfigs subgroupMatrixConfigs;
        adapterInfo.nextInChain = &subgroupMatrixConfigs;

        EXPECT_EQ(device.GetAdapterInfo(&adapterInfo), expected);
    }
}

DAWN_INSTANTIATE_TEST(SubgroupMatrixTestBase, D3D12Backend(), MetalBackend(), VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
