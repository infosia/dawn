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

#include "dawn/native/webgpu/DeviceWGPU.h"

#include <string>
#include <utility>

#include "dawn/common/Constants.h"
#include "dawn/common/StringViewUtils.h"
#include "dawn/native/BackendConnection.h"
#include "dawn/native/BindGroup.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Buffer.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/Instance.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/QuerySet.h"
#include "dawn/native/Queue.h"
#include "dawn/native/RenderPipeline.h"
#include "dawn/native/Sampler.h"
#include "dawn/native/Surface.h"
#include "dawn/native/SwapChain.h"
#include "dawn/native/Texture.h"
#include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/CommandBufferWGPU.h"
#include "dawn/native/webgpu/PhysicalDeviceWGPU.h"
#include "dawn/native/webgpu/QueueWGPU.h"
#include "dawn/native/webgpu/RecordCommandSerializer.h"

#include "tint/tint.h"

namespace dawn::native::webgpu {

// static
ResultOrError<Ref<Device>> Device::Create(AdapterBase* adapter,
                                          WGPUAdapter innerAdapter,
                                          const UnpackedPtr<DeviceDescriptor>& descriptor,
                                          const TogglesState& deviceToggles,
                                          Ref<DeviceBase::DeviceLostEvent>&& lostEvent) {
    Ref<Device> device = AcquireRef(
        new Device(adapter, innerAdapter, descriptor, deviceToggles, std::move(lostEvent)));
    DAWN_TRY(device->Initialize(descriptor));
    return device;
}

Device::Device(AdapterBase* adapter,
               WGPUAdapter innerAdapter,
               const UnpackedPtr<DeviceDescriptor>& descriptor,
               const TogglesState& deviceToggles,
               Ref<DeviceBase::DeviceLostEvent>&& lostEvent)
    : DeviceBase(adapter, descriptor, deviceToggles, std::move(lostEvent)),
      wgpu(ToBackend(adapter->GetPhysicalDevice())->GetFunctions()),
      mDeviceHandle(ReserveHandle())
      {
    DAWN_ASSERT(adapter->GetPhysicalDevice()->GetBackendType() == wgpu::BackendType::WebGPU);

    mCommandSerializerImpl = std::make_unique<RecordCommandSerializer>();
    // mDeviceHandle = ReserveHandle();

    // link error
    // mSerializer = std::make_unique<dawn::wire::ChunkedCommandSerializer>(mCommandSerializerImpl.get());

    // mObjectIdProvider = std::make_unique<DeviceObjectIdProvider>(this);

    WGPUDeviceDescriptor apiDesc = *(ToAPI(*descriptor));
    std::string label = "Inner Device on " + adapter->GetPhysicalDevice()->GetName();
    apiDesc.label = ToOutputStringView(label);
    // Acquire a Ref to the outer webgpu::Device to avoid possible dangling pointer in the callback.
    Ref<Device>* outerDeviceRef = new Ref<Device>(this);
    apiDesc.deviceLostCallbackInfo = {
        nullptr, WGPUCallbackMode_AllowProcessEvents,
        [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*,
           void* outerDeviceRef) {
            Ref<Device>* deviceWGPURef = reinterpret_cast<Ref<Device>*>(outerDeviceRef);
            Device* deviceWGPU = deviceWGPURef->Get();
            if (reason == WGPUDeviceLostReason_Unknown) {
                // Internal crash of the implementation device, call device lost callback of the
                // upper DeviceWGPU
                if (!deviceWGPU->IsLost()) {
                    deviceWGPU->APIForceLoss(FromAPI(reason), message);
                }
            }
            delete deviceWGPURef;
        },
        nullptr, reinterpret_cast<void*>(outerDeviceRef)};
    // TODO(crbug.com/413053623): revisit for error scope.
    apiDesc.uncapturedErrorCallbackInfo = {nullptr, nullptr, nullptr, nullptr};

    // TODO(crbug.com/413053623): use adapterRequestDevice instead as dawn_wire doesn't support
    // adapterCreateDevice.
    mInnerDevice = wgpu.adapterCreateDevice(innerAdapter, &apiDesc);
}

Device::~Device() {
    Destroy();
}

WGPUDevice Device::GetInnerHandle() const {
    return mInnerDevice;
}

dawn::wire::ObjectHandle Device::ReserveHandle() {
    // std::lock_guard<Mutex> lock(mObjectHandleMutex);
    std::lock_guard<std::mutex> lock(mObjectHandleMutex);
    // Simple generation management: always use 1 for new objects.
    // A full ObjectStore would handle generation reuse more carefully.
    return dawn::wire::ObjectHandle(mNextObjectId++, 1);
}

WGPUInstance Device::GetInnerInstance() const {
    return ToBackend(GetPhysicalDevice())->GetBackend()->GetInnerInstance();
}

MaybeError Device::Initialize(const UnpackedPtr<DeviceDescriptor>& descriptor) {
    Ref<Queue> queue;
    DAWN_TRY_ASSIGN(queue, Queue::Create(this, &descriptor->defaultQueue));
    return DeviceBase::Initialize(descriptor, std::move(queue));
}

ResultOrError<Ref<BindGroupBase>> Device::CreateBindGroupImpl(
    const BindGroupDescriptor* descriptor) {
    return Ref<BindGroupBase>{nullptr};
}
ResultOrError<Ref<BindGroupLayoutInternalBase>> Device::CreateBindGroupLayoutImpl(
    const BindGroupLayoutDescriptor* descriptor) {
    // TODO(crbug.com/413053623): Replace with webgpu::BindGroupLayout object.
    // This placeholder implementation is needed because device is creating empty BindGroupLayout
    // and getting content hash.
    return AcquireRef(new BindGroupLayoutInternalBase(this, descriptor));
}
ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(
    const UnpackedPtr<BufferDescriptor>& descriptor) {
    return Buffer::Create(this, descriptor);

    // ResultOrError<Ref<webgpu::Buffer>> result = Buffer::Create(this, descriptor);
    // if (result.IsSuccess()) {
    //     Ref<webgpu::Buffer> buffer = result.AcquireSuccess();

    //     dawn::wire::DeviceCreateBufferCmd cmd;
    //     cmd.deviceId = mDeviceH
    //     cmd.descriptor = ToAPI(*descriptor); // Careful with lifetime
    //     cmd.result = GetOrCreateHandle(buffer.Get());
    //     cmd.readHandleCreateInfoLength = 0;
    //     cmd.readHandleCreateInfo = nullptr;
    //     cmd.writeHandleCreateInfoLength = 0;
    //     cmd.writeHandleCreateInfo = nullptr;

    //     // The ObjectIdProvider should be `this` or a member that can resolve handles.
    //     // For this example, we pass `*mObjectIdProvider.get()`
    //     // mSerializer->SerializeCommand(cmd, *mObjectIdProvider.get());
    //     mSerializer->SerializeCommand(cmd);
    //     // mSerializer->Flush(); // Or batch flushes

    //     return static_cast<Ref<BufferBase>>(buffer);
    // }
    // return result.AcquireError();
}
ResultOrError<Ref<CommandBufferBase>> Device::CreateCommandBuffer(
    CommandEncoder* encoder,
    const CommandBufferDescriptor* descriptor) {
    // This is called by CommandEncoder::Finish
    // TODO(crbug.com/413053623): Store CommandEncoderDescriptor and assign here.
    return CommandBuffer::Create(encoder, descriptor);
}
Ref<ComputePipelineBase> Device::CreateUninitializedComputePipelineImpl(
    const UnpackedPtr<ComputePipelineDescriptor>& descriptor) {
    return Ref<ComputePipelineBase>{nullptr};
}
ResultOrError<Ref<PipelineLayoutBase>> Device::CreatePipelineLayoutImpl(
    const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) {
    // TODO(crbug.com/413053623): Replace with webgpu::PipelineLayout object.
    // This placeholder implementation is needed because device is creating empty PipelineLayout
    // and getting content hash.
    return AcquireRef(new PipelineLayoutBase(this, descriptor));
}
ResultOrError<Ref<QuerySetBase>> Device::CreateQuerySetImpl(const QuerySetDescriptor* descriptor) {
    return Ref<QuerySetBase>{nullptr};
}
Ref<RenderPipelineBase> Device::CreateUninitializedRenderPipelineImpl(
    const UnpackedPtr<RenderPipelineDescriptor>& descriptor) {
    return Ref<RenderPipelineBase>{nullptr};
}
ResultOrError<Ref<SamplerBase>> Device::CreateSamplerImpl(const SamplerDescriptor* descriptor) {
    return Ref<SamplerBase>{nullptr};
}
ResultOrError<Ref<ShaderModuleBase>> Device::CreateShaderModuleImpl(
    const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
    const std::vector<tint::wgsl::Extension>& internalExtensions,
    ShaderModuleParseResult* parseResult) {
    return Ref<ShaderModuleBase>{nullptr};
}
ResultOrError<Ref<SwapChainBase>> Device::CreateSwapChainImpl(Surface* surface,
                                                              SwapChainBase* previousSwapChain,
                                                              const SurfaceConfiguration* config) {
    return Ref<SwapChainBase>{nullptr};
}
ResultOrError<Ref<TextureBase>> Device::CreateTextureImpl(
    const UnpackedPtr<TextureDescriptor>& descriptor) {
    return Ref<TextureBase>{nullptr};
}
ResultOrError<Ref<TextureViewBase>> Device::CreateTextureViewImpl(
    TextureBase* texture,
    const UnpackedPtr<TextureViewDescriptor>& descriptor) {
    return Ref<TextureViewBase>{nullptr};
}

void Device::DestroyImpl() {
    DAWN_ASSERT(GetState() == State::Disconnected);
    // TODO(crbug.com/dawn/831): DestroyImpl is called from two places.
    // - It may be called if the device is explicitly destroyed with APIDestroy.
    //   This case is NOT thread-safe and needs proper synchronization with other
    //   simultaneous uses of the device.
    // - It may be called when the last ref to the device is dropped and the device
    //   is implicitly destroyed. This case is thread-safe because there are no
    //   other threads using the device since there are no other live refs.

    if (mInnerDevice) {
        // webgpu.h guarantees that losing this reference will cause all internal resources to be
        // freed and wait on the GPU if needed to do so.
        wgpu.deviceRelease(mInnerDevice);
        mInnerDevice = nullptr;
    }
}

MaybeError Device::CopyFromStagingToBuffer(BufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) {
    return DAWN_UNIMPLEMENTED_ERROR("webgpu::Device implementation is incomplete.");
}

MaybeError Device::CopyFromStagingToTextureImpl(const BufferBase* source,
                                                const TexelCopyBufferLayout& src,
                                                const TextureCopy& dst,
                                                const Extent3D& copySizePixels) {
    return DAWN_UNIMPLEMENTED_ERROR("webgpu::Device implementation is incomplete.");
}

MaybeError Device::TickImpl() {
    wgpu.deviceTick(mInnerDevice);
    return {};
}

uint32_t Device::GetOptimalBytesPerRowAlignment() const {
    return kTextureBytesPerRowAlignment;
}

uint64_t Device::GetOptimalBufferToTextureCopyOffsetAlignment() const {
    return 1;
}

float Device::GetTimestampPeriodInNS() const {
    return 1.0f;
}

// dawn::wire::ObjectHandle Device::GetOrCreateHandle(const ApiObjectBase* obj) {
//     // std::lock_guard<Mutex> lock(mObjectHandleMutex);
//     auto it = mObjectHandles.find(obj);
//     if (it != mObjectHandles.end()) {
//         return it->second;
//     }
//     dawn::wire::ObjectHandle handle(mNextObjectId++, 1); // Generation 1 for simplicity
//     mObjectHandles.emplace(obj, handle);
//     return handle;
// }

// dawn::wire::ObjectIdProvider* Device::GetObjectIdProvider() {
//     return mObjectIdProvider.get();
// }

// // DeviceObjectIdProvider implementation
// template <typename T>
// dawn::wire::WireResult Device::DeviceObjectIdProvider::GetIdImpl(T, dawn::wire::ObjectId*) const {
//     // Default: Not implemented for this type for this simplified example
//     // A full implementation would need to handle all WGPU object types.
//     DAWN_UNREACHABLE();
//     return dawn::wire::WireResult::FatalError;
// }

// dawn::wire::WireResult Device::DeviceObjectIdProvider::GetId(WGPUBuffer cBuffer, dawn::wire::ObjectId* out) const {
//     // This is tricky because WGPUBuffer is an opaque handle.
//     // We'd need to map it back to our ApiObjectBase* or have a direct map from WGPUBuffer -> ObjectId
//     // For this example, we'll assume we can get the ApiObjectBase* somehow if needed,
//     // or that the caller (QueueWGPU) passes the ApiObjectBase*.
//     // Let's adjust GetOrCreateHandle to be callable from Queue.
//     // For now, this specific GetId will be simplified.
//     // In a real scenario, the object passed to ObjectIdProvider::GetId would be the Dawn object.
//     // Here, we're in the native backend, so we'd look up the handle for our *native* object.
//     return mDevice->GetId(FromAPI(cBuffer), out);
// }

}  // namespace dawn::native::webgpu
