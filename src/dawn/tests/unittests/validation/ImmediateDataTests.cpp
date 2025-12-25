// Copyright 2024 The Dawn & Tint Authors
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

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "dawn/common/NonMovable.h"
#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/ComboRenderPipelineDescriptor.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

enum class FeatureMode {
    Enabled,
    DisabledViaNotAllowUnsafeAPIs,
    DisabledViaBlocklistedFeatures,
};

// Test that the feature only works when enabled
struct ImmediateDataDisableTest : ValidationTestWithParam<FeatureMode> {
    std::vector<const char*> GetWGSLBlocklistedFeatures() override {
        switch (GetParam()) {
            case FeatureMode::Enabled:
                return {};
            case FeatureMode::DisabledViaNotAllowUnsafeAPIs:
                return {};
            case FeatureMode::DisabledViaBlocklistedFeatures:
                return {"immediate_address_space"};
        }
        DAWN_UNREACHABLE();
        return {};
    }

    bool AllowUnsafeAPIs() override {
        switch (GetParam()) {
            case FeatureMode::Enabled:
                // Currently the only way to enable ImmediateAddressSpace is via AllowUnsafeAPIs.
                // See GetLanguageFeatureStatus.
                return true;
            case FeatureMode::DisabledViaNotAllowUnsafeAPIs:
                return false;
            case FeatureMode::DisabledViaBlocklistedFeatures:
                // Enabling AllowUnsafeAPIs while disabling via blocklist should still fail.
                return true;
        }
        DAWN_UNREACHABLE();
        return false;
    }
};

// Check that creating a PipelineLayout with non-zero immediateSize is disallowed
// without the feature enabled.
TEST_P(ImmediateDataDisableTest, ImmediateSizeNotAllowed) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;
    desc.immediateSize = 1;

    if (GetParam() == FeatureMode::Enabled) {
        device.CreatePipelineLayout(&desc);
    } else {
        ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
    }
}

// Check that SetImmediates doesn't work (even with size=0) without the feature enabled.
TEST_P(ImmediateDataDisableTest, SetImmediates) {
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetImmediates(0, nullptr, 0);
        pass.End();
        if (GetParam() == FeatureMode::Enabled) {
            encoder.Finish();
        } else {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }
    {
        const uint32_t data = 0;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetImmediates(0, &data, 4);
        pass.End();
        if (GetParam() == FeatureMode::Enabled) {
            encoder.Finish();
        } else {
            ASSERT_DEVICE_ERROR(encoder.Finish());
        }
    }
}

// Check that limits.maxImmediateSize is 0 when the feature is disabled, and kMaxImmediateDataBytes
// otherwise.
TEST_P(ImmediateDataDisableTest, MaxImmediateSizeIsZero) {
    if (GetParam() == FeatureMode::Enabled) {
        ASSERT_EQ(GetSupportedLimits().maxImmediateSize, kMaxImmediateDataBytes);
    } else {
        ASSERT_EQ(GetSupportedLimits().maxImmediateSize, 0u);
    }
}

INSTANTIATE_TEST_SUITE_P(,
                         ImmediateDataDisableTest,
                         ::testing::ValuesIn({FeatureMode::Enabled,
                                              FeatureMode::DisabledViaNotAllowUnsafeAPIs,
                                              FeatureMode::DisabledViaBlocklistedFeatures}));

class ImmediateDataTest : public ValidationTest {
  protected:
    wgpu::BindGroupLayout CreateBindGroupLayout() {
        wgpu::BindGroupLayoutEntry entries[1];
        entries[0].binding = 0;
        entries[0].visibility = wgpu::ShaderStage::Compute;
        entries[0].buffer.type = wgpu::BufferBindingType::Storage;

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = entries;

        return device.CreateBindGroupLayout(&bindGroupLayoutDesc);
    }

    wgpu::PipelineLayout CreatePipelineLayout(uint32_t requiredImmediateSize) {
        wgpu::BindGroupLayout bindGroupLayout = CreateBindGroupLayout();

        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        pipelineLayoutDesc.immediateSize = requiredImmediateSize;
        return device.CreatePipelineLayout(&pipelineLayoutDesc);
    }

    wgpu::ShaderModule mShaderModule;
};

// Check that non-zero immediateSize is possible with feature enabled and size must
// below max size limits.
TEST_F(ImmediateDataTest, ValidateImmediateSize) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;

    // Success case with valid immediateSize.
    {
        desc.immediateSize = kMaxImmediateDataBytes;
        device.CreatePipelineLayout(&desc);
    }

    // Failed case with invalid immediateSize that exceed limits.
    {
        desc.immediateSize = kMaxImmediateDataBytes + 1;
        ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
    }
}

