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

#include "dawn/native/ShaderStatistics.h"

#include <sstream>
#include <string>

#include "dawn/native/Device.h"
#include "dawn/native/Toggles.h"

namespace dawn::native {

void ShaderStatisticsCache::BuildCApiView() {
    flatStages.clear();
    flatEntries.clear();

    size_t totalEntries = 0;
    for (const auto& stage : data.stages) {
        totalEntries += stage.extensionEntries.size();
    }
    flatEntries.reserve(totalEntries);

    for (const auto& stageData : data.stages) {
        size_t entryOffset = flatEntries.size();
        for (const auto& entry : stageData.extensionEntries) {
            wgpu::ShaderStatisticEntry cEntry = {};
            cEntry.name = {entry.name.data(), entry.name.size()};
            cEntry.description = {entry.description.data(), entry.description.size()};
            cEntry.format = entry.format;
            cEntry.value = entry.value;
            flatEntries.push_back(cEntry);
        }

        wgpu::ShaderStageStatistics cStage = {};
        cStage.stage = stageData.stage;
        cStage.core = stageData.core;
        cStage.extensionEntryCount = stageData.extensionEntries.size();
        cStage.extensionEntries =
            stageData.extensionEntries.empty() ? nullptr : &flatEntries[entryOffset];
        flatStages.push_back(cStage);
    }
}

wgpu::ShaderStatistics ShaderStatisticsCache::GetCApiView() const {
    wgpu::ShaderStatistics result = {};
    result.stageCount = flatStages.size();
    result.stages = flatStages.empty() ? nullptr : flatStages.data();
    return result;
}

namespace {

const char* ShaderStageToString(wgpu::ShaderStage stage) {
    switch (stage) {
        case wgpu::ShaderStage::Vertex:
            return "vertex";
        case wgpu::ShaderStage::Fragment:
            return "fragment";
        case wgpu::ShaderStage::Compute:
            return "compute";
        default:
            return "unknown";
    }
}

constexpr uint64_t kHighRegisterPressureThreshold = 128;

}  // namespace

void AnalyzeAndWarn(DeviceBase* device, const ShaderStatisticsData& data) {
    bool warnSpill = device->IsToggleEnabled(Toggle::WarnOnShaderSpill);
    bool warnPressure = device->IsToggleEnabled(Toggle::WarnOnHighRegisterPressure);
    bool logAll = device->IsToggleEnabled(Toggle::LogShaderStatistics);

    for (const auto& stage : data.stages) {
        const char* stageName = ShaderStageToString(stage.stage);

        if (warnSpill) {
            std::ostringstream spillDetails;
            bool hasSpill = false;
            for (const auto& entry : stage.extensionEntries) {
                if (entry.name.find("Spill") != std::string::npos ||
                    entry.name.find("spill") != std::string::npos) {
                    uint64_t val = entry.value.valueUint64;
                    if (val > 0) {
                        if (hasSpill) {
                            spillDetails << ", ";
                        }
                        spillDetails << entry.name << ": " << val;
                        hasSpill = true;
                    }
                }
            }
            if (stage.core.validFields & wgpu::ShaderStatisticValidField::ScratchMemoryBytes) {
                if (stage.core.scratchMemoryBytes > 0) {
                    if (hasSpill) {
                        spillDetails << ", ";
                    }
                    spillDetails << "scratchMemoryBytes: " << stage.core.scratchMemoryBytes;
                    hasSpill = true;
                }
            }
            if (hasSpill) {
                std::ostringstream msg;
                msg << "Shader register spilling detected in " << stageName
                    << " stage of pipeline.\n  " << spillDetails.str()
                    << "\n  This may impact GPU performance. Consider reducing shader complexity.";
                device->EmitLog(wgpu::LoggingType::Warning, msg.str());
            }
        }

        if (warnPressure &&
            (stage.core.validFields & wgpu::ShaderStatisticValidField::RegisterCount) &&
            stage.core.registerCount > kHighRegisterPressureThreshold) {
            std::ostringstream msg;
            msg << "High register pressure in " << stageName
                << " stage: registerCount=" << stage.core.registerCount
                << " (threshold=" << kHighRegisterPressureThreshold << ").";
            device->EmitLog(wgpu::LoggingType::Warning, msg.str());
        }

        if (logAll) {
            std::ostringstream msg;
            msg << "Shader statistics for " << stageName << " stage:";
            const auto& core = stage.core;
            struct {
                wgpu::ShaderStatisticValidField flag;
                const char* label;
                uint64_t value;
            } coreFields[] = {
                {wgpu::ShaderStatisticValidField::RegisterCount, "registerCount",
                 core.registerCount},
                {wgpu::ShaderStatisticValidField::ScalarRegisterCount, "scalarRegisterCount",
                 core.scalarRegisterCount},
                {wgpu::ShaderStatisticValidField::InstructionCount, "instructionCount",
                 core.instructionCount},
                {wgpu::ShaderStatisticValidField::ScratchMemoryBytes, "scratchMemoryBytes",
                 core.scratchMemoryBytes},
                {wgpu::ShaderStatisticValidField::CodeSizeBytes, "codeSizeBytes",
                 core.codeSizeBytes},
                {wgpu::ShaderStatisticValidField::ThreadgroupMemoryBytes, "threadgroupMemoryBytes",
                 core.threadgroupMemoryBytes},
                {wgpu::ShaderStatisticValidField::SimdWidth, "simdWidth", core.simdWidth},
                {wgpu::ShaderStatisticValidField::MaxThreadsPerGroup, "maxThreadsPerGroup",
                 core.maxThreadsPerGroup},
            };
            for (const auto& field : coreFields) {
                if (core.validFields & field.flag) {
                    msg << " " << field.label << "=" << field.value;
                }
            }
            for (const auto& entry : stage.extensionEntries) {
                msg << " " << entry.name << "=" << entry.value.valueUint64;
            }
            device->EmitLog(msg.str());
        }
    }
}

}  // namespace dawn::native
