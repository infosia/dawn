// Copyright 2026 The Dawn & Tint Authors
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

#include <webgpu/webgpu_cpp.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "dawn/native/DawnNative.h"

namespace {

const char* ComponentTypeToWgslType(wgpu::SubgroupMatrixComponentType c) {
    switch (c) {
        case wgpu::SubgroupMatrixComponentType::F32:
            return "f32";
        case wgpu::SubgroupMatrixComponentType::F16:
            return "f16";
        case wgpu::SubgroupMatrixComponentType::U32:
            return "u32";
        case wgpu::SubgroupMatrixComponentType::I32:
            return "i32";
        case wgpu::SubgroupMatrixComponentType::U8:
            return "u8";
        case wgpu::SubgroupMatrixComponentType::I8:
            return "i8";
    }
    return "<invalid>";
}

const char* ComponentTypeToScalarType(wgpu::SubgroupMatrixComponentType c) {
    switch (c) {
        case wgpu::SubgroupMatrixComponentType::F32:
            return "f32";
        case wgpu::SubgroupMatrixComponentType::F16:
            return "f16";
        case wgpu::SubgroupMatrixComponentType::U32:
        case wgpu::SubgroupMatrixComponentType::U8:
            return "u32";
        case wgpu::SubgroupMatrixComponentType::I32:
        case wgpu::SubgroupMatrixComponentType::I8:
            return "i32";
    }
    return "<invalid>";
}

uint32_t ComponentTypeByteSize(wgpu::SubgroupMatrixComponentType c) {
    switch (c) {
        case wgpu::SubgroupMatrixComponentType::F32:
        case wgpu::SubgroupMatrixComponentType::U32:
        case wgpu::SubgroupMatrixComponentType::I32:
            return 4;
        case wgpu::SubgroupMatrixComponentType::F16:
            return 2;
        case wgpu::SubgroupMatrixComponentType::U8:
        case wgpu::SubgroupMatrixComponentType::I8:
            return 1;
    }
    return 0;
}

void CpuMatMul(const std::vector<float>& a,
               const std::vector<float>& b,
               std::vector<float>& c,
               uint32_t M,
               uint32_t N,
               uint32_t K) {
    for (uint32_t row = 0; row < M; row++) {
        for (uint32_t col = 0; col < N; col++) {
            float sum = 0.0f;
            for (uint32_t k = 0; k < K; k++) {
                sum += a[row * K + k] * b[k * N + col];
            }
            c[row * N + col] = sum;
        }
    }
}

}  // namespace