struct SetImmediatesCase {
    uint32_t offset;
    uint32_t size;
    bool success;
};

class ImmediateDataSetImmediatesValidationTest
    : public ImmediateDataTest,
      public testing::WithParamInterface<SetImmediatesCase> {};

TEST_P(ImmediateDataSetImmediatesValidationTest, Test) {
    const auto& param = GetParam();
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();

    if (param.size > 0) {
        std::vector<uint8_t> data(param.size, 0);
        computePass.SetImmediates(param.offset, data.data(), param.size);
    } else {
        computePass.SetImmediates(param.offset, nullptr, 0);
    }
    computePass.End();

    if (param.success) {
        encoder.Finish();
    } else {
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ImmediateDataSetImmediatesValidationTest,
    ::testing::Values(
        // Alignment tests
        SetImmediatesCase{0, 4, true},   // Success: aligned offset and size
        SetImmediatesCase{4, 0, true},   // Success: aligned offset and 0 size
        SetImmediatesCase{2, 0, false},  // Failure: unaligned offset
        SetImmediatesCase{0, 2, false},  // Failure: unaligned size
        // OOB tests
        SetImmediatesCase{0, kMaxImmediateDataBytes, true},       // Success: full range
        SetImmediatesCase{kMaxImmediateDataBytes, 0, true},       // Success: empty at end
        SetImmediatesCase{kMaxImmediateDataBytes - 4, 4, true},   // Success: last 4 bytes
        SetImmediatesCase{kMaxImmediateDataBytes + 4, 0, false},  // Failure: offset OOB
        SetImmediatesCase{0, kMaxImmediateDataBytes + 4, false},  // Failure: size OOB
        SetImmediatesCase{kMaxImmediateDataBytes, 4, false},      // Failure: offset + size OOB
        SetImmediatesCase{std::numeric_limits<uint32_t>::max() - 3, 4,
                          false}));  // Failure: overflow

// Check that pipelineLayout immediate data bytes compatible with shaders.
TEST_F(ImmediateDataTest, ValidatePipelineLayoutImmediateDataBytesAndShaders) {
    constexpr uint32_t kShaderImmediateDataBytes = 12u;
    wgpu::ShaderModule shaderModule = utils::CreateShaderModule(device, R"(
        var<immediate> fragmentConstants: vec3f;
        var<immediate> computeConstants: vec3u;
        @vertex fn vsMain(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
            const pos = array(
                vec2( 1.0, -1.0),
                vec2(-1.0, -1.0),
                vec2( 0.0,  1.0),
            );
            return vec4(pos[VertexIndex], 0.0, 1.0);
        }

        // to reuse the same pipeline layout
        @fragment fn fsMain() -> @location(0) vec4f {
            return vec4f(fragmentConstants, 1.0);
        }


        @group(0) @binding(0) var<storage, read_write> output : vec3u;

        @compute @workgroup_size(1, 1, 1)
        fn csMain() {
            output = computeConstants;
        })");

    // Success cases
    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = shaderModule;
        pipelineDescriptor.cFragment.module = shaderModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        pipelineDescriptor.layout = CreatePipelineLayout(kShaderImmediateDataBytes);
        device.CreateRenderPipeline(&pipelineDescriptor);
    }

    {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = shaderModule;
        csDesc.layout = CreatePipelineLayout(kShaderImmediateDataBytes);

        device.CreateComputePipeline(&csDesc);
    }

    // Default layout
    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = shaderModule;
        pipelineDescriptor.cFragment.module = shaderModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        device.CreateRenderPipeline(&pipelineDescriptor);
    }

    {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = shaderModule;

        device.CreateComputePipeline(&csDesc);
    }

    // Failed case with fragment shader requires more immediate data.
    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = shaderModule;
        pipelineDescriptor.cFragment.module = shaderModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        pipelineDescriptor.layout = CreatePipelineLayout(kShaderImmediateDataBytes - 4);
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&pipelineDescriptor));
    }

    // Failed cases with compute shader requires more immediate data.
    {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = shaderModule;
        csDesc.layout = CreatePipelineLayout(kShaderImmediateDataBytes - 4);

        ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&csDesc));
    }
}

