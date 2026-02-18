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

#include "dawn/native/webgpu/SharedFenceWGPU.h"

#include "dawn/common/Platform.h"
#include "dawn/common/StringViewUtils.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

// static
ResultOrError<Ref<SharedFence>> SharedFence::Create(Device* device,
                                                    StringView label,
                                                    const SharedFenceDescriptor* descriptor) {
    UnpackedPtr<SharedFenceDescriptor> unpacked;
    DAWN_TRY_ASSIGN(unpacked, ValidateAndUnpack(descriptor));

    WGPUSharedFenceDescriptor innerDesc = WGPU_SHARED_FENCE_DESCRIPTOR_INIT;
    innerDesc.label = ToOutputStringView(label);

    // TODO(451310574): Handle all possible chained structures in SharedFenceDescriptor.
    // For now we only handle IOSurface as it is used in tests on macOS.
#if DAWN_PLATFORM_IS(MACOS)
    WGPUSharedFenceMTLSharedEventDescriptor mtlEventDesc =
        WGPU_SHARED_FENCE_MTL_SHARED_EVENT_DESCRIPTOR_INIT;
    if (auto* mtlEventChain = unpacked.Get<SharedFenceMTLSharedEventDescriptor>()) {
        DAWN_INVALID_IF(mtlEventChain->sharedEvent == nullptr, "MTLSharedEvent is missing.");
        mtlEventDesc.sharedEvent = mtlEventChain->sharedEvent;
        innerDesc.nextInChain = &mtlEventDesc.chain;
    }
#endif

    const DawnProcTable& wgpu = device->wgpu.get();

    WGPUSharedFence innerHandle =
        wgpu.deviceImportSharedFence(device->GetInnerHandle(), &innerDesc);
    if (innerHandle == nullptr) {
        return DAWN_VALIDATION_ERROR(
            "sharedFenceImport failed on inner device. Fence chain is likely missing.");
    }

    WGPUSharedFenceExportInfo innerInfo = WGPU_SHARED_FENCE_EXPORT_INFO_INIT;
    wgpu.sharedFenceExportInfo(innerHandle, &innerInfo);
    if (innerInfo.type == 0) {
        return DAWN_VALIDATION_ERROR(
            "sharedFenceImport failed on inner device. Fence chain is likely missing.");
    }

    Ref<SharedFence> fence = AcquireRef(new SharedFence(device, label, descriptor, innerHandle));
#if DAWN_PLATFORM_IS(MACOS)
    if (auto* mtlEventChain = unpacked.Get<SharedFenceMTLSharedEventDescriptor>()) {
        fence->mSharedResourceHandle = mtlEventChain->sharedEvent;
    }
#endif
    return fence;
}

SharedFence::SharedFence(Device* device,
                         StringView label,
                         const SharedFenceDescriptor* descriptor,
                         WGPUSharedFence innerHandle)
    : SharedFenceBase(device, label),
      RecordableObject(schema::ObjectType::SharedFence),
      ObjectWGPU(device->wgpu->sharedFenceRelease) {
    mInnerHandle = innerHandle;
}

MaybeError SharedFence::ExportInfoImpl(UnpackedPtr<SharedFenceExportInfo>& info) const {
    WGPUSharedFenceExportInfo innerInfo = WGPU_SHARED_FENCE_EXPORT_INFO_INIT;

    // TODO(451310574): Handle all possible chained structures in SharedFenceExportInfo.
    // For now we only handle Metal as it is used in tests on macOS.
#if DAWN_PLATFORM_IS(MACOS)
    WGPUSharedFenceMTLSharedEventExportInfo mtlExportInfo =
        WGPU_SHARED_FENCE_MTL_SHARED_EVENT_EXPORT_INFO_INIT;
    if (info.Get<SharedFenceMTLSharedEventExportInfo>()) {
        innerInfo.nextInChain = &mtlExportInfo.chain;
    }
#endif

    const DawnProcTable& wgpu = ToBackend(GetDevice())->wgpu.get();
    wgpu.sharedFenceExportInfo(mInnerHandle, &innerInfo);

    info->type = static_cast<wgpu::SharedFenceType>(innerInfo.type);

#if DAWN_PLATFORM_IS(MACOS)
    if (auto* mtlExportInfoChain = info.Get<SharedFenceMTLSharedEventExportInfo>()) {
        mtlExportInfoChain->sharedEvent = mtlExportInfo.sharedEvent;
    }
#endif

    return {};
}

void SharedFence::SetLabelImpl() {
    ToBackend(GetDevice())->CaptureSetLabel(this, GetLabel());
}

MaybeError SharedFence::AddReferenced(CaptureContext& captureContext) {
    return {};
}

MaybeError SharedFence::CaptureCreationParameters(CaptureContext& captureContext) {
    // SharedFenceExportInfo info;
    // DAWN_TRY(ExportInfo(&info));
    // schema::SharedFence data{{
    //     .type = info.type,
    //     .handle = reinterpret_cast<uint64_t>(mSharedResourceHandle),
    // }};
    // Serialize(captureContext, data);
    return {};
}

}  // namespace dawn::native::webgpu