int main(int /*argc*/, const char** /*argv*/) {
    // Create instance.
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.capabilities.timedWaitAnyEnable = true;
    auto instance = wgpu::CreateInstance(&instanceDesc);
    if (!instance) {
        fprintf(stderr, "Failed to create instance\n");
        return 1;
    }

    // Request adapter.
    wgpu::Adapter adapter;
    wgpu::RequestAdapterOptions options{};
    options.powerPreference = wgpu::PowerPreference::HighPerformance;
    auto adapterFuture = instance.RequestAdapter(
        &options, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestAdapterStatus status, wgpu::Adapter result, wgpu::StringView msg) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                fprintf(stderr, "Failed to get adapter: %.*s\n", static_cast<int>(msg.length),
                        msg.data);
                return;
            }
            adapter = std::move(result);
        });
    instance.WaitAny(adapterFuture, UINT64_MAX);
    if (!adapter) {
        fprintf(stderr, "No adapter available\n");
        return 1;
    }

    // Check feature support.
    if (!adapter.HasFeature(wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix)) {
        fprintf(stderr, "ChromiumExperimentalSubgroupMatrix not supported\n");
        return 1;
    }

    // Query configs.
    wgpu::AdapterInfo adapterInfo;
    wgpu::AdapterPropertiesSubgroupMatrixConfigs matrixConfigs;
    adapterInfo.nextInChain = &matrixConfigs;
    if (adapter.GetInfo(&adapterInfo) != wgpu::Status::Success || matrixConfigs.configCount == 0) {
        fprintf(stderr, "No subgroup matrix configs available\n");
        return 1;
    }

    printf("Adapter: %s\n", adapterInfo.device.data);
    printf("Found %zu subgroup matrix config(s)\n\n", matrixConfigs.configCount);

    // Pick the first f32 config, or fall back to the first config.
    const wgpu::SubgroupMatrixConfig* chosen = &matrixConfigs.configs[0];
    for (size_t i = 0; i < matrixConfigs.configCount; i++) {
        if (matrixConfigs.configs[i].componentType == wgpu::SubgroupMatrixComponentType::F32 &&
            matrixConfigs.configs[i].resultComponentType ==
                wgpu::SubgroupMatrixComponentType::F32) {
            chosen = &matrixConfigs.configs[i];
            break;
        }
    }

    uint32_t M = chosen->M;
    uint32_t N = chosen->N;
    uint32_t K = chosen->K;
    printf("Using config: M=%u N=%u K=%u component=%s result=%s\n", M, N, K,
           ComponentTypeToWgslType(chosen->componentType),
           ComponentTypeToWgslType(chosen->resultComponentType));

    // Only f32 configs are supported in this sample for simplicity.
    if (chosen->componentType != wgpu::SubgroupMatrixComponentType::F32 ||
        chosen->resultComponentType != wgpu::SubgroupMatrixComponentType::F32) {
        fprintf(stderr, "This sample only supports f32 configs\n");
        return 1;
    }

    // Request device with required features.
    std::vector<wgpu::FeatureName> features = {
        wgpu::FeatureName::ChromiumExperimentalSubgroupMatrix};
    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.requiredFeatures = features.data();
    deviceDesc.requiredFeatureCount = features.size();
    deviceDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
            fprintf(stderr, "Device error (%d): %.*s\n", static_cast<int>(type),
                    static_cast<int>(msg.length), msg.data);
        });

    wgpu::Device device;
    auto deviceFuture = adapter.RequestDevice(
        &deviceDesc, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestDeviceStatus status, wgpu::Device result, wgpu::StringView msg) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                fprintf(stderr, "Failed to get device: %.*s\n", static_cast<int>(msg.length),
                        msg.data);
                return;
            }
            device = std::move(result);
        });
    instance.WaitAny(deviceFuture, UINT64_MAX);
    if (!device) {
        return 1;
    }
    wgpu::Queue queue = device.GetQueue();

    // Prepare input data: A is MxK, B is KxN, C is MxN.
    std::vector<float> hostA(M * K);
    std::vector<float> hostB(K * N);
    for (uint32_t i = 0; i < M * K; i++) {
        hostA[i] = static_cast<float>((i % 5) + 1);
    }
    for (uint32_t i = 0; i < K * N; i++) {
        hostB[i] = static_cast<float>((i % 3) + 1);
    }

    // Create GPU buffers.
    uint32_t inputSize = (M * K + K * N) * sizeof(float);
    uint32_t outputSize = M * N * sizeof(float);

    wgpu::BufferDescriptor inputBufDesc{};
    inputBufDesc.size = inputSize;
    inputBufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    wgpu::Buffer inputBuf = device.CreateBuffer(&inputBufDesc);

    wgpu::BufferDescriptor outputBufDesc{};
    outputBufDesc.size = outputSize;
    outputBufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::MapRead;
    wgpu::Buffer outputBuf = device.CreateBuffer(&outputBufDesc);

    // Upload input data.
    queue.WriteBuffer(inputBuf, 0, hostA.data(), M * K * sizeof(float));
    queue.WriteBuffer(inputBuf, M * K * sizeof(float), hostB.data(), K * N * sizeof(float));

    // Build shader.
    char shader[2048];
    snprintf(shader, sizeof(shader),
             R"(enable chromium_experimental_subgroup_matrix;

const M = %uu;
const N = %uu;
const K = %uu;
const kLoadOffset = K * M;

alias LeftType = subgroup_matrix_left<f32, K, M>;
alias RightType = subgroup_matrix_right<f32, N, K>;
alias ResultType = subgroup_matrix_result<f32, N, M>;

@group(0) @binding(0) var<storage, read>       inputs : array<f32, K*M + N*K>;
@group(0) @binding(1) var<storage, read_write> output : array<f32, M*N>;

@compute @workgroup_size(%u)
fn main() {
    let lhs = subgroupMatrixLoad<LeftType>(&inputs, 0, false, K);
    let rhs = subgroupMatrixLoad<RightType>(&inputs, kLoadOffset, false, N);
    let result = subgroupMatrixMultiply<f32>(lhs, rhs);
    subgroupMatrixStore(&output, 0, result, false, N);
}
)",
             M, N, K, adapterInfo.subgroupMaxSize);

    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shader;
    wgpu::ShaderModuleDescriptor smDesc{};
    smDesc.nextInChain = &wgslDesc;
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&smDesc);

    wgpu::ComputePipelineDescriptor pipelineDesc{};
    pipelineDesc.compute.module = shaderModule;
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&pipelineDesc);

    wgpu::BindGroup bindGroup;
    {
        wgpu::BindGroupEntry entries[2] = {};
        entries[0].binding = 0;
        entries[0].buffer = inputBuf;
        entries[0].size = inputSize;
        entries[1].binding = 1;
        entries[1].buffer = outputBuf;
        entries[1].size = outputSize;
        wgpu::BindGroupDescriptor bgDesc{};
        bgDesc.layout = pipeline.GetBindGroupLayout(0);
        bgDesc.entryCount = 2;
        bgDesc.entries = entries;
        bindGroup = device.CreateBindGroup(&bgDesc);
    }

    // Dispatch.
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.DispatchWorkgroups(1);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Read back results.
    bool mapDone = false;
    wgpu::MapAsyncStatus mapStatus;
    auto mapFuture =
        outputBuf.MapAsync(wgpu::MapMode::Read, 0, outputSize, wgpu::CallbackMode::WaitAnyOnly,
                           [&](wgpu::MapAsyncStatus status, wgpu::StringView) {
                               mapStatus = status;
                               mapDone = true;
                           });
    instance.WaitAny(mapFuture, UINT64_MAX);

    if (mapStatus != wgpu::MapAsyncStatus::Success) {
        fprintf(stderr, "Failed to map output buffer\n");
        return 1;
    }

    const float* gpuResult = static_cast<const float*>(outputBuf.GetConstMappedRange());

    // CPU reference.
    std::vector<float> cpuResult(M * N);
    CpuMatMul(hostA, hostB, cpuResult, M, N, K);

    // Compare.
    bool match = true;
    float maxError = 0.0f;
    for (uint32_t i = 0; i < M * N; i++) {
        float err = std::fabs(gpuResult[i] - cpuResult[i]);
        if (err > maxError) {
            maxError = err;
        }
        if (err > 1e-3f) {
            match = false;
        }
    }

    printf("GPU result %s CPU reference (max error: %e)\n", match ? "matches" : "DOES NOT match",
           maxError);

    outputBuf.Unmap();
    return match ? 0 : 1;
}