// Check that default pipelineLayout has too many immediate data bytes .
TEST_F(ImmediateDataTest, ValidateDefaultPipelineLayout) {
    wgpu::ShaderModule shaderModule = utils::CreateShaderModule(device, R"(
            var<immediate> fragmentConstants: array<vec4f, 4>;
            var<immediate> computeConstants: array<vec4u, 4>;
            @vertex fn vsMain(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
                const pos = array(
                    vec2( 1.0, -1.0),
                    vec2(-1.0, -1.0),
                    vec2( 0.0,  1.0),
                );
                return vec4(pos[VertexIndex], 0.0, 1.0);
            }

            // to reuse the same pipeline layout
            @fragment fn fsMain() -> @location(0) vec4f {
                return vec4f(fragmentConstants[0].x, fragmentConstants[0].yzw);
            }

            @group(0) @binding(0) var<storage, read_write> output : vec4u;

            @compute @workgroup_size(1, 1, 1)
            fn csMain() {
                output = vec4u(computeConstants[0].x, computeConstants[0].yzw);
            })");

    wgpu::ShaderModule oobShaderModule = utils::CreateShaderModule(device, R"(
            struct FragmentConstants {
                constants: array<vec4f, 4>,
                constantsOOB: f32,
            };

            struct ComputeConstants {
                constants: array<vec4u, 4>,
                constantsOOB: u32,
            };
            var<immediate> fragmentConstants: FragmentConstants;
            var<immediate> computeConstants: ComputeConstants;
            @vertex fn vsMain(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
                const pos = array(
                    vec2( 1.0, -1.0),
                    vec2(-1.0, -1.0),
                    vec2( 0.0,  1.0),
                );
                return vec4(pos[VertexIndex], 0.0, 1.0);
            }

            // to reuse the same pipeline layout
            @fragment fn fsMain() -> @location(0) vec4f {
                return vec4f(fragmentConstants.constants[0].x + fragmentConstants.constantsOOB,
                             fragmentConstants.constants[0].yzw);
            }

            @group(0) @binding(0) var<storage, read_write> output : vec4u;

            @compute @workgroup_size(1, 1, 1)
            fn csMain() {
                output = vec4u(computeConstants.constants[0].x + computeConstants.constantsOOB,
                               computeConstants.constants[0].yzw);
            })");

    // Success cases
    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = shaderModule;
        pipelineDescriptor.cFragment.module = shaderModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        device.CreateRenderPipeline(&pipelineDescriptor);
    }

    {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = shaderModule;

        device.CreateComputePipeline(&csDesc);
    }

    // Using too many immediate data cases
    {
        utils::ComboRenderPipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.vertex.module = oobShaderModule;
        pipelineDescriptor.cFragment.module = oobShaderModule;
        pipelineDescriptor.cFragment.targetCount = 1;
        ASSERT_DEVICE_ERROR(device.CreateRenderPipeline(&pipelineDescriptor));
    }

    {
        wgpu::ComputePipelineDescriptor csDesc;
        csDesc.compute.module = oobShaderModule;

        ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&csDesc));
    }
}

struct ImmediateDataRequiredCase {
    const char* shader;
    uint32_t setOffset;
    uint32_t setSize;
    bool success;
};

class ImmediateDataRequiredTest : public ImmediateDataTest,
                                  public testing::WithParamInterface<ImmediateDataRequiredCase> {};

TEST_P(ImmediateDataRequiredTest, Test) {
    const auto& param = GetParam();
    wgpu::ShaderModule shaderModule = utils::CreateShaderModule(device, param.shader);

    wgpu::ComputePipelineDescriptor descriptor;
    descriptor.compute.module = shaderModule;
    descriptor.compute.entryPoint = "main";
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&descriptor);

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);

    if (param.setSize > 0) {
        std::vector<uint32_t> data(param.setSize / 4, 0);
        pass.SetImmediates(param.setOffset, data.data(), param.setSize);
    }

    pass.DispatchWorkgroups(1);
    pass.End();

    if (param.success) {
        encoder.Finish();
    } else {
        ASSERT_DEVICE_ERROR(encoder.Finish());
    }
}

static const char* kShader1 = R"(
        struct ImmediateData {
            val : u32
        }
        var<immediate> data : ImmediateData;

        @compute @workgroup_size(1)
        fn main() {
            _ = data.val;
        }
    )";

static const char* kShader2 = R"(
        struct ImmediateData {
            val1 : u32,
            val2 : u32,
        }
        var<immediate> data : ImmediateData;

        @compute @workgroup_size(1)
        fn main() {
            _ = data.val1;
            _ = data.val2;
        }
    )";

INSTANTIATE_TEST_SUITE_P(,
                         ImmediateDataRequiredTest,
                         ::testing::Values(
                             // Shader requires 4 bytes, but 0 provided.
                             ImmediateDataRequiredCase{kShader1, 0, 0, false},
                             // Shader requires 4 bytes, and 4 provided.
                             ImmediateDataRequiredCase{kShader1, 0, 4, true},
                             // Shader requires 8 bytes, but 4 provided.
                             ImmediateDataRequiredCase{kShader2, 0, 4, false},
                             // Shader requires 8 bytes, and 8 provided.
                             ImmediateDataRequiredCase{kShader2, 0, 8, true}));

}  // anonymous namespace
}  // namespace dawn
