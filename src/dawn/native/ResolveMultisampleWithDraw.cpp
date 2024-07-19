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

#include "dawn/native/ResolveMultisampleWithDraw.h"

#include <sstream>
#include <string>

#include "absl/container/inlined_vector.h"
#include "dawn/common/Assert.h"
#include "dawn/common/Enumerator.h"
#include "dawn/common/HashUtils.h"
#include "dawn/native/BindGroup.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/Device.h"
#include "dawn/native/InternalPipelineStore.h"
#include "dawn/native/RenderPassEncoder.h"
#include "dawn/native/RenderPipeline.h"
#include "dawn/native/utils/WGPUHelpers.h"
#include "dawn/native/webgpu_absl_format.h"

namespace dawn::native {

namespace {

constexpr char kResolveMultisampleVS[] = R"(

@vertex fn vert_fullscreen_quad(
  @builtin(vertex_index) vertex_index : u32,
) -> @builtin(position) vec4f {
  const pos = array(
      vec2f(-1.0, -1.0),
      vec2f( 3.0, -1.0),
      vec2f(-1.0,  3.0));
  return vec4f(pos[vertex_index], 0.0, 1.0);
}
)";

// Generate the fragment shader to average multiple samples into one.
std::string GenerateFS(uint32_t sampleCount) {
    std::ostringstream ss;

    ss << R"(
@group(0) @binding(0) var srcTex : texture_multisampled_2d<f32>;

@fragment
fn resolve_multisample(@builtin(position) position : vec4f) -> @location(0) vec4f {
    var sum = vec4f(0.0, 0.0, 0.0, 0.0);)";

    for (uint32_t sample = 0; sample < sampleCount; ++sample) {
        ss << absl::StrFormat("    sum += textureLoad(srcTex, vec2u(position.xy), %u);\n", sample);
    }
    ss << absl::StrFormat("    return sum / f32(%u);\n", sampleCount) << "}\n";

    return ss.str();
}

ResultOrError<Ref<RenderPipelineBase>> GetOrCreateResolveMultisamplePipeline(
    DeviceBase* device,
    const ResolveMultisampleWithDrawPipelineKey& pipelineKey) {
    // Try finding the pipeline from the cache.
    InternalPipelineStore* store = device->GetInternalPipelineStore();
    {
        auto it = store->resolveMultisamplePipelines.find(pipelineKey);
        if (it != store->resolveMultisamplePipelines.end()) {
            return it->second;
        }
    }

    // vertex shader's source.
    ShaderModuleWGSLDescriptor wgslDesc = {};
    ShaderModuleDescriptor shaderModuleDesc = {};
    shaderModuleDesc.nextInChain = &wgslDesc;
    wgslDesc.code = kResolveMultisampleVS;

    Ref<ShaderModuleBase> vshaderModule;
    DAWN_TRY_ASSIGN(vshaderModule, device->CreateShaderModule(&shaderModuleDesc));

    // fragment shader's source will depend on sample count.
    std::string fsCode = GenerateFS(pipelineKey.sampleCount);
    wgslDesc.code = fsCode.c_str();
    Ref<ShaderModuleBase> fshaderModule;
    DAWN_TRY_ASSIGN(fshaderModule, device->CreateShaderModule(&shaderModuleDesc));

    FragmentState fragmentState = {};
    fragmentState.module = fshaderModule.Get();
    fragmentState.entryPoint = "resolve_multisample";

    // Color target states.
    ColorTargetState colorTarget = {};
    colorTarget.format = pipelineKey.colorTargetFormat;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    RenderPipelineDescriptor renderPipelineDesc = {};
    renderPipelineDesc.label = "resolve_multisample";
    renderPipelineDesc.vertex.module = vshaderModule.Get();
    renderPipelineDesc.vertex.entryPoint = "vert_fullscreen_quad";
    renderPipelineDesc.fragment = &fragmentState;

    // Bind group layout.
    BindGroupLayoutEntry bglEntry;
    bglEntry.binding = 0;
    bglEntry.visibility = wgpu::ShaderStage::Fragment;
    bglEntry.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
    bglEntry.texture.viewDimension = wgpu::TextureViewDimension::e2D;
    bglEntry.texture.multisampled = true;

    BindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entries = &bglEntry;
    bglDesc.entryCount = 1;

    Ref<BindGroupLayoutBase> bindGroupLayout;
    DAWN_TRY_ASSIGN(bindGroupLayout, device->CreateBindGroupLayout(&bglDesc));

    Ref<PipelineLayoutBase> pipelineLayout;
    DAWN_TRY_ASSIGN(pipelineLayout, utils::MakeBasicPipelineLayout(device, bindGroupLayout));
    renderPipelineDesc.layout = pipelineLayout.Get();

    Ref<RenderPipelineBase> pipeline;
    DAWN_TRY_ASSIGN(pipeline, device->CreateRenderPipeline(&renderPipelineDesc));

    store->resolveMultisamplePipelines.emplace(pipelineKey, pipeline);
    return pipeline;
}

}  // namespace

