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

#ifndef SRC_DAWN_NATIVE_OPENGL_PIPELINEGL_H_
#define SRC_DAWN_NATIVE_OPENGL_PIPELINEGL_H_

#include <future>
#include <utility>
#include <vector>

#include "dawn/native/IntegerTypes.h"
#include "dawn/native/Pipeline.h"

#include "include/tint/tint.h"

#include "dawn/native/PerStage.h"
#include "dawn/native/opengl/BindingPoint.h"
#include "dawn/native/opengl/FutureHandle.h"
#include "dawn/native/opengl/opengl_platform.h"

namespace dawn::native {
struct ProgrammableStage;
}  // namespace dawn::native

namespace dawn::native::opengl {

class Device;
struct OpenGLFunctions;
class PipelineLayout;
class Sampler;
class Buffer;
class TextureView;

class PipelineGL {
  public:
    PipelineGL();
    ~PipelineGL();

    const std::vector<GLuint>& GetTextureUnitsForSampler(GLuint index) const;
    const std::vector<GLuint>& GetTextureUnitsForTextureView(GLuint index) const;
    const FutureHandle& GetProgramHandle() const;

    const BindingPointToFunctionAndOffset& GetBindingPointBuiltinDataInfo() const;
    bool NeedsTextureBuiltinUniformBuffer() const;

    bool NeedsSSBOLengthUniformBuffer() const;

  protected:
    class SharedData : public RefCounted {
      public:
        SharedData();
        ~SharedData() override;

        FutureHandle mHandle;
        std::vector<std::vector<GLuint>> mUnitsForSamplers;
        std::vector<std::vector<GLuint>> mUnitsForTextures;
        std::vector<GLuint> mPlaceholderSamplerUnits;
        Ref<Sampler> mPlaceholderSampler;
        bool mNeedsSSBOLengthUniformBuffer = false;
        BindingPointToFunctionAndOffset mBindingPointEmulatedBuiltins;
    };

    MaybeError ApplyNow(const OpenGLFunctions& gl, const PipelineLayout* layout);
    MaybeError InitializeBase(Device* device,
                              const PipelineLayout* layout,
                              const PerStage<ProgrammableStage>& stages,
                              bool usesVertexIndex,
                              bool usesInstanceIndex,
                              bool usesFragDepth,
                              VertexAttributeMask bgraSwizzleAttributes);
    void DeleteProgram(Device* device);

    Ref<SharedData> mSharedData;
};

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_PIPELINEGL_H_
