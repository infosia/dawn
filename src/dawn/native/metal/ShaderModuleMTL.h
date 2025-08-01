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

#ifndef SRC_DAWN_NATIVE_METAL_SHADERMODULEMTL_H_
#define SRC_DAWN_NATIVE_METAL_SHADERMODULEMTL_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "absl/strings/escaping.h"
#include "dawn/native/ShaderModule.h"

#include "dawn/common/NSRef.h"
#include "dawn/native/Error.h"

#import <Metal/Metal.h>

namespace dawn::native {
struct ProgrammableStage;
}

namespace dawn::native::metal {

class Device;
class PipelineLayout;
class RenderPipeline;

struct MetalShaderParams {
    SingleShaderStage stage;
    ShaderModuleBase::ShaderModuleHash shaderModuleHash;
    std::string entryPointName;
    PipelineConstantEntries pipelineConstants;
    tint::msl::writer::Options tintOptions;
    bool useStrictMath;

    bool operator==(const MetalShaderParams& other) const {
        return std::tie(stage, shaderModuleHash, entryPointName, pipelineConstants, tintOptions,
                        useStrictMath) == std::tie(other.stage, other.shaderModuleHash,
                                                   other.entryPointName, other.pipelineConstants,
                                                   other.tintOptions, other.useStrictMath);
    }

    template <typename H>
    friend H AbslHashValue(H state, const MetalShaderParams& params) {
        return H::combine(std::move(state), params.stage, params.shaderModuleHash,
                          params.entryPointName, params.pipelineConstants,
                          params.tintOptions.HashCode(), params.useStrictMath);
    }
};

struct MetalFunctionData {
    NSRef<NSString> msl;
    NSPRef<id<MTLFunction>> function;
    bool needsStorageBufferLength;
    std::vector<uint32_t> workgroupAllocations;
    MTLSize localWorkgroupSize;
};

MetalShaderParams ComputeShaderParams(PipelineBase* pipeline, SingleShaderStage stage);

ResultOrError<MetalFunctionData> LoadShaderEntryPoint(PipelineBase* pipeline,
                                                      SingleShaderStage stage);

class MetalShaderLibrary : public RefCounted,
                           public CachedObject,
                           public ContentLessObjectCacheable<MetalShaderLibrary> {
  public:
    MetalShaderLibrary(Device* device, MetalShaderParams params, std::string_view label);

    using LoadEntryPointCallback = std::function<void(ResultOrError<MetalFunctionData>)>;
    void LoadEntryPoint(ShaderModuleBase::ScopedUseTintProgram useTintProgram,
                        LoadEntryPointCallback callback);

    // For ContentLessObjectCache:
    size_t ComputeContentHash() override { return absl::HashOf(mParams); }

    struct EqualityFunc {
        bool operator()(const MetalShaderLibrary* lhs, const MetalShaderLibrary* rhs) const {
            return lhs->mParams == rhs->mParams;
        }
    };

  private:
    ~MetalShaderLibrary() override;
    void DeleteThis() override;

    const raw_ptr<Device> mDevice;
    const MetalShaderParams mParams;
    const std::string mLabel;

    Mutex mMutex;
    enum class ResultState {
        kNotStarted,
        kPending,
        kReady,
    };
    ResultState mResultState = ResultState::kNotStarted;
    std::optional<MetalFunctionData> mResult;
    std::unique_ptr<ErrorData> mError;
    std::vector<LoadEntryPointCallback> mPendingCallbacks;
};

class ShaderModule final : public ShaderModuleBase {
  public:
    static ResultOrError<Ref<ShaderModule>> Create(
        Device* device,
        const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
        const std::vector<tint::wgsl::Extension>& internalExtensions,
        ShaderModuleParseResult* parseResult);

  private:
    ShaderModule(Device* device,
                 const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
                 std::vector<tint::wgsl::Extension> internalExtensions);
    ~ShaderModule() override;

    MaybeError Initialize(ShaderModuleParseResult* parseResult);
};

}  // namespace dawn::native::metal

#endif  // SRC_DAWN_NATIVE_METAL_SHADERMODULEMTL_H_
