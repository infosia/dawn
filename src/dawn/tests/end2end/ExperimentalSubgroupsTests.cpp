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

#include <string>
#include <vector>

#include "dawn/common/Math.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class ExperimentalSubgroupsTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        // Always require ChromiumExperimentalSubgroups feature if available.
        if (SupportsFeatures({wgpu::FeatureName::ChromiumExperimentalSubgroups})) {
            mRequiredSubgroupsFeature = true;
            return {wgpu::FeatureName::ChromiumExperimentalSubgroups};
        }

        return {};
    }

    void TestSubgroupBroadcast(uint32_t workgroupSize, std::string broadcastType) {
        DAWN_TEST_UNSUPPORTED_IF(!IsSubgroupsFeatureRequired());

        std::stringstream shaderCode;
        shaderCode << R"(
enable chromium_experimental_subgroups;

const workgroupSize = )"
                   << workgroupSize << R"(u;

@group(0) @binding(0) var<storage, read_write> output : array<u32, workgroupSize>;

@compute @workgroup_size(workgroupSize, 1, 1)
fn main(@builtin(local_invocation_id) local_id : vec3u) {
    output[local_id.x] = u32(subgroupBroadcast()"
                   << broadcastType << R"((1), 0u));
}
)";

        auto shaderModule = utils::CreateShaderModule(device, shaderCode.str().c_str());

        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = shaderModule;

        device.CreateComputePipeline(&csDesc);
    }

    // Helper function that create shader module with subgroups extension required and a empty
    // compute entry point, named main, of given workgroup size
    wgpu::ShaderModule CreateShaderModuleWithSubgroupsRequired(  //
        WGPUExtent3D workgroupSize = {1, 1, 1}) {
        std::stringstream code;
        code << R"(
        enable chromium_experimental_subgroups;

        @compute @workgroup_size()"
             << workgroupSize.width << ", " << workgroupSize.height << ", "
             << workgroupSize.depthOrArrayLayers << R"()
        fn main() {}
)";
        return utils::CreateShaderModule(device, code.str().c_str());
    }

    // Helper function that create shader module with subgroups extension required and a empty
    // compute entry point, named main, of workgroup size that are override constants.
    wgpu::ShaderModule CreateShaderModuleWithOverrideWorkgroupSize() {
        std::stringstream code;
        code << R"(
        enable chromium_experimental_subgroups;

        override wgs_x: u32;
        override wgs_y: u32;
        override wgs_z: u32;

        @compute @workgroup_size(wgs_x, wgs_y, wgs_z)
        fn main() {}
)";
        return utils::CreateShaderModule(device, code.str().c_str());
    }

    struct TestCase {
        WGPUExtent3D workgroupSize;
        bool isFullSubgroups;
    };

    // Helper function that generate workgroup size cases for full subgroups test, based on device
    // reported max subgroup size.
    std::vector<TestCase> GenerateFullSubgroupsWorkgroupSizeCases() {
        wgpu::SupportedLimits limits{};
        wgpu::DawnExperimentalSubgroupLimits subgroupLimits{};
        limits.nextInChain = &subgroupLimits;
        EXPECT_EQ(device.GetLimits(&limits), wgpu::Status::Success);
        uint32_t maxSubgroupSize = subgroupLimits.maxSubgroupSize;
        EXPECT_TRUE(1 <= maxSubgroupSize && maxSubgroupSize <= 128);
        // maxSubgroupSize should be a power of 2.
        EXPECT_TRUE(IsPowerOfTwo(maxSubgroupSize));

        std::vector<TestCase> cases;

        // workgroup_size.x = maxSubgroupSize, is a multiple of maxSubgroupSize.
        cases.push_back({{maxSubgroupSize, 1, 1}, true});
        // Note that maxSubgroupSize is no larger than 128, so threads in the wrokgroups below is no
        // more than 256, fits in the maxComputeInvocationsPerWorkgroup limit which is at least 256.
        cases.push_back({{maxSubgroupSize * 2, 1, 1}, true});
        cases.push_back({{maxSubgroupSize, 2, 1}, true});
        cases.push_back({{maxSubgroupSize, 1, 2}, true});

        EXPECT_TRUE(maxSubgroupSize >= 4);
        // workgroup_size.x = maxSubgroupSize / 2, not a multiple of maxSubgroupSize.
        cases.push_back({{maxSubgroupSize / 2, 1, 1}, false});
        cases.push_back({{maxSubgroupSize / 2, 2, 1}, false});
        // workgroup_size.x = maxSubgroupSize - 1, not a multiple of maxSubgroupSize.
        cases.push_back({{maxSubgroupSize - 1, 1, 1}, false});
        // workgroup_size.x = maxSubgroupSize * 2 - 1, not a multiple of maxSubgroupSize if
        // maxSubgroupSize > 1.
        cases.push_back({{maxSubgroupSize * 2 - 1, 1, 1}, false});
        // workgroup_size.x = 1, not a multiple of maxSubgroupSize. Test that validation
        // checks the x dimension of workgroup size instead of others.
        cases.push_back({{1, maxSubgroupSize, 1}, false});

        return cases;
    }

    bool IsSubgroupsFeatureRequired() const { return mRequiredSubgroupsFeature; }

  private:
    bool mRequiredSubgroupsFeature = false;
};

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize16) {
    TestSubgroupBroadcast(16, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize16) {
    TestSubgroupBroadcast(16, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize16) {
    TestSubgroupBroadcast(16, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize15) {
    TestSubgroupBroadcast(15, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize15) {
    TestSubgroupBroadcast(15, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize15) {
    TestSubgroupBroadcast(15, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize14) {
    TestSubgroupBroadcast(14, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize14) {
    TestSubgroupBroadcast(14, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize14) {
    TestSubgroupBroadcast(14, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize13) {
    TestSubgroupBroadcast(13, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize13) {
    TestSubgroupBroadcast(13, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize13) {
    TestSubgroupBroadcast(13, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize12) {
    TestSubgroupBroadcast(12, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize12) {
    TestSubgroupBroadcast(12, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize12) {
    TestSubgroupBroadcast(12, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize11) {
    TestSubgroupBroadcast(11, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize11) {
    TestSubgroupBroadcast(11, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize11) {
    TestSubgroupBroadcast(11, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize10) {
    TestSubgroupBroadcast(10, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize10) {
    TestSubgroupBroadcast(10, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize10) {
    TestSubgroupBroadcast(10, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize9) {
    TestSubgroupBroadcast(9, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize9) {
    TestSubgroupBroadcast(9, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize9) {
    TestSubgroupBroadcast(9, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize8) {
    TestSubgroupBroadcast(8, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize8) {
    TestSubgroupBroadcast(8, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize8) {
    TestSubgroupBroadcast(8, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize7) {
    TestSubgroupBroadcast(7, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize7) {
    TestSubgroupBroadcast(7, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize7) {
    TestSubgroupBroadcast(7, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize6) {
    TestSubgroupBroadcast(6, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize6) {
    TestSubgroupBroadcast(6, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize6) {
    TestSubgroupBroadcast(6, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize5) {
    TestSubgroupBroadcast(5, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize5) {
    TestSubgroupBroadcast(5, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize5) {
    TestSubgroupBroadcast(5, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize4) {
    TestSubgroupBroadcast(4, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize4) {
    TestSubgroupBroadcast(4, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize4) {
    TestSubgroupBroadcast(4, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize3) {
    TestSubgroupBroadcast(3, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize3) {
    TestSubgroupBroadcast(3, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize3) {
    TestSubgroupBroadcast(3, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize2) {
    TestSubgroupBroadcast(2, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize2) {
    TestSubgroupBroadcast(2, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize2) {
    TestSubgroupBroadcast(2, "f32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstI32WithWorkgroupSize1) {
    TestSubgroupBroadcast(1, "i32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstU32WithWorkgroupSize1) {
    TestSubgroupBroadcast(1, "u32");
}

TEST_P(ExperimentalSubgroupsTests, CreatePipelineSubgroupBroadcastConstF32WithWorkgroupSize1) {
    TestSubgroupBroadcast(1, "f32");
}

// Test that creating compute pipeline with full subgroups required will validate the workgroup size
// as expected, when using compute shader with literal workgroup size.
TEST_P(ExperimentalSubgroupsTests, ComputePipelineRequiringFullSubgroupsWithLiteralWorkgroupSize) {
    if (!IsSubgroupsFeatureRequired()) {
        return;
    }

    // Keep all success compute pipeline alive, so that we can test the compute pipeline cache.
    std::vector<wgpu::ComputePipeline> computePipelines;

    for (const TestCase& c : GenerateFullSubgroupsWorkgroupSizeCases()) {
        // Reuse the shader module for both not requiring and requiring full subgroups cases, to
        // test that cached compute pipeline will not be used unexpectly.
        auto shaderModule = CreateShaderModuleWithSubgroupsRequired(c.workgroupSize);
        for (bool requiresFullSubgroups : {false, true}) {
            wgpu::ComputePipelineDescriptor csDesc;
            csDesc.compute.module = shaderModule;

            wgpu::DawnComputePipelineFullSubgroups fullSubgroupsOption;
            fullSubgroupsOption.requiresFullSubgroups = requiresFullSubgroups;
            csDesc.nextInChain = &fullSubgroupsOption;

            // It should be a validation error if full subgroups is required but given workgroup
            // size does not fit.
            if (requiresFullSubgroups && !c.isFullSubgroups) {
                ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&csDesc));
            } else {
                // Otherwise, creating compute pipeline should succeed.
                computePipelines.push_back(device.CreateComputePipeline(&csDesc));
            }
        }
    }
}
// Test that creating compute pipeline with full subgroups required will validate the workgroup size
// as expected, when using compute shader with override constants workgroup size.
TEST_P(ExperimentalSubgroupsTests, ComputePipelineRequiringFullSubgroupsWithOverrideWorkgroupSize) {
    if (!IsSubgroupsFeatureRequired()) {
        return;
    }
    // Reuse the same shader module for all case to test the validation happened as expected.
    auto shaderModule = CreateShaderModuleWithOverrideWorkgroupSize();
    // Keep all success compute pipeline alive, so that we can test the compute pipeline cache.
    std::vector<wgpu::ComputePipeline> computePipelines;

    for (const TestCase& c : GenerateFullSubgroupsWorkgroupSizeCases()) {
        for (bool requiresFullSubgroups : {false, true}) {
            std::vector<wgpu::ConstantEntry> constants{
                {nullptr, "wgs_x", static_cast<double>(c.workgroupSize.width)},
                {nullptr, "wgs_y", static_cast<double>(c.workgroupSize.height)},
                {nullptr, "wgs_z", static_cast<double>(c.workgroupSize.depthOrArrayLayers)},
            };

            wgpu::ComputePipelineDescriptor csDesc;
            csDesc.compute.module = shaderModule;
            csDesc.compute.constants = constants.data();
            csDesc.compute.constantCount = constants.size();

            wgpu::DawnComputePipelineFullSubgroups fullSubgroupsOption;
            fullSubgroupsOption.requiresFullSubgroups = requiresFullSubgroups;
            csDesc.nextInChain = &fullSubgroupsOption;

            // It should be a validation error if full subgroups is required but given workgroup
            // size does not fit.
            if (requiresFullSubgroups && !c.isFullSubgroups) {
                ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&csDesc));
            } else {
                // Otherwise, creating compute pipeline should succeed.
                computePipelines.push_back(device.CreateComputePipeline(&csDesc));
            }
        }
    }
}

// DawnTestBase::CreateDeviceImpl always enables allow_unsafe_apis toggle.
DAWN_INSTANTIATE_TEST(ExperimentalSubgroupsTests,
                      D3D12Backend(),
                      D3D12Backend({}, {"use_dxc"}),
                      MetalBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
