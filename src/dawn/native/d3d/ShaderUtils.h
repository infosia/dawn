// Copyright 2023 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_D3D_SHADEUTILS_H_
#define SRC_DAWN_NATIVE_D3D_SHADEUTILS_H_

#include <bitset>
#include <string>
#include <utility>
#include <vector>

#include "dawn/common/Sha3.h"
#include "dawn/native/Blob.h"
#include "dawn/native/Serializable.h"
#include "dawn/native/ShaderStatistics.h"
#include "dawn/native/d3d/D3DCompilationRequest.h"
#include "dawn/native/d3d/d3d_platform.h"

namespace dawn::native::d3d {

class Device;
class PlatformFunctions;

#define COMPILED_SHADER_MEMBERS(X) \
    X(Blob, shaderBlob)            \
    X(std::string, hlslSource)     \
    X(Sha3_256::Output, sha3)      \
    X(bool, usesVertexIndex)       \
    X(bool, usesInstanceIndex)     \
    X(Extent3D, workgroupSize)

// `CompiledShader` holds a ref to one of the various representations of shader blobs and
// information used to emulate vertex/instance index starts. It also holds the `hlslSource` for the
// shader compilation, which is only transiently available during Compile, and cleared before it
// returns. It is not written to or loaded from the cache unless Toggle dump_shaders is true.
// clang-format off
DAWN_SERIALIZABLE(struct, CompiledShader, COMPILED_SHADER_MEMBERS) {
    static ResultOrError<CompiledShader> FromValidatedBlob(Blob blob) {
        CompiledShader result;
        DAWN_TRY_ASSIGN(result, FromBlob(std::move(blob)));
        // Only confirm there's a shader blob, the hlslSource is not necessarily preserved.
        DAWN_INVALID_IF(result.shaderBlob.Empty(),
                        "Cached CompiledShader result has empty shader blob");
        return result;
    }
};
// clang-format on
#undef COMPILED_SHADER_MEMBERS

std::string CompileFlagsToString(uint32_t compileFlags);

ResultOrError<CompiledShader> CompileShader(d3d::D3DCompilationRequest r);

InterStageShaderVariablesMask ToInterStageShaderVariablesMask(const std::vector<bool>& inputMask);

void DumpFXCCompiledShader(Device* device,
                           const CompiledShader& compiledShader,
                           uint32_t compileFlags);

struct D3DShaderReflectionData {
    bool valid = false;
    uint32_t tempRegisterCount = 0;
    uint32_t instructionCount = 0;
    uint32_t floatInstructionCount = 0;
    uint32_t intInstructionCount = 0;
    uint32_t uintInstructionCount = 0;
    uint32_t staticFlowControlCount = 0;
    uint32_t dynamicFlowControlCount = 0;
    uint32_t textureNormalInstructions = 0;
    uint32_t textureLoadInstructions = 0;
    uint32_t textureCompInstructions = 0;
    uint32_t barrierInstructions = 0;
    uint32_t interlockedInstructions = 0;
};

D3DShaderReflectionData ReflectShader(const PlatformFunctions* functions,
                                      const Blob& shaderBlob,
                                      DxcCreateInstanceProc dxcCreateInstance = nullptr);

ShaderStageStatisticsData BuildStageStatistics(const D3DShaderReflectionData& reflection,
                                               wgpu::ShaderStage stage);

}  // namespace dawn::native::d3d

#endif  // SRC_DAWN_NATIVE_D3D_SHADEUTILS_H_
