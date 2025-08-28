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

namespace {

class ComboRenderPipelineDescriptor {
  public:
    explicit ComboRenderPipelineDescriptor(const RenderPipeline* pipeline) {
        mDesc.nextInChain = nullptr;
        mDesc.label = ToOutputStringView(pipeline->GetLabel());
        auto layout = pipeline->GetLayout();
        if (layout != nullptr) {
            mDesc.layout = ToBackend(layout)->GetInnerHandle();
        } else {
            mDesc.layout = nullptr;
        }

        // Vertex State
        const ProgrammableStage& vertex = pipeline->GetStage(SingleShaderStage::Vertex);
        mDesc.vertex.nextInChain = nullptr;
        mDesc.vertex.module = ToBackend(vertex.module.Get())->GetInnerHandle();
        mDesc.vertex.entryPoint = ToOutputStringView(vertex.entryPoint);
        PopulateWGPUConstants(&mVertexConstants, vertex.constants);
        mDesc.vertex.constants = mVertexConstants.data();
        mDesc.vertex.constantCount = mVertexConstants.size();

        // Vertex Buffers
        size_t bufferCount = 0;
        for (VertexBufferSlot slot : pipeline->GetVertexBuffersUsed()) {
            const VertexBufferInfo& dawnBuffer = pipeline->GetVertexBuffer(slot);
            WGPUVertexBufferLayout* wgpuBuffer = &mVertexBuffers[bufferCount];
            wgpuBuffer->arrayStride = dawnBuffer.arrayStride;
            wgpuBuffer->stepMode = ToAPI(dawnBuffer.stepMode);

            auto& wgpuAttributes = mVertexAttributes[static_cast<size_t>(slot)];
            wgpuAttributes.clear();

            for (VertexAttributeLocation location : pipeline->GetAttributeLocationsUsed()) {
                const VertexAttributeInfo& dawnAttr = pipeline->GetAttribute(location);
                if (dawnAttr.vertexBufferSlot == slot) {
                    wgpuAttributes.push_back({
                        .nextInChain = nullptr,
                        .format = ToAPI(dawnAttr.format),
                        .offset = dawnAttr.offset,
                        .shaderLocation = static_cast<uint32_t>(dawnAttr.shaderLocation),
                    });
                }
            }
            wgpuBuffer->attributes = wgpuAttributes.data();
            wgpuBuffer->attributeCount = wgpuAttributes.size();
            bufferCount++;
        }
        mDesc.vertex.bufferCount = bufferCount;
        mDesc.vertex.buffers = mVertexBuffers.data();

        // Primitive State
        mDesc.primitive.nextInChain = nullptr;
        mDesc.primitive.topology = ToAPI(pipeline->GetPrimitiveTopology());
        if (IsStripPrimitiveTopology(pipeline->GetPrimitiveTopology())) {
            mDesc.primitive.stripIndexFormat = ToAPI(pipeline->GetStripIndexFormat());
        } else {
            mDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        }
        mDesc.primitive.frontFace = ToAPI(pipeline->GetFrontFace());
        mDesc.primitive.cullMode = ToAPI(pipeline->GetCullMode());
        mDesc.primitive.unclippedDepth = pipeline->HasUnclippedDepth();

        // Depth Stencil State
        if (pipeline->HasDepthStencilAttachment()) {
            mDepthStencil = ToWGPU(pipeline->GetDepthStencilState());
            mDesc.depthStencil = &mDepthStencil;
        } else {
            mDesc.depthStencil = nullptr;
        }

        // Multisample State
        mDesc.multisample.nextInChain = nullptr;
        mDesc.multisample.count = pipeline->GetSampleCount();
        mDesc.multisample.mask = pipeline->GetSampleMask();
        mDesc.multisample.alphaToCoverageEnabled = pipeline->IsAlphaToCoverageEnabled();

        // Fragment State
        if (pipeline->HasStage(SingleShaderStage::Fragment)) {
            const ProgrammableStage& fragment = pipeline->GetStage(SingleShaderStage::Fragment);
            mFragmentState.nextInChain = nullptr;
            mFragmentState.module = ToBackend(fragment.module.Get())->GetInnerHandle();
            mFragmentState.entryPoint = ToOutputStringView(fragment.entryPoint);
            PopulateWGPUConstants(&mFragmentConstants, fragment.constants);
            mFragmentState.constants = mFragmentConstants.data();
            mFragmentState.constantCount = mFragmentConstants.size();

            uint32_t targetCount = 0;
            for (auto i : pipeline->GetColorAttachmentsMask()) {
                const ColorTargetState* dawnTarget = pipeline->GetColorTargetState(i);
                WGPUColorTargetState* wgpuTarget = &mColorTargets[targetCount];
                wgpuTarget->nextInChain = nullptr;
                wgpuTarget->format = ToAPI(dawnTarget->format);

                if (dawnTarget->blend != nullptr) {
                    mBlends[targetCount] = ToWGPU(dawnTarget->blend);
                    wgpuTarget->blend = &mBlends[targetCount];
                } else {
                    wgpuTarget->blend = nullptr;
                }
                wgpuTarget->writeMask = ToAPI(dawnTarget->writeMask);
                targetCount++;
            }
            mFragmentState.targetCount = targetCount;
            mFragmentState.targets = mColorTargets.data();

            mDesc.fragment = &mFragmentState;
        } else {
            mDesc.fragment = nullptr;
        }
    }
    const WGPURenderPipelineDescriptor* Get() const { return &mDesc; }

  private:
    WGPURenderPipelineDescriptor mDesc;
    ConstantEntryVector mVertexConstants;
    std::array<WGPUVertexBufferLayout, kMaxVertexBuffers> mVertexBuffers;
    std::array<absl::InlinedVector<WGPUVertexAttribute, kMaxVertexAttributes>, kMaxVertexBuffers>
        mVertexAttributes;
    WGPUDepthStencilState mDepthStencil;
    WGPUFragmentState mFragmentState;
    ConstantEntryVector mFragmentConstants;
    std::array<WGPUColorTargetState, kMaxColorAttachments> mColorTargets;
    std::array<WGPUBlendState, kMaxColorAttachments> mBlends;
};

}  // namespace

// static
Ref<RenderPipeline> RenderPipeline::CreateUninitialized(
    Device* device,
    const UnpackedPtr<RenderPipelineDescriptor>& descriptor) {
    return AcquireRef(new RenderPipeline(device, descriptor));
}

RenderPipeline::RenderPipeline(Device* device,
                               const UnpackedPtr<RenderPipelineDescriptor>& descriptor)
    : RenderPipelineBase(device, descriptor), ObjectWGPU(device->wgpu.renderPipelineRelease) {}

MaybeError RenderPipeline::InitializeImpl() {
    auto device = ToBackend(GetDevice());

    ComboRenderPipelineDescriptor comboDesc(this);
    mInnerHandle =
        device->wgpu.deviceCreateRenderPipeline(device->GetInnerHandle(), comboDesc.Get());
    DAWN_ASSERT(mInnerHandle);
    return {};
}

}  // namespace dawn::native::webgpu
