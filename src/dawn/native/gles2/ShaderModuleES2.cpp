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

#include "dawn/native/gles2/ShaderModuleES2.h"

#include "dawn/native/TintUtils.h"
#include "dawn/native/gles2/DeviceES2.h"
#include "dawn/native/gles2/Forward.h"
#include "dawn/native/gles2/GLSLData.h"
#include "dawn/native/gles2/PipelineLayoutES2.h"

namespace dawn::native::gles2 {

namespace {

GLSLType WGSLTypeNameToGLSLType(const std::string& name) {
    if (name == "f32") {
        return GLSLType::kFloat;
    } else if (name == "vec2<f32>") {
        return GLSLType::kFloat2;
    } else if (name == "vec4<f32>") {
        return GLSLType::kFloat4;
    // TODO try not to allow unsigned types to show up here
    } else if (name == "u32") {
        return GLSLType::kInt;
    } else if (name == "mat4x4<f32>") {
        return GLSLType::kMatrix4;
    } else {
        printf("!! unsupported wgsl type: %s\n", name.c_str());
        return GLSLType::kFloat;
    }
}

}

ResultOrError<Ref<ShaderModule>> ShaderModule::Create(
    Device* device,
    const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
    std::vector<tint::wgsl::Extension> internalExtensions,
    ShaderModuleParseResult* parseResult,
    OwnedCompilationMessages* compilationMessages) {
    Ref<ShaderModule> module = AcquireRef(new ShaderModule(device, descriptor, internalExtensions));
    DAWN_TRY(module->Initialize(parseResult, compilationMessages));
    return module;
}

ShaderModule::ShaderModule(DeviceBase* device,
             const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
             std::vector<tint::wgsl::Extension> internalExtensions)
    : ShaderModuleBase(device, descriptor, internalExtensions) {}

MaybeError ShaderModule::Initialize(ShaderModuleParseResult* parseResult,
                                    OwnedCompilationMessages* compilationMessages) {
    ScopedTintICEHandler scopedICEHandler(GetDevice());
    return InitializeBase(parseResult, compilationMessages);
}

ResultOrError<std::string> ShaderModule::GenerateSource(
    const ProgrammableStage& stage,
    SingleShaderStage stageType,
    const PipelineLayout* layout,
    std::vector<InputDeclaration>* vertexInputs,
    std::vector<UniformDeclaration>* uniforms) const {

    // TODO this functionality is copied from ShaderModuleGL.cpp, except we need to do caching here
    Device* device = ToBackend(GetDevice());

    tint::glsl::writer::Options options;
    {
        const gl::OpenGLVersion& version = device->GetGL().GetVersion();

        tint::glsl::writer::Version::Standard standard;
        switch (version.GetStandard()) {
            case gl::OpenGLVersion::Standard::Desktop:
                standard = tint::glsl::writer::Version::Standard::kDesktop;
                break;
            case gl::OpenGLVersion::Standard::ES:
                standard = tint::glsl::writer::Version::Standard::kES;
                break;
            default:
                DAWN_UNREACHABLE();
        }

        options.version =
            tint::glsl::writer::Version(standard, version.GetMajor(), version.GetMinor());
        options.disable_polyfill_integer_div_mod =
            device->IsToggleEnabled(Toggle::DisablePolyfillsOnIntegerDivisonAndModulo);
        options.disable_robustness = false;
        // TODO
        // options.bindings = ;
    }

    tint::Program program;
    std::string remappedEntryPoint;
    {
        tint::ast::transform::Renamer::Remappings assignedRenamings = {};

        const EntryPointMetadata& entryPointMetadata = GetEntryPoint(stage.entryPoint);
        for (size_t i = 0; i < entryPointMetadata.interStageVariables.size(); ++i) {
            if (entryPointMetadata.usedInterStageVariables[i]) {
                assignedRenamings.emplace(
                    entryPointMetadata.interStageVariables[i].name,
                    "dawn_interstage_location_" + std::to_string(i));
            }
        }

        const BindingInfoArray& moduleBindingInfo = entryPointMetadata.bindings;
        for (BindGroupIndex group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
            for (const auto& [binding, shaderBindingInfo] : moduleBindingInfo[group]) {
                if (std::holds_alternative<BufferBindingInfo>(shaderBindingInfo.bindingInfo)) {
                    assignedRenamings.emplace(
                        shaderBindingInfo.name,
                        (stageType == SingleShaderStage::Vertex ? "v_" : "f_") + shaderBindingInfo.name);
                }
            }
        }

        const bool disableSymbolRenaming = device->IsToggleEnabled(Toggle::DisableSymbolRenaming);

        tint::ast::transform::Manager manager;
        tint::ast::transform::DataMap inputs;
        manager.Add<tint::ast::transform::SingleEntryPoint>();
        inputs.Add<tint::ast::transform::SingleEntryPoint::Config>(stage.entryPoint);
        manager.Add<tint::ast::transform::Renamer>();
        inputs.Add<tint::ast::transform::Renamer::Config>(
            disableSymbolRenaming ? tint::ast::transform::Renamer::Target::kGlslKeywords
                                  : tint::ast::transform::Renamer::Target::kAll,
            false, std::move(assignedRenamings));

        if (!stage.metadata->overrides.empty()) {
            manager.Add<tint::ast::transform::SubstituteOverride>();
            inputs.Add<tint::ast::transform::SubstituteOverride::Config>(
                BuildSubstituteOverridesTransformConfig(stage));
        }

        tint::ast::transform::DataMap outputs;
        DAWN_TRY_ASSIGN(program, RunTransforms(&manager, &GetTintProgram()->program, inputs,
                                               &outputs, nullptr));

        auto* data = outputs.Get<tint::ast::transform::Renamer::Data>();
        auto it = data->remappings.find(stage.entryPoint.data());
        if (it != data->remappings.end()) {
            remappedEntryPoint = it->second;
        } else {
            remappedEntryPoint = stage.entryPoint;
        }
    }

    tint::ast::PipelineStage tintStage = tint::ast::PipelineStage::kNone;
    switch (stageType) {
        case SingleShaderStage::Vertex:
            tintStage = tint::ast::PipelineStage::kVertex;
            break;
        case SingleShaderStage::Fragment:
            tintStage = tint::ast::PipelineStage::kFragment;
            break;
        default:
            break;
    }

    std::vector<tint::glsl::writer::InputDeclaration> newVertexInputs;
    std::vector<tint::glsl::writer::UniformDeclaration> newUniforms;

    tint::Result<tint::glsl::writer::Output> result = tint::glsl::writer::Generate(
        program, options, remappedEntryPoint, tintStage,
        stageType == SingleShaderStage::Vertex ? &newVertexInputs : nullptr,
        &newUniforms);

    DAWN_INVALID_IF(result != tint::Success, "An error occurred while generating GLSL:\n%s",
                    result.Failure().reason.Str());

    for (const tint::glsl::writer::InputDeclaration& attribute : newVertexInputs) {
        vertexInputs->push_back({
            attribute.mLocation,
            WGSLTypeNameToGLSLType(attribute.mTypeName),
            attribute.mName,
        });
    }

    for (const tint::glsl::writer::UniformDeclaration& uniform : newUniforms) {
        DAWN_ASSERT(uniform.mGroup == 0);
        uniforms->push_back({
            uniform.mBinding,
            WGSLTypeNameToGLSLType(uniform.mTypeName),
            uniform.mName,
        });
    }

    if (device->IsToggleEnabled(Toggle::DumpShaders)) {
        std::ostringstream dumpedMsg;
        dumpedMsg << "/* Dumped generated GLSL */\n" << result->glsl;
        device->EmitLog(WGPULoggingType_Info, dumpedMsg.str().c_str());
        printf("%s\n", result->glsl.c_str());
    }

    return result->glsl;
}

}  // namespace dawn::native::gles2
