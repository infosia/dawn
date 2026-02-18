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

#include "dawn/native/webgpu/SharedTextureMemoryWGPU.h"

#include <utility>
#include <vector>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"
#include "dawn/native/webgpu/SharedFenceWGPU.h"
#include "dawn/native/webgpu/TextureWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

// static
ResultOrError<Ref<SharedTextureMemory>> SharedTextureMemory::Create(
    Device* device,
    StringView label,
    const UnpackedPtr<SharedTextureMemoryDescriptor>& descriptor) {
    WGPUSharedTextureMemoryDescriptor innerDesc = WGPU_SHARED_TEXTURE_MEMORY_DESCRIPTOR_INIT;
    innerDesc.label = ToOutputStringView(label);

    // TODO(483147423): Handle all possible chained structures in SharedTextureMemoryDescriptor.
    // For now we only handle Metal.
    WGPUSharedTextureMemoryIOSurfaceDescriptor ioSurfaceDesc =
        WGPU_SHARED_TEXTURE_MEMORY_IO_SURFACE_DESCRIPTOR_INIT;
    if (auto* ioSurfaceChain = descriptor.Get<SharedTextureMemoryIOSurfaceDescriptor>()) {
        ioSurfaceDesc.ioSurface = ioSurfaceChain->ioSurface;
        ioSurfaceDesc.allowStorageBinding = ioSurfaceChain->allowStorageBinding;
        innerDesc.nextInChain = &ioSurfaceDesc.chain;
    }

    const DawnProcTable& wgpu = device->wgpu.get();

    WGPUSharedTextureMemory innerHandle =
        wgpu.deviceImportSharedTextureMemory(device->GetInnerHandle(), &innerDesc);
    if (innerHandle == nullptr) {
        return DAWN_VALIDATION_ERROR(
            "sharedTextureMemoryImport failed on inner device. Memory chain is likely missing.");
    }

    WGPUSharedTextureMemoryProperties innerProperties = WGPU_SHARED_TEXTURE_MEMORY_PROPERTIES_INIT;
    wgpu.sharedTextureMemoryGetProperties(innerHandle, &innerProperties);

    SharedTextureMemoryProperties properties;
    properties.usage = static_cast<wgpu::TextureUsage>(innerProperties.usage);
    properties.size = {innerProperties.size.width, innerProperties.size.height,
                       innerProperties.size.depthOrArrayLayers};
    properties.format = static_cast<wgpu::TextureFormat>(innerProperties.format);

    if (properties.format == wgpu::TextureFormat::Undefined) {
        return DAWN_VALIDATION_ERROR(
            "sharedTextureMemoryImport failed on inner device. Memory chain is likely missing.");
    }

    Ref<SharedTextureMemory> stm =
        AcquireRef(new SharedTextureMemory(device, label, descriptor, properties, innerHandle));

    if (auto* ioSurfaceChain = descriptor.Get<SharedTextureMemoryIOSurfaceDescriptor>()) {
        stm->mSharedResourceHandle = ioSurfaceChain->ioSurface;
        stm->mIOSurfaceDesc.ioSurface = ioSurfaceChain->ioSurface;
        stm->mIOSurfaceDesc.allowStorageBinding = ioSurfaceChain->allowStorageBinding;
    }

    stm->Initialize();
    return stm;
}

SharedTextureMemory::SharedTextureMemory(
    Device* device,
    StringView label,
    const UnpackedPtr<SharedTextureMemoryDescriptor>& descriptor,
    const SharedTextureMemoryProperties& properties,
    WGPUSharedTextureMemory innerHandle)
    : SharedTextureMemoryBase(device, label, properties),
      RecordableObject(schema::ObjectType::SharedTextureMemory),
      ObjectWGPU(device->wgpu->sharedTextureMemoryRelease) {
    mInnerHandle = innerHandle;
    DAWN_ASSERT(mInnerHandle);
}

ResultOrError<Ref<TextureBase>> SharedTextureMemory::CreateTextureImpl(
    const UnpackedPtr<TextureDescriptor>& descriptor) {
    return Texture::CreateFromSharedTextureMemory(this, descriptor);
}

