// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/opengl/PipelineGL.h"

#include <algorithm>
#include <future>
#include <set>
#include <sstream>
#include <string>

#include "dawn/common/Range.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Device.h"
#include "dawn/native/Pipeline.h"
#include "dawn/native/opengl/BufferGL.h"
#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/Forward.h"
#include "dawn/native/opengl/FutureHandle.h"
#include "dawn/native/opengl/OpenGLFunctions.h"
#include "dawn/native/opengl/PipelineLayoutGL.h"
#include "dawn/native/opengl/QueueGL.h"
#include "dawn/native/opengl/SamplerGL.h"
#include "dawn/native/opengl/ShaderModuleGL.h"
#include "dawn/native/opengl/TextureGL.h"
#include "dawn/native/opengl/UtilsGL.h"

namespace dawn::native::opengl {

PipelineGL::SharedData::SharedData() = default;
PipelineGL::SharedData::~SharedData() = default;

PipelineGL::PipelineGL() : mSharedData(AcquireRef(new SharedData())) {}

PipelineGL::~PipelineGL() = default;

MaybeError PipelineGL::InitializeBase(Device* device,
                                      const PipelineLayout* layout,
                                      const PerStage<ProgrammableStage>& stages,
                                      bool usesVertexIndex,
                                      bool usesInstanceIndex,
                                      bool usesFragDepth,
                                      VertexAttributeMask bgraSwizzleAttributes) {
    Queue* queue = ToBackend(device->GetQueue());

    mSharedData->mHandle = AcquireRef(new FutureHandle());
    Ref<SharedData> sharedData = mSharedData;
    return queue->AddGLTask([sharedData, device, layout, stages, usesVertexIndex, usesInstanceIndex,
                             usesFragDepth, bgraSwizzleAttributes]() mutable -> MaybeError {
        const OpenGLFunctions& gl = device->GetGL();
        GLuint program = DAWN_GL_TRY(gl, CreateProgram());

        wgpu::ShaderStage activeStages = wgpu::ShaderStage::None;
        for (SingleShaderStage stage : IterateStages(kAllStages)) {
            if (stages[stage].module != nullptr) {
                activeStages |= StageBit(stage);
            }
        }

        std::set<CombinedSampler> combinedSamplers;
        sharedData->mNeedsSSBOLengthUniformBuffer = false;
        std::vector<GLuint> glShaders;
        for (SingleShaderStage stage : IterateStages(activeStages)) {
            ShaderModule* module = ToBackend(stages[stage].module.Get());
            bool needsSSBOLengthUniformBuffer = false;
            std::vector<CombinedSampler> stageCombinedSamplers;
            GLuint shader;
            DAWN_TRY_ASSIGN(
                shader,
                module->CompileShader(gl, stages[stage], stage, usesVertexIndex, usesInstanceIndex,
                                      usesFragDepth, bgraSwizzleAttributes, &stageCombinedSamplers,
                                      layout, &sharedData->mBindingPointEmulatedBuiltins,
                                      &needsSSBOLengthUniformBuffer));

            sharedData->mNeedsSSBOLengthUniformBuffer |= needsSSBOLengthUniformBuffer;
            combinedSamplers.insert(stageCombinedSamplers.begin(), stageCombinedSamplers.end());

            DAWN_GL_TRY(gl, AttachShader(program, shader));
            glShaders.push_back(shader);
        }

        DAWN_GL_TRY(gl, LinkProgram(program));

        GLint linkStatus = GL_FALSE;
        DAWN_GL_TRY(gl, GetProgramiv(program, GL_LINK_STATUS, &linkStatus));
        if (linkStatus == GL_FALSE) {
            GLint infoLogLength = 0;
            DAWN_GL_TRY(gl, GetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength));

            if (infoLogLength > 1) {
                std::vector<char> buffer(infoLogLength);
                DAWN_GL_TRY(gl, GetProgramInfoLog(program, infoLogLength, nullptr, &buffer[0]));
                return DAWN_VALIDATION_ERROR("Program link failed:\n%s", buffer.data());
            }
        }

        DAWN_GL_TRY(gl, UseProgram(program));
        const auto& indices = layout->GetBindingIndexInfo();

        sharedData->mUnitsForSamplers.resize(layout->GetNumSamplers());
        sharedData->mUnitsForTextures.resize(layout->GetNumSampledTextures());

        GLuint textureUnit = 0;
        for (const auto& combined : combinedSamplers) {
            absl::InlinedVector<GLint, 1> uniformsToSet;

            const BindGroupLayoutInternalBase* textureBgl =
                layout->GetBindGroupLayout(combined.textureLocation.group);
            BindingIndex textureArrayStart =
                textureBgl->GetBindingIndex(combined.textureLocation.binding);

            for (auto textureArrayElement : Range(combined.textureLocation.arraySize)) {
                GLuint textureGLIndex = indices[combined.textureLocation.group]
                                               [textureArrayStart + textureArrayElement];
                sharedData->mUnitsForTextures[textureGLIndex].push_back(textureUnit);

                if (!combined.samplerLocation) {
                    sharedData->mPlaceholderSamplerUnits.push_back(textureUnit);
                } else {
                    const BindGroupLayoutInternalBase* samplerBgl =
                        layout->GetBindGroupLayout(combined.samplerLocation->group);
                    BindingIndex samplerBindingIndex =
                        samplerBgl->GetBindingIndex(combined.samplerLocation->binding);

                    GLuint samplerGLIndex =
                        indices[combined.samplerLocation->group][samplerBindingIndex];
                    sharedData->mUnitsForSamplers[samplerGLIndex].push_back(textureUnit);
                }

                uniformsToSet.push_back(textureUnit);
                textureUnit++;
            }

            std::string name = combined.GetName();
            GLint location = DAWN_GL_TRY(gl, GetUniformLocation(program, name.c_str()));
            if (uniformsToSet.size() == 1) {
                DAWN_GL_TRY(gl, Uniform1i(location, uniformsToSet[0]));
            } else {
                DAWN_GL_TRY(gl, Uniform1iv(location, uniformsToSet.size(), uniformsToSet.data()));
            }
        }

        if (!sharedData->mPlaceholderSamplerUnits.empty()) {
            Ref<SamplerBase> sampler;
            DAWN_TRY_ASSIGN(sampler, layout->GetDevice()->CreateSampler());
            sharedData->mPlaceholderSampler = ToBackend(std::move(sampler));
        }

        for (GLuint glShader : glShaders) {
            DAWN_GL_TRY(gl, DetachShader(program, glShader));
            DAWN_GL_TRY(gl, DeleteShader(glShader));
        }

        sharedData->mHandle->Set(program);
        return {};
    });
}


