// Copyright 2025 The Dawn & Tint Authors
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

#ifndef INCLUDE_DAWN_REPLAY_REPLAY_H_
#define INCLUDE_DAWN_REPLAY_REPLAY_H_

#include <istream>
#include <memory>

#include "dawn/webgpu_cpp.h"

namespace dawn::replay {

using ReplayStream = std::istream;

enum class ReplayResourceType {
    BindGroup,
    BindGroupLayout,
    Buffer,
    CommandBuffer,
    ComputePipeline,
    Device,
    PipelineLayout,
    QuerySet,
    RenderBundle,
    RenderPipeline,
    Sampler,
    ShaderModule,
    Texture,
    TextureView,
    Invalid
};

template <typename T>
struct ReplayResourceTypeTraits {
    static constexpr ReplayResourceType Type = ReplayResourceType::Invalid;
    using CType = void*;
};

#define DAWN_REPLAY_DEFINE_RESOURCE_TYPE(WgpuType, EnumVal, CTypeVal)           \
    template <>                                                                 \
    struct ReplayResourceTypeTraits<wgpu::WgpuType> {                           \
        static constexpr ReplayResourceType Type = ReplayResourceType::EnumVal; \
        using CType = CTypeVal;                                                 \
    };

DAWN_REPLAY_DEFINE_RESOURCE_TYPE(BindGroup, BindGroup, WGPUBindGroup)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(BindGroupLayout, BindGroupLayout, WGPUBindGroupLayout)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(Buffer, Buffer, WGPUBuffer)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(CommandBuffer, CommandBuffer, WGPUCommandBuffer)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(ComputePipeline, ComputePipeline, WGPUComputePipeline)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(Device, Device, WGPUDevice)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(PipelineLayout, PipelineLayout, WGPUPipelineLayout)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(QuerySet, QuerySet, WGPUQuerySet)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(RenderBundle, RenderBundle, WGPURenderBundle)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(RenderPipeline, RenderPipeline, WGPURenderPipeline)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(Sampler, Sampler, WGPUSampler)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(ShaderModule, ShaderModule, WGPUShaderModule)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(Texture, Texture, WGPUTexture)
DAWN_REPLAY_DEFINE_RESOURCE_TYPE(TextureView, TextureView, WGPUTextureView)

#undef DAWN_REPLAY_DEFINE_RESOURCE_TYPE

class Replay {
  public:
    virtual ~Replay() = default;

    template <typename T>
    T GetObjectByLabel(const char* label) const {
        void* handle = GetImpl(label, ReplayResourceTypeTraits<T>::Type);
        if (!handle) {
            return nullptr;
        }
        return T(reinterpret_cast<typename ReplayResourceTypeTraits<T>::CType>(handle));
    }

  protected:
    virtual void* GetImpl(const char* label, ReplayResourceType type) const = 0;
};

std::unique_ptr<Replay> ReplayCapture(wgpu::Device device,
                                      ReplayStream& commandStream,
                                      ReplayStream& contentStream);

}  // namespace dawn::replay

#endif  // INCLUDE_DAWN_REPLAY_REPLAY_H_
