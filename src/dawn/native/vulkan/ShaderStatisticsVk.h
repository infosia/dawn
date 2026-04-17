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

#ifndef SRC_DAWN_NATIVE_VULKAN_SHADERSTATISTICSVK_H_
#define SRC_DAWN_NATIVE_VULKAN_SHADERSTATISTICSVK_H_

#include <cstring>
#include <vector>

#include "dawn/common/vulkan_platform.h"
#include "dawn/native/ShaderStatistics.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "dawn/native/vulkan/VulkanFunctions.h"

namespace dawn::native::vulkan {

inline wgpu::ShaderStatisticFormat VkStatFormatToWgpu(
    VkPipelineExecutableStatisticFormatKHR format) {
    switch (format) {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
            return wgpu::ShaderStatisticFormat::Bool32;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
            return wgpu::ShaderStatisticFormat::Int64;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
            return wgpu::ShaderStatisticFormat::Uint64;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
            return wgpu::ShaderStatisticFormat::Float64;
        default:
            return wgpu::ShaderStatisticFormat::Uint64;
    }
}

inline wgpu::ShaderStatisticValue VkStatValueToWgpu(VkPipelineExecutableStatisticFormatKHR format,
                                                    VkPipelineExecutableStatisticValueKHR value) {
    wgpu::ShaderStatisticValue result = {};
    switch (format) {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
            result.valueBool = value.b32;
            break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
            result.valueInt64 = value.i64;
            break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
            result.valueUint64 = value.u64;
            break;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
            result.valueFloat64 = value.f64;
            break;
        default:
            break;
    }
    return result;
}

inline uint64_t VkStatToUint64(VkPipelineExecutableStatisticFormatKHR format,
                               VkPipelineExecutableStatisticValueKHR value) {
    switch (format) {
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
            return value.u64;
        case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
            return static_cast<uint64_t>(value.i64);
        default:
            return 0;
    }
}

inline wgpu::ShaderStage VkShaderStageToWgpu(VkShaderStageFlags stages) {
    wgpu::ShaderStage result = wgpu::ShaderStage::None;
    if (stages & VK_SHADER_STAGE_VERTEX_BIT) {
        result |= wgpu::ShaderStage::Vertex;
    }
    if (stages & VK_SHADER_STAGE_FRAGMENT_BIT) {
        result |= wgpu::ShaderStage::Fragment;
    }
    if (stages & VK_SHADER_STAGE_COMPUTE_BIT) {
        result |= wgpu::ShaderStage::Compute;
    }
    return result;
}

// Queries pipeline executable statistics from Vulkan and converts to Dawn's internal format.
ShaderStatisticsData QueryVulkanPipelineStatistics(Device* device, VkPipeline pipeline);

}  // namespace dawn::native::vulkan

#endif  // SRC_DAWN_NATIVE_VULKAN_SHADERSTATISTICSVK_H_