ResultOrError<ResolveMultisampleCallback> ResolveMultisampleWithDraw(
    DeviceBase* device,
    CommandEncoder* encoder,
    const UnpackedPtr<RenderPassDescriptor>& renderPassDescriptor,
    TextureViewBase* src,
    TextureViewBase* dst) {
    DAWN_ASSERT(device->IsLockedByCurrentThreadIfNeeded());
    DAWN_ASSERT(device->CanTextureLoadResolveTargetInTheSameRenderpass());

    ResolveMultisampleWithDrawPipelineKey pipelineKey{dst->GetFormat().format,
                                                      src->GetTexture()->GetSampleCount()};
    Ref<RenderPipelineBase> pipeline;
    DAWN_TRY_ASSIGN(pipeline, GetOrCreateResolveMultisamplePipeline(device, pipelineKey));

    Ref<BindGroupLayoutBase> bgl;
    DAWN_TRY_ASSIGN(bgl, pipeline->GetBindGroupLayout(0));

    BindGroupEntry bgEntry = {};
    bgEntry.binding = 0;
    bgEntry.textureView = src;

    BindGroupDescriptor bgDesc = {};
    bgDesc.layout = bgl.Get();
    bgDesc.entryCount = 1;
    bgDesc.entries = &bgEntry;

    Ref<BindGroupBase> bindGroup;
    DAWN_TRY_ASSIGN(bindGroup, device->CreateBindGroup(&bgDesc));

    ResolveMultisampleCallback callback =
        [encoder, pipeline, bindGroup, dst,
         rect = renderPassDescriptor.Get<RenderPassDescriptorExpandResolveRect>()]() -> void {
        // Color attachment descriptor.
        RenderPassColorAttachment colorAttachmentDesc;
        colorAttachmentDesc.view = dst;
        colorAttachmentDesc.loadOp = wgpu::LoadOp::Load;
        colorAttachmentDesc.storeOp = wgpu::StoreOp::Store;

        // Create render pass.
        RenderPassDescriptor renderPassDesc;
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &colorAttachmentDesc;
        Ref<RenderPassEncoder> renderEncoder = encoder->BeginRenderPass(&renderPassDesc);

        // Draw to perform the resolve.
        renderEncoder->APISetBindGroup(0, bindGroup.Get(), 0, nullptr);
        renderEncoder->APISetPipeline(pipeline.Get());
        if (rect) {
            renderEncoder->APISetScissorRect(rect->x, rect->y, rect->width, rect->height);
        }
        renderEncoder->APIDraw(3, 1, 0, 0);
        renderEncoder->End();
    };

    return callback;
}

size_t ResolveMultisampleWithDrawPipelineKey::HashFunc::operator()(
    const ResolveMultisampleWithDrawPipelineKey& key) const {
    size_t hash = 0;

    HashCombine(&hash, key.colorTargetFormat);
    HashCombine(&hash, key.sampleCount);

    return hash;
}

bool ResolveMultisampleWithDrawPipelineKey::EqualityFunc::operator()(
    const ResolveMultisampleWithDrawPipelineKey& a,
    const ResolveMultisampleWithDrawPipelineKey& b) const {
    return a.colorTargetFormat == b.colorTargetFormat && a.sampleCount == b.sampleCount;
}

}  // namespace dawn::native
