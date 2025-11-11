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

#include "dawn/native/webgpu/RenderBundleEncoderWGPU.h"

#include <utility>
#include <vector>

#include "dawn/native/CommandEncoder.h"
// #include "dawn/native/Commands.h"
// #include "dawn/native/RenderBundleEncoder.h"
// #include "dawn/native/webgpu/BindGroupWGPU.h"
// #include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
// #include "dawn/native/webgpu/RenderPipelineWGPU.h"
// #include "dawn/native/webgpu/ToWGPU.h"
#include "dawn/native/webgpu/RenderBundleWGPU.h"

namespace dawn::native::webgpu {

// static
Ref<RenderBundleEncoder> RenderBundleEncoder::Create(
    Device* device,
    const RenderBundleEncoderDescriptor* descriptor) {
    return AcquireRef(new RenderBundleEncoder(device, descriptor));
}

RenderBundleEncoder::RenderBundleEncoder(Device* device,
                                         const RenderBundleEncoderDescriptor* descriptor)
    : RenderBundleEncoderBase(device, descriptor),
      ObjectWGPU(device->wgpu.renderBundleEncoderRelease) {
    const AttachmentState* attachmentState = GetAttachmentState();

    PerColorAttachment<WGPUTextureFormat> colorFormats = {};
    size_t colorAttachmentCount = 0;
    for (ColorAttachmentIndex i : attachmentState->GetColorAttachmentsMask()) {
        colorFormats[i] = ToAPI(attachmentState->GetColorAttachmentFormat(i));
        colorAttachmentCount = static_cast<size_t>(i) + 1;
    }

    WGPURenderBundleEncoderDescriptor bundleEncoderDescriptor =
        WGPU_RENDER_BUNDLE_ENCODER_DESCRIPTOR_INIT;
    bundleEncoderDescriptor.colorFormatCount = colorAttachmentCount;
    bundleEncoderDescriptor.colorFormats = colorFormats.data();
    if (attachmentState->HasDepthStencilAttachment()) {
        bundleEncoderDescriptor.depthStencilFormat =
            ToAPI(attachmentState->GetDepthStencilFormat());
    }
    bundleEncoderDescriptor.sampleCount = attachmentState->GetSampleCount();
    bundleEncoderDescriptor.depthReadOnly = IsDepthReadOnly();
    bundleEncoderDescriptor.stencilReadOnly = IsStencilReadOnly();

    mInnerHandle = device->wgpu.deviceCreateRenderBundleEncoder(device->GetInnerHandle(),
                                                                &bundleEncoderDescriptor);
    DAWN_ASSERT(mInnerHandle);
}

Ref<RenderBundleBase> RenderBundleEncoder::CreateRenderBundleImpl(
    const RenderBundleDescriptor* descriptor,
    RenderPassResourceUsage usages) {
    return RenderBundle::Create(this, descriptor, std::move(usages),
                                std::move(mIndirectDrawMetadata));
}

void RenderBundleEncoder::DestroyImpl() {
    RenderBundleEncoderBase::DestroyImpl();
    ToBackend(GetDevice())->wgpu.renderBundleEncoderRelease(mInnerHandle);
    mInnerHandle = nullptr;
}

}  // namespace dawn::native::webgpu