void PipelineGL::DeleteProgram(Device* device) {
    Queue* queue = ToBackend(device->GetQueue());
    Ref<FutureHandle> handle = mSharedData->mHandle;

    IgnoreErrors(queue->AddGLTask([device, handle]() -> MaybeError {
        const OpenGLFunctions& gl = device->GetGL();
        DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteProgram(handle->GetOrDefault()));
        return {};
    }));
}

const std::vector<GLuint>& PipelineGL::GetTextureUnitsForSampler(GLuint index) const {
    DAWN_ASSERT(index < mSharedData->mUnitsForSamplers.size());
    return mSharedData->mUnitsForSamplers[index];
}

const std::vector<GLuint>& PipelineGL::GetTextureUnitsForTextureView(GLuint index) const {
    DAWN_ASSERT(index < mSharedData->mUnitsForTextures.size());
    return mSharedData->mUnitsForTextures[index];
}

const FutureHandle& PipelineGL::GetProgramHandle() const {
    return *mSharedData->mHandle.Get();
}

MaybeError PipelineGL::ApplyNow(const OpenGLFunctions& gl, const PipelineLayout* layout) {
    DAWN_GL_TRY(gl, UseProgram(GetProgramHandle()));
    for (GLuint unit : mSharedData->mPlaceholderSamplerUnits) {
        DAWN_ASSERT(mSharedData->mPlaceholderSampler.Get() != nullptr);
        DAWN_GL_TRY(gl, BindSampler(unit, mSharedData->mPlaceholderSampler->GetHandle()));
    }

    return {};
}

const BindingPointToFunctionAndOffset& PipelineGL::GetBindingPointBuiltinDataInfo() const {
    return mSharedData->mBindingPointEmulatedBuiltins;
}

bool PipelineGL::NeedsTextureBuiltinUniformBuffer() const {
    return !mSharedData->mBindingPointEmulatedBuiltins.empty();
}

bool PipelineGL::NeedsSSBOLengthUniformBuffer() const {
    return mSharedData->mNeedsSSBOLengthUniformBuffer;
}

}  // namespace dawn::native::opengl
