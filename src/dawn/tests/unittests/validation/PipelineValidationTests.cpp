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

namespace dawn {
namespace {

class PipelineValidationTest : public ValidationTest {};

class PipelineValidationTest_ResourceTable : public ValidationTest {
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        return {wgpu::FeatureName::ChromiumExperimentalSamplingResourceTable};
    }

  public:
    wgpu::PipelineLayoutDescriptor CreatePipelineLayoutDescriptor(bool usesResourceTable) {
        wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
            device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

        wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
        pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
        pipelineLayoutDescriptor.bindGroupLayouts = &bindGroupLayout;

        wgpu::PipelineLayoutResourceTable resourceTable;
        resourceTable.usesResourceTable = usesResourceTable;
        pipelineLayoutDescriptor.nextInChain = &resourceTable;

        return pipelineLayoutDescriptor;
    }
};
class PipelineValidationTest_ResourceTableDisabled : public PipelineValidationTest_ResourceTable {
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override { return {}; }
};

// Control case where enabling use of a resource table with the feature enabled is valid.
TEST_F(PipelineValidationTest_ResourceTable, SuccessWithFeatureEnabled) {
    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &bindGroupLayout;

    wgpu::PipelineLayoutResourceTable resourceTable;
    resourceTable.usesResourceTable = true;
    pipelineLayoutDescriptor.nextInChain = &resourceTable;

    device.CreatePipelineLayout(&pipelineLayoutDescriptor);
}

// Error case where enabling use of a resource table with the feature disabled is an error.
TEST_F(PipelineValidationTest_ResourceTableDisabled, FailureWithFeatureDisabled) {
    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &bindGroupLayout;

    wgpu::PipelineLayoutResourceTable resourceTable;
    pipelineLayoutDescriptor.nextInChain = &resourceTable;

    // Failure case
    resourceTable.usesResourceTable = true;
    ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&pipelineLayoutDescriptor));

    // Success case
    resourceTable.usesResourceTable = false;
    device.CreatePipelineLayout(&pipelineLayoutDescriptor);
}

// Test that a shader using a resource table requires a layout with one.
TEST_F(PipelineValidationTest_ResourceTable, ShaderRequiresLayoutWithResourceTable) {
    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = utils::CreateShaderModule(device, R"(
        enable chromium_experimental_resource_table;
        @compute @workgroup_size(1) fn main() {
            _ = hasResource<texture_2d<f32>>(0);
            _ = getResource<texture_3d<i32>>(1);
        }
    )");

    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &bindGroupLayout;

    wgpu::PipelineLayoutResourceTable resourceTable;
    pipelineLayoutDescriptor.nextInChain = &resourceTable;

    // Success case, the layout uses a resource table
    resourceTable.usesResourceTable = true;
    csDesc.layout = device.CreatePipelineLayout(&pipelineLayoutDescriptor);
    device.CreateComputePipeline(&csDesc);

    // Failure case, the layout does not use a resource table
    resourceTable.usesResourceTable = false;
    csDesc.layout = device.CreatePipelineLayout(&pipelineLayoutDescriptor);
    ASSERT_DEVICE_ERROR(device.CreateComputePipeline(&csDesc));
}

// Test that it is valid to have a layout specifying a resource table with a shader that
// doesn't have one.
TEST_F(PipelineValidationTest_ResourceTable, ShaderNoResourceTableWithLayoutThatHasOne) {
    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {
        }
    )");

    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor;
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &bindGroupLayout;

    wgpu::PipelineLayoutResourceTable resourceTable;
    pipelineLayoutDescriptor.nextInChain = &resourceTable;

    resourceTable.usesResourceTable = true;
    csDesc.layout = device.CreatePipelineLayout(&pipelineLayoutDescriptor);
    device.CreateComputePipeline(&csDesc);
}

// Test that an auto-generated pipeline with a shader that uses a resource table has a
// PipelineLayoutResourceTable with usesResourceTable == true.
// TODO(crbug.com/463925499): Enable once pipeline layout defaulting is implemented
TEST_F(PipelineValidationTest_ResourceTable, DISABLED_ShaderGeneratesLayoutWithResourceTable) {
    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = utils::CreateShaderModule(device, R"(
        enable chromium_experimental_resource_table;
        @compute @workgroup_size(1) fn main() {
            _ = hasResource<texture_2d<f32>>(0);
            _ = getResource<texture_3d<i32>>(1);
        }
    )");

    wgpu::BindGroupLayout bindGroupLayout = utils::MakeBindGroupLayout(
        device, {{0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform}});

    csDesc.layout = nullptr;  // Auto
    device.CreateComputePipeline(&csDesc);
    // TODO(crbug.com/463925499): Check that resulting pipeline requires a dispatch time resource
    // table to be set
}

}  // namespace
}  // namespace dawn