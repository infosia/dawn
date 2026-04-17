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

#include "dawn/tests/unittests/wire/WireTest.h"

namespace dawn::wire {
namespace {

using testing::_;
using testing::Return;

class WireShaderStatisticsTests : public WireTest {
  protected:
    void SetUp() override {
        WireTest::SetUp();

        // Create shader module via wire
        wgpu::ShaderModuleDescriptor smDesc = {};
        mShader = device.CreateShaderModule(&smDesc);
        mApiShader = api.GetNewShaderModule();
        EXPECT_CALL(api, DeviceCreateShaderModule(apiDevice, _)).WillOnce(Return(mApiShader));
        FlushClient();
    }

    void TearDown() override {
        mShader = nullptr;
        WireTest::TearDown();
    }

    wgpu::ShaderModule mShader;
    WGPUShaderModule mApiShader;
};

// Wire client stub returns error for ComputePipeline GetShaderStatistics.
TEST_F(WireShaderStatisticsTests, ComputePipelineReturnsError) {
    wgpu::ComputePipelineDescriptor desc = {};
    desc.compute.module = mShader;

    WGPUComputePipeline apiPipeline = api.GetNewComputePipeline();
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);
    EXPECT_CALL(api, DeviceCreateComputePipeline(apiDevice, _)).WillOnce(Return(apiPipeline));
    FlushClient();

    wgpu::ShaderStatistics stats = {};
    EXPECT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Error);
}

// Wire client stub returns error for RenderPipeline GetShaderStatistics.
TEST_F(WireShaderStatisticsTests, RenderPipelineReturnsError) {
    wgpu::RenderPipelineDescriptor desc = {};
    desc.vertex.module = mShader;

    WGPURenderPipeline apiPipeline = api.GetNewRenderPipeline();
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&desc);
    EXPECT_CALL(api, DeviceCreateRenderPipeline(apiDevice, _)).WillOnce(Return(apiPipeline));
    FlushClient();

    wgpu::ShaderStatistics stats = {};
    EXPECT_EQ(pipeline.GetShaderStatistics(&stats), wgpu::Status::Error);
}

}  // namespace
}  // namespace dawn::wire
