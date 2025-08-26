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

// The purpose of these tests is to prevent regressions of features and limits on architectures that
// are known to have these supported capabilities.
//
// Why? Because of the nature of feature/limit support in WebGPU we can easily accidentally drop
// (regress) optional capabilities for specific devices and dawn/CTS testing machinery and even
// external webgpu clients will respond gracefully. We (dawn) never want to silently regress
// features on critical platforms. If we are regressing it should be a deliberate decision which may
// involve changes to the tests below.
//
// How? Currently it is difficult to know our features/limit support for all hardware (could even be
// driver version dependent). So here we conservatively assert that specific architectures have
// specific features/limits.
//
// When? This file may need to be updated with new conservative tests if the CQ changes hardware or
// if we have deliberately chosen to regress specific features/limits on some devices.
//
// Not an official dawn/webgpu resource but this page can provide insights into features/limits:
// https://web3dsurvey.com/webgpu

namespace dawn {
namespace {

using TierArchInfoTestBase = DawnTestWithParams<>;

class TierArchInfoTest_TieredMaxLimits : public TierArchInfoTestBase {
  protected:
    bool GetRequireUseTieredLimits() override { return true; }
    void GetRequiredLimits(const dawn::utils::ComboLimits& supported,
                           dawn::utils::ComboLimits& required) override {
        supported.UnlinkedCopyTo(&required);
    }
};

static_assert(sizeof(dawn::utils::ComboLimits) == 184,
              "ComboLimits struct has changed. Please update the list below.");
// clang-format off
#define ENUM_LIST_LIMITS                                                                \
    /*                   field name                                 limit                */ \
    ENUM_LIMIT_PROPERTY(maxTextureDimension1D,                      16384)                                   \
    ENUM_LIMIT_PROPERTY(maxTextureDimension2D,                      16384)                                   \
    ENUM_LIMIT_PROPERTY(maxTextureDimension3D,                      2048)                                    \
    ENUM_LIMIT_PROPERTY(maxTextureArrayLayers,                      2048)                                    \
    ENUM_LIMIT_PROPERTY(maxBindGroups,                              4)                                               \
    ENUM_LIMIT_PROPERTY(maxBindGroupsPlusVertexBuffers,             24)                             \
    ENUM_LIMIT_PROPERTY(maxBindingsPerBindGroup,                    1000)                                  \
    ENUM_LIMIT_PROPERTY(maxDynamicUniformBuffersPerPipelineLayout,  8)                   \
    ENUM_LIMIT_PROPERTY(maxDynamicStorageBuffersPerPipelineLayout,  4)                   \
    ENUM_LIMIT_PROPERTY(maxSampledTexturesPerShaderStage,           16)                           \
    ENUM_LIMIT_PROPERTY(maxSamplersPerShaderStage,                  16)                                  \
    ENUM_LIMIT_PROPERTY(maxStorageBuffersPerShaderStage,            10)                            \
    ENUM_LIMIT_PROPERTY(maxStorageTexturesPerShaderStage,           4)                            \
    ENUM_LIMIT_PROPERTY(maxUniformBuffersPerShaderStage,            12)                            \
    ENUM_LIMIT_PROPERTY(maxUniformBufferBindingSize,                65536)                             \
    ENUM_LIMIT_PROPERTY(maxStorageBufferBindingSize,                4294967292)                        \
    ENUM_LIMIT_PROPERTY(minUniformBufferOffsetAlignment,            256)                           \
    ENUM_LIMIT_PROPERTY(maxVertexBuffers,                           8)                                            \
    ENUM_LIMIT_PROPERTY(maxBufferSize,                              4294967296)                                      \
    ENUM_LIMIT_PROPERTY(maxVertexAttributes,                        16)                                        \
    ENUM_LIMIT_PROPERTY(maxVertexBufferArrayStride,                 2048)                               \
    ENUM_LIMIT_PROPERTY(maxInterStageShaderVariables,               28)                               \
    ENUM_LIMIT_PROPERTY(maxColorAttachments,                        8)                                         \
    ENUM_LIMIT_PROPERTY(maxColorAttachmentBytesPerSample,           32)                           \
    ENUM_LIMIT_PROPERTY(maxComputeWorkgroupStorageSize,             65536)                          \
    ENUM_LIMIT_PROPERTY(maxComputeInvocationsPerWorkgroup,          1024)                        \
    ENUM_LIMIT_PROPERTY(maxComputeWorkgroupSizeX,                   1024)                                 \
    ENUM_LIMIT_PROPERTY(maxComputeWorkgroupSizeY,                   1024)                                 \
    ENUM_LIMIT_PROPERTY(maxComputeWorkgroupSizeZ,                   64)                                   \
    ENUM_LIMIT_PROPERTY(maxComputeWorkgroupsPerDimension,           65535)                        \
    ENUM_LIMIT_PROPERTY(maxImmediateSize,                           64)                                           \
    ENUM_LIMIT_PROPERTY(maxStorageBuffersInVertexStage,             10)                             \
    ENUM_LIMIT_PROPERTY(maxStorageTexturesInVertexStage,            4)                             \
    ENUM_LIMIT_PROPERTY(maxStorageBuffersInFragmentStage,           10)                           \
    ENUM_LIMIT_PROPERTY(maxStorageTexturesInFragmentStage,          4)
// clang-format on

TEST_P(TierArchInfoTest_TieredMaxLimits, ExhaustiveTestAllFields) {
    // Swiftshader will return a lower limit than any modern device on CQ.
    DAWN_TEST_UNSUPPORTED_IF(IsSwiftshader());

    auto& curr_limits = GetAdapterLimits();
#define ENUM_LIMIT_PROPERTY(fieldName, fieldLimit) \
    EXPECT_EQ(curr_limits.fieldName, static_cast<decltype(curr_limits.fieldName)>(fieldLimit));
    ENUM_LIST_LIMITS
}

DAWN_INSTANTIATE_TEST(TierArchInfoTest_TieredMaxLimits,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
