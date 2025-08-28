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
    mDesc.nextInChain = nullptr;
    mDesc.label = ToOutputStringView(desc->label);
    if (desc->layout != nullptr) {
        mDesc.layout = ToBackend(desc->layout)->GetInnerHandle();
    } else {
        mDesc.layout = nullptr;
    }

    // Vertex State
    mDesc.vertex.nextInChain = nullptr;
    mDesc.vertex.module = ToBackend(desc->vertex.module)->GetInnerHandle();
    mDesc.vertex.entryPoint = ToWGPUEntryPoint(desc->vertex.entryPoint);
    mDesc.vertex.constantCount = desc->vertex.constantCount;
    mDesc.vertex.constants =
        ToWGPU(desc->vertex.constants, desc->vertex.constantCount, &mVertexConstants);

    // Vertex Buffers
    mDesc.vertex.bufferCount = desc->vertex.bufferCount;
    for (size_t i = 0; i < desc->vertex.bufferCount; ++i) {
        const auto& nativeBuffer = desc->vertex.buffers[i];
        mVertexBuffers[i].nextInChain = nullptr;
        mVertexBuffers[i].stepMode = ToAPI(nativeBuffer.stepMode);
        mVertexBuffers[i].arrayStride = nativeBuffer.arrayStride;
        mVertexBuffers[i].attributeCount = nativeBuffer.attributeCount;

        mVertexAttributes[i].reserve(nativeBuffer.attributeCount);
        for (size_t j = 0; j < nativeBuffer.attributeCount; ++j) {
            const auto& nativeAttribute = nativeBuffer.attributes[j];
            mVertexAttributes[i].push_back({
                .nextInChain = nullptr,
                .format = ToAPI(nativeAttribute.format),
                .offset = nativeAttribute.offset,
                .shaderLocation = nativeAttribute.shaderLocation,
            });
        }
        mVertexBuffers[i].attributes = mVertexAttributes[i].data();
    }
    mDesc.vertex.buffers = mVertexBuffers.data();

    // Primitive State
    mDesc.primitive = ToWGPU(&desc->primitive);

    // Depth Stencil State
    if (desc->depthStencil != nullptr) {
        mDepthStencil = ToWGPU(desc->depthStencil);
        mDesc.depthStencil = &mDepthStencil;
    } else {
        mDesc.depthStencil = nullptr;
    }

    // Multisample State
    mDesc.multisample = ToWGPU(&desc->multisample);

    // Fragment State
    if (desc->fragment != nullptr) {
        auto f = desc->fragment;
        mFragmentState.nextInChain = nullptr;
        mFragmentState.module = ToBackend(f->module)->GetInnerHandle();
        mFragmentState.entryPoint = ToWGPUEntryPoint(f->entryPoint);
        mFragmentState.constantCount = f->constantCount;
        mFragmentState.constants = ToWGPU(f->constants, f->constantCount, &mFragmentConstants);

        // Color targets
        mFragmentState.targetCount = f->targetCount;
        size_t blendCount = 0;

        for (size_t i = 0; i < f->targetCount; ++i) {
            const auto& nativeTarget = f->targets[i];
            mColorTargets[i].nextInChain = nullptr;
            mColorTargets[i].format = ToAPI(nativeTarget.format);
            mColorTargets[i].writeMask = ToAPI(nativeTarget.writeMask);

            if (nativeTarget.blend != nullptr) {
                mBlends[blendCount] = ToWGPU(nativeTarget.blend);
                mColorTargets[i].blend = &mBlends[blendCount];
                blendCount++;
            } else {
                mColorTargets[i].blend = nullptr;
            }
        }
        mFragmentState.targets = mColorTargets.data();

        mDesc.fragment = &mFragmentState;
    } else {
        mDesc.fragment = nullptr;
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
