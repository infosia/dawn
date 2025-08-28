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

#include "src/dawn/native/webgpu/RenderPipelineWGPU.h"

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/PipelineLayoutWGPU.h"
#include "dawn/native/webgpu/ShaderModuleWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

ComboRenderPipelineDescriptor::ComboRenderPipelineDescriptor(const RenderPipelineDescriptor* desc) {
    cDesc.nextInChain = nullptr;
    cDesc.label = ToOutputStringView(desc->label);
    if (desc->layout != nullptr) {
        cDesc.layout = ToBackend(desc->layout)->GetInnerHandle();
    } else {
        cDesc.layout = nullptr;
    }

    // Vertex State
    cDesc.vertex.nextInChain = nullptr;
    cDesc.vertex.module = ToBackend(desc->vertex.module)->GetInnerHandle();
    cDesc.vertex.entryPoint = ToWGPUEntryPoint(desc->vertex.entryPoint);
    cDesc.vertex.constantCount = desc->vertex.constantCount;
    cDesc.vertex.constants =
        ToWGPU(desc->vertex.constants, desc->vertex.constantCount, &cVertexConstants);

    // Vertex Buffers
    cDesc.vertex.bufferCount = desc->vertex.bufferCount;
    for (size_t i = 0; i < desc->vertex.bufferCount; ++i) {
        const auto& nativeBuffer = desc->vertex.buffers[i];
        cVertexBuffers[i].nextInChain = nullptr;
        cVertexBuffers[i].stepMode = ToAPI(nativeBuffer.stepMode);
        cVertexBuffers[i].arrayStride = nativeBuffer.arrayStride;
        cVertexBuffers[i].attributeCount = nativeBuffer.attributeCount;

        cVertexAttributes[i].reserve(nativeBuffer.attributeCount);
        for (size_t j = 0; j < nativeBuffer.attributeCount; ++j) {
            const auto& nativeAttribute = nativeBuffer.attributes[j];
            cVertexAttributes[i].push_back({
                .nextInChain = nullptr,
                .format = ToAPI(nativeAttribute.format),
                .offset = nativeAttribute.offset,
                .shaderLocation = nativeAttribute.shaderLocation,
            });
        }
        cVertexBuffers[i].attributes = cVertexAttributes[i].data();
    }
    cDesc.vertex.buffers = cVertexBuffers.data();

    // Primitive State
    cDesc.primitive = ToWGPU(&desc->primitive);

    // Depth Stencil State
    if (desc->depthStencil != nullptr) {
        cDepthStencil = ToWGPU(desc->depthStencil);
        cDesc.depthStencil = &cDepthStencil;
    } else {
        cDesc.depthStencil = nullptr;
    }

    // Multisample State
    cDesc.multisample = ToWGPU(&desc->multisample);

    // Fragment State
    if (desc->fragment != nullptr) {
        // cFragmentState = ToWGPU(desc->fragment);
        auto f = desc->fragment;
        cFragmentState.nextInChain = nullptr;
        cFragmentState.module = ToBackend(f->module)->GetInnerHandle();
        cFragmentState.entryPoint = ToWGPUEntryPoint(f->entryPoint);
        cFragmentState.constantCount = f->constantCount;
        cFragmentState.constants = ToWGPU(f->constants, f->constantCount, &cFragmentConstants);

        // Color targets
        cFragmentState.targetCount = f->targetCount;
        size_t blendCount = 0;

        for (size_t i = 0; i < f->targetCount; ++i) {
            const auto& nativeTarget = f->targets[i];
            cColorTargets[i].nextInChain = nullptr;
            cColorTargets[i].format = ToAPI(nativeTarget.format);
            cColorTargets[i].writeMask = ToAPI(nativeTarget.writeMask);

            if (nativeTarget.blend != nullptr) {
                cBlends[blendCount] = ToWGPU(nativeTarget.blend);
                cColorTargets[i].blend = &cBlends[blendCount];
                blendCount++;
            } else {
                cColorTargets[i].blend = nullptr;
            }
        }
        cFragmentState.targets = cColorTargets.data();

        cDesc.fragment = &cFragmentState;
    } else {
        cDesc.fragment = nullptr;
    }
}

// static
Ref<RenderPipeline> RenderPipeline::CreateUninitialized(
    Device* device,
    const UnpackedPtr<RenderPipelineDescriptor>& descriptor) {
    return AcquireRef(new RenderPipeline(device, descriptor));
}

RenderPipeline::RenderPipeline(Device* device,
                               const UnpackedPtr<RenderPipelineDescriptor>& descriptor)
    : RenderPipelineBase(device, descriptor),
      ObjectWGPU(device->wgpu.renderPipelineRelease),
      mDescriptor(*descriptor) {}

MaybeError RenderPipeline::InitializeImpl() {
    auto device = ToBackend(GetDevice());
    mInnerHandle =
        device->wgpu.deviceCreateRenderPipeline(device->GetInnerHandle(), mDescriptor.Get());
    DAWN_ASSERT(mInnerHandle);
    return {};
}

}  // namespace dawn::native::webgpu