ResultOrError<FenceAndSignalValue> SharedTextureMemory::EndAccessImpl(
    TextureBase* texture,
    ExecutionSerial lastUsageSerial,
    UnpackedPtr<EndAccessState>& state) {
    WGPUSharedTextureMemoryEndAccessState innerState =
        WGPU_SHARED_TEXTURE_MEMORY_END_ACCESS_STATE_INIT;

    // TODO(483147423): Handle all possible chained structures in EndAccessState.
    // For now we only handle Metal.
    WGPUSharedTextureMemoryMetalEndAccessState mtlEndState =
        WGPU_SHARED_TEXTURE_MEMORY_METAL_END_ACCESS_STATE_INIT;
    if (state.Get<SharedTextureMemoryMetalEndAccessState>()) {
        innerState.nextInChain = &mtlEndState.chain;
    }

    const DawnProcTable& wgpu = ToBackend(GetDevice())->wgpu.get();
    if (!wgpu.sharedTextureMemoryEndAccess(mInnerHandle, ToBackend(texture)->GetInnerHandle(),
                                           &innerState)) {
        return DAWN_INTERNAL_ERROR("sharedTextureMemoryEndAccess failed");
    }

    if (innerState.initialized) {
        texture->SetInitialized(true);
    }

    if (auto* mtlEndStateChain = state.Get<SharedTextureMemoryMetalEndAccessState>()) {
        if (lastUsageSerial != kBeginningOfGPUTime) {
            wgpu::Future future = ToBackend(GetDevice()->GetQueue())->GetCommandsScheduledFuture();
            mtlEndStateChain->commandsScheduledFuture = {future.id};
        } else {
            mtlEndStateChain->commandsScheduledFuture = {0};
        }
    }

    if (lastUsageSerial != kBeginningOfGPUTime) {
        // Texture is used by gpu.
        // The fence could only be from the inner queue.
        DAWN_ASSERT(innerState.fenceCount == 1);
        Ref<SharedFence> fence = SharedFence::CreateFromHandle(ToBackend(GetDevice()),
                                                               "Inner Fence", innerState.fences[0]);
        uint64_t signaledValue = innerState.signaledValues[0];

        // ?? GetOrCreateSharedFence
        ToBackend(GetDevice()->GetQueue())->AddSharedFence(ExecutionSerial(signaledValue), fence);

        return FenceAndSignalValue{std::move(fence), signaledValue};
    } else {
        // Texture is not used by gpu.
        // No extra queue fence should be exported.
        // Emit a nullptr fence which will be skipped in SharedResourceMemory::EndAccess by if
        // (fence.object)
        return FenceAndSignalValue{nullptr, 0};
    }
}

bool SharedTextureMemory::ShouldAlwaysCallEndAccessInternal() const {
    // Regardless of whether the resource is actually used, WebGPU backend needs to pass down the
    // EndAccess call to the inner handle. See SharedResourceMemory::EndAccess for where this
    // function is called.
    return true;
}

MaybeError SharedTextureMemory::BeginAccessImpl(
    TextureBase* texture,
    const UnpackedPtr<BeginAccessDescriptor>& descriptor) {
    WGPUSharedTextureMemoryBeginAccessDescriptor innerDesc =
        WGPU_SHARED_TEXTURE_MEMORY_BEGIN_ACCESS_DESCRIPTOR_INIT;
    innerDesc.concurrentRead = descriptor->concurrentRead;
    innerDesc.initialized = descriptor->initialized;

    std::vector<WGPUSharedFence> innerFences;
    innerFences.reserve(descriptor->fenceCount);
    for (size_t i = 0; i < descriptor->fenceCount; ++i) {
        innerFences.push_back(ToBackend(descriptor->fences[i])->GetInnerHandle());
    }
    innerDesc.fenceCount = innerFences.size();
    innerDesc.fences = innerFences.data();
    innerDesc.signaledValues = descriptor->signaledValues;

    const DawnProcTable& wgpu = ToBackend(GetDevice())->wgpu.get();
    if (!wgpu.sharedTextureMemoryBeginAccess(mInnerHandle, ToBackend(texture)->GetInnerHandle(),
                                             &innerDesc)) {
        return DAWN_INTERNAL_ERROR("sharedTextureMemoryBeginAccess failed");
    }

    return {};
}

void SharedTextureMemory::DestroyImpl(DestroyReason reason) {
    SharedTextureMemoryBase::DestroyImpl(reason);
    mSharedResourceHandle = nullptr;
}

void SharedTextureMemory::SetLabelImpl() {
    ToBackend(GetDevice())->CaptureSetLabel(this, GetLabel());
}

MaybeError SharedTextureMemory::AddReferenced(CaptureContext& captureContext) {
    // TODO(483147423): Implement Capture.
    return {};
}

MaybeError SharedTextureMemory::CaptureCreationParameters(CaptureContext& captureContext) {
    // TODO(483147423): Implement Capture.
    return {};
}

}  // namespace dawn::native::webgpu
