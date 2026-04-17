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

#include "dawn/native/vulkan/ShaderStatisticsVk.h"

#include <cstring>
#include <vector>

namespace dawn::native::vulkan {

ShaderStatisticsData QueryVulkanPipelineStatistics(Device* device, VkPipeline pipeline) {
    VkPipelineInfoKHR pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
    pipelineInfo.pipeline = pipeline;

    uint32_t executableCount = 0;
    device->fn.GetPipelineExecutablePropertiesKHR(device->GetVkDevice(), &pipelineInfo,
                                                  &executableCount, nullptr);
    if (executableCount == 0) {
        return {};
    }

    std::vector<VkPipelineExecutablePropertiesKHR> executableProps(executableCount);
    for (auto& prop : executableProps) {
        prop.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
        prop.pNext = nullptr;
    }
    device->fn.GetPipelineExecutablePropertiesKHR(device->GetVkDevice(), &pipelineInfo,
                                                  &executableCount, executableProps.data());

    ShaderStatisticsData data;

    for (uint32_t i = 0; i < executableCount; ++i) {
        VkPipelineExecutableStatisticKHR statTemplate = {};
        statTemplate.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;

        VkPipelineExecutableInfoKHR execInfo = {};
        execInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
        execInfo.pipeline = pipeline;
        execInfo.executableIndex = i;

        uint32_t statCount = 0;
        device->fn.GetPipelineExecutableStatisticsKHR(device->GetVkDevice(), &execInfo, &statCount,
                                                      nullptr);
        if (statCount == 0) {
            continue;
        }

        std::vector<VkPipelineExecutableStatisticKHR> stats(statCount, statTemplate);
        device->fn.GetPipelineExecutableStatisticsKHR(device->GetVkDevice(), &execInfo, &statCount,
                                                      stats.data());

        ShaderStageStatisticsData stageData;
        stageData.stage = VkShaderStageToWgpu(executableProps[i].stages);
        stageData.core.simdWidth = executableProps[i].subgroupSize;
        if (executableProps[i].subgroupSize > 0) {
            stageData.core.validFields |= wgpu::ShaderStatisticValidField::SimdWidth;
        }

        for (uint32_t s = 0; s < statCount; ++s) {
            const auto& stat = stats[s];
            const char* name = stat.name;
            uint64_t val = VkStatToUint64(stat.format, stat.value);

            // Map well-known stat names to core fields.
            // AMD uses: "VGPRs", "SGPRs", "Scratch size", "Code size", "ISA size", "LDS size"
            // NVIDIA uses: "Register Count", "Binary Size", "Stack Size",
            //              "Local Memory Size", "Shared Memory Size"
            // Intel uses: "Instruction Count", "Scratch Memory Size",
            //             "Workgroup Memory Size"
            if (std::strcmp(name, "VGPRs") == 0 || std::strcmp(name, "GRFs") == 0 ||
                std::strcmp(name, "Register Count") == 0) {
                stageData.core.registerCount = val;
                stageData.core.validFields |= wgpu::ShaderStatisticValidField::RegisterCount;
            } else if (std::strcmp(name, "SGPRs") == 0) {
                stageData.core.scalarRegisterCount = val;
                stageData.core.validFields |= wgpu::ShaderStatisticValidField::ScalarRegisterCount;
            } else if (std::strcmp(name, "Instruction Count") == 0) {
                stageData.core.instructionCount = val;
                stageData.core.validFields |= wgpu::ShaderStatisticValidField::InstructionCount;
            } else if (std::strcmp(name, "Scratch size") == 0 ||
                       std::strcmp(name, "Stack Size") == 0 ||
                       std::strcmp(name, "Scratch Memory Size") == 0) {
                stageData.core.scratchMemoryBytes = val;
                stageData.core.validFields |= wgpu::ShaderStatisticValidField::ScratchMemoryBytes;
            } else if (std::strcmp(name, "Code size") == 0 ||
                       std::strcmp(name, "ISA size") == 0 ||
                       std::strcmp(name, "Binary Size") == 0) {
                stageData.core.codeSizeBytes = val;
                stageData.core.validFields |= wgpu::ShaderStatisticValidField::CodeSizeBytes;
            } else if (std::strcmp(name, "LDS size") == 0 ||
                       std::strcmp(name, "Shared Memory Size") == 0 ||
                       std::strcmp(name, "Workgroup Memory Size") == 0) {
                stageData.core.threadgroupMemoryBytes = val;
                stageData.core.validFields |=
                    wgpu::ShaderStatisticValidField::ThreadgroupMemoryBytes;
            } else {
                ShaderStatisticEntryData entry;
                entry.name = stat.name;
                entry.description = stat.description;
                entry.format = VkStatFormatToWgpu(stat.format);
                entry.value = VkStatValueToWgpu(stat.format, stat.value);
                stageData.extensionEntries.push_back(std::move(entry));
            }
        }

        // When a single executable covers multiple stages (e.g., vertex+fragment merged),
        // duplicate the statistics into separate per-stage entries.
        wgpu::ShaderStage execStages = VkShaderStageToWgpu(executableProps[i].stages);
        bool hasVertex = (execStages & wgpu::ShaderStage::Vertex) != wgpu::ShaderStage::None;
        bool hasFragment = (execStages & wgpu::ShaderStage::Fragment) != wgpu::ShaderStage::None;
        bool hasCompute = (execStages & wgpu::ShaderStage::Compute) != wgpu::ShaderStage::None;

        if (hasVertex && hasFragment) {
            ShaderStageStatisticsData vertexData = stageData;
            vertexData.stage = wgpu::ShaderStage::Vertex;
            data.stages.push_back(std::move(vertexData));

            stageData.stage = wgpu::ShaderStage::Fragment;
            data.stages.push_back(std::move(stageData));
        } else if (hasVertex) {
            stageData.stage = wgpu::ShaderStage::Vertex;
            data.stages.push_back(std::move(stageData));
        } else if (hasFragment) {
            stageData.stage = wgpu::ShaderStage::Fragment;
            data.stages.push_back(std::move(stageData));
        } else if (hasCompute) {
            stageData.stage = wgpu::ShaderStage::Compute;
            data.stages.push_back(std::move(stageData));
        } else {
            stageData.stage = execStages;
            data.stages.push_back(std::move(stageData));
        }
    }

    return data;
}

}  // namespace dawn::native::vulkan
