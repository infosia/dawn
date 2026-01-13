// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/native/webgpu/ExternalTextureWGPU.h"

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/TextureWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

// static
ResultOrError<Ref<ExternalTextureBase>> ExternalTexture::Create(
    Device* device,
    const ExternalTextureDescriptor* descriptor) {
    Ref<ExternalTexture> externalTexture = AcquireRef(new ExternalTexture(device, descriptor));
    DAWN_TRY(externalTexture->Initialize(device, descriptor));
    return externalTexture;
}

ExternalTexture::ExternalTexture(Device* device, const ExternalTextureDescriptor* descriptor)
    : ExternalTextureBase(device, descriptor),
      RecordableObject(schema::ObjectType::Texture),
      ObjectWGPU(device->wgpu.externalTextureRelease) {}

ExternalTexture::~ExternalTexture() = default;

MaybeError ExternalTexture::Initialize(Device* device,
                                       const ExternalTextureDescriptor* descriptor) {
    DAWN_TRY(ExternalTextureBase::Initialize(device, descriptor));

    WGPUExternalTextureDescriptor wgpuDescriptor = {};
    std::string_view label_sv(descriptor->label);
    wgpuDescriptor.label = {.data = label_sv.data(), .length = label_sv.length()};
    wgpuDescriptor.plane0 = ToBackend(descriptor->plane0)->GetInnerHandle();
    if (descriptor->plane1) {
        wgpuDescriptor.plane1 = ToBackend(descriptor->plane1)->GetInnerHandle();
    }
    wgpuDescriptor.cropOrigin = {descriptor->cropOrigin.x, descriptor->cropOrigin.y};
    wgpuDescriptor.cropSize = {descriptor->cropSize.width, descriptor->cropSize.height};
    wgpuDescriptor.apparentSize = {descriptor->apparentSize.width, descriptor->apparentSize.height};
    wgpuDescriptor.doYuvToRgbConversionOnly = descriptor->doYuvToRgbConversionOnly;
    wgpuDescriptor.yuvToRgbConversionMatrix = descriptor->yuvToRgbConversionMatrix;
    wgpuDescriptor.srcTransferFunctionParameters = descriptor->srcTransferFunctionParameters;
    wgpuDescriptor.dstTransferFunctionParameters = descriptor->dstTransferFunctionParameters;
    wgpuDescriptor.gamutConversionMatrix = descriptor->gamutConversionMatrix;
    wgpuDescriptor.mirrored = descriptor->mirrored;
    wgpuDescriptor.rotation = static_cast<WGPUExternalTextureRotation>(descriptor->rotation);

    mInnerHandle =
        device->wgpu.deviceCreateExternalTexture(device->GetInnerHandle(), &wgpuDescriptor);
    return {};
}

void ExternalTexture::DestroyImpl(DestroyReason reason) {
    ExternalTextureBase::DestroyImpl(reason);
    // Releasing the inner handle is done by the ObjectWGPU destructor.
}

void ExternalTexture::SetLabelImpl() {
    ToBackend(GetDevice())->CaptureSetLabel(this, GetLabel());
}

MaybeError ExternalTexture::AddReferenced(CaptureContext& captureContext) {
    // TODO(crbug.com/465184041): Capture ExternalTexture
    DAWN_UNREACHABLE();
}

MaybeError ExternalTexture::CaptureCreationParameters(CaptureContext& context) {
    // TODO(crbug.com/465184041): Capture ExternalTexture
    DAWN_UNREACHABLE();
}

}  // namespace dawn::native::webgpu
