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

#include "dawn/native/gles2/RenderPipelineES2.h"

#include "dawn/native/gles2/Forward.h"
#include "dawn/native/gles2/DeviceES2.h"
#include "dawn/native/gles2/GLSLData.h"
#include "dawn/native/gles2/ShaderModuleES2.h"

namespace dawn::native::gles2 {

RenderPipeline::RenderPipeline(Device* device,
                               const UnpackedPtr<RenderPipelineDescriptor>& descriptor)
    : RenderPipelineBase(device, descriptor) {}

MaybeError RenderPipeline::InitializeImpl() {
    const PerStage<ProgrammableStage>& stages = GetAllStages();

    for (SingleShaderStage stageType : IterateStages(kAllStages)) {
        const ProgrammableStage& stage = stages[stageType];
        const ShaderModule* shaderModule = ToBackend(stage.module.Get());
        if (!shaderModule) {
            continue;
        }

        if (!(stageType == SingleShaderStage::Vertex || stageType == SingleShaderStage::Fragment)) {
            return DAWN_INTERNAL_ERROR("Pipeline unexpectedly has compute shader module.\n");
        }

        const PipelineLayout* layout = ToBackend(GetLayout());
        std::string& src = stageType == SingleShaderStage::Vertex ? mVertexSrc : mFragmentSrc;
        DAWN_TRY_ASSIGN(src, shaderModule->GenerateSource(stage, stageType, layout, &mVertexInputs, &mUniforms));
    }

    return {};
}

}  // namespace dawn::native::gles2
