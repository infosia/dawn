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

#include <webgpu/webgpu.h>

#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/BackendConnection.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/Commands.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/Instance.h"
#include "dawn/native/Surface.h"
#include "dawn/native/TintUtils.h"
// #include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/PhysicalDeviceWGPU.h"
#include "partition_alloc/pointers/raw_ptr.h"

#include "tint/tint.h"

namespace dawn::native::webgpu {

// struct CopyFromStagingToBufferOperation : PendingOperation {
//     void Execute() override {
//         destination->CopyFromStaging(staging.Get(), sourceOffset, destinationOffset, size);
//     }

//     Ref<BufferBase> staging;
//     Ref<Buffer> destination;
//     uint64_t sourceOffset;
//     uint64_t destinationOffset;
//     uint64_t size;
// };

// Device

// static
ResultOrError<Ref<Device>> Device::Create(AdapterBase* adapter,
                                          const UnpackedPtr<DeviceDescriptor>& descriptor,
                                          const TogglesState& deviceToggles,
                                          Ref<DeviceBase::DeviceLostEvent>&& lostEvent) {
    Ref<Device> device =
        AcquireRef(new Device(adapter, descriptor, deviceToggles, std::move(lostEvent)));
    DAWN_TRY(device->Initialize(descriptor));
    return device;
}

Device::Device(AdapterBase* adapter,
               const UnpackedPtr<DeviceDescriptor>& descriptor,
               const TogglesState& deviceToggles,
               Ref<DeviceBase::DeviceLostEvent>&& lostEvent)
    : DeviceBase(adapter, descriptor, deviceToggles, std::move(lostEvent)) {
    auto options = ToBackend(GetPhysicalDevice())->GetRequestAdapterOptions();
    auto adapters = adapter->GetInstance()->EnumerateAdapters(options);
    // The adapters vector is sorted by SortAdapters in Adapter.cpp
    // in the order of Core -> Compat -> Testing while preferring OS-specific backends.
    auto adapterImpl = std::move(adapters[0]);

    // Make sure the selected adapter is not WebGPU backend again.
    DAWN_ASSERT(adapterImpl->GetPhysicalDevice()->GetBackendType() != wgpu::BackendType::WebGPU);

    WGPUDeviceDescriptor apiDesc = *(ToAPI(*descriptor));
    std::string label = "API Device [" + adapterImpl->GetName() + "]";
    apiDesc.label = ToOutputStringView(label);
    apiDesc.deviceLostCallbackInfo = {
        nullptr,
        // TODO(crbug.com/413053623): need to set to allow spontaneous to avoid the dangling pointer
        // when called after deviceWGPU is released for now.
        WGPUCallbackMode_AllowSpontaneous,
        [](WGPUDevice const* device, WGPUDeviceLostReason reason, WGPUStringView message,
           void* callbackParam, void* deviceParam) {
            if (reason == WGPUDeviceLostReason_Unknown) {
                // Internal crash of the implementation device, call device lost callback of the
                // upper DeviceWGPU
                auto deviceWGPU = reinterpret_cast<Device*>(deviceParam);
                if (deviceWGPU && !deviceWGPU->IsLost()) {
                    deviceWGPU->APIForceLoss(FromAPI(reason), message);
                }
            }
        },
        nullptr, reinterpret_cast<void*>(this)};
    // TODO(crbug.com/413053623): revisit for error scope.
    apiDesc.uncapturedErrorCallbackInfo = {nullptr, nullptr, nullptr, nullptr};

    // TODO(crbug.com/413053623): use adapterRequestDevice instead as dawn_wire doesn't support
    // adapterCreateDevice.
    mApiDevice = GetFunctions().adapterCreateDevice(ToAPI(adapterImpl.Get()), &apiDesc);
}

Device::~Device() {
    Destroy();
}

const DawnProcTable& Device::GetFunctions() const {
    return ToBackend(GetPhysicalDevice())->GetFunctions();
}

WGPUDevice Device::GetApiDevice() const {
    return mApiDevice;
}

MaybeError Device::Initialize(const UnpackedPtr<DeviceDescriptor>& descriptor) {
    return DeviceBase::Initialize(AcquireRef(new Queue(this, &descriptor->defaultQueue)));
}

ResultOrError<Ref<BindGroupBase>> Device::CreateBindGroupImpl(
    const BindGroupDescriptor* descriptor) {
    Ref<BindGroup> bindGroup = AcquireRef(new BindGroup(this, descriptor));
    DAWN_TRY(bindGroup->Initialize(descriptor));
    return bindGroup;
}
ResultOrError<Ref<BindGroupLayoutInternalBase>> Device::CreateBindGroupLayoutImpl(
    const BindGroupLayoutDescriptor* descriptor) {
    return AcquireRef(new BindGroupLayout(this, descriptor));
}
ResultOrError<Ref<BufferBase>> Device::CreateBufferImpl(
    const UnpackedPtr<BufferDescriptor>& descriptor) {
    return Buffer::Create(this, descriptor);
}
ResultOrError<Ref<CommandBufferBase>> Device::CreateCommandBuffer(
    CommandEncoder* encoder,
    const CommandBufferDescriptor* descriptor) {
    // This is called by CommandEncoder::Finish
    // TODO(crbug.com/413053623): Store CommandEncoderDescriptor and assign here.
    WGPUCommandEncoder apiEncoder = ToBackend(encoder->GetDevice())
                                        ->GetFunctions()
                                        .deviceCreateCommandEncoder(mApiDevice, nullptr);

    return AcquireRef(new CommandBuffer(encoder, descriptor, apiEncoder));
}
Ref<ComputePipelineBase> Device::CreateUninitializedComputePipelineImpl(
    const UnpackedPtr<ComputePipelineDescriptor>& descriptor) {
    return AcquireRef(new ComputePipeline(this, descriptor));
}
ResultOrError<Ref<PipelineLayoutBase>> Device::CreatePipelineLayoutImpl(
    const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) {
    return AcquireRef(new PipelineLayout(this, descriptor));
}
ResultOrError<Ref<QuerySetBase>> Device::CreateQuerySetImpl(const QuerySetDescriptor* descriptor) {
    return AcquireRef(new QuerySet(this, descriptor));
}
Ref<RenderPipelineBase> Device::CreateUninitializedRenderPipelineImpl(
    const UnpackedPtr<RenderPipelineDescriptor>& descriptor) {
    return AcquireRef(new RenderPipeline(this, descriptor));
}
ResultOrError<Ref<SamplerBase>> Device::CreateSamplerImpl(const SamplerDescriptor* descriptor) {
    return AcquireRef(new Sampler(this, descriptor));
}
ResultOrError<Ref<ShaderModuleBase>> Device::CreateShaderModuleImpl(
    const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
    const std::vector<tint::wgsl::Extension>& internalExtensions,
    ShaderModuleParseResult* parseResult,
    std::unique_ptr<OwnedCompilationMessages>* compilationMessages) {
    Ref<ShaderModule> module = AcquireRef(new ShaderModule(this, descriptor, internalExtensions));
    DAWN_TRY(module->Initialize(parseResult, compilationMessages));
    return module;
}
ResultOrError<Ref<SwapChainBase>> Device::CreateSwapChainImpl(Surface* surface,
                                                              SwapChainBase* previousSwapChain,
                                                              const SurfaceConfiguration* config) {
    return SwapChain::Create(this, surface, previousSwapChain, config);
}
ResultOrError<Ref<TextureBase>> Device::CreateTextureImpl(
    const UnpackedPtr<TextureDescriptor>& descriptor) {
    return AcquireRef(new Texture(this, descriptor));
}
ResultOrError<Ref<TextureViewBase>> Device::CreateTextureViewImpl(
    TextureBase* texture,
    const UnpackedPtr<TextureViewDescriptor>& descriptor) {
    return AcquireRef(new TextureView(texture, descriptor));
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

    if (mApiDevice) {
        GetFunctions().deviceRelease(mApiDevice);
        mApiDevice = nullptr;
    }
}

MaybeError Device::CopyFromStagingToBufferImpl(BufferBase* source,
                                               uint64_t sourceOffset,
                                               BufferBase* destination,
                                               uint64_t destinationOffset,
                                               uint64_t size) {
    DAWN_UNREACHABLE();
}

MaybeError Device::CopyFromStagingToTextureImpl(const BufferBase* source,
                                                const TexelCopyBufferLayout& src,
                                                const TextureCopy& dst,
                                                const Extent3D& copySizePixels) {
    DAWN_UNREACHABLE();
}

MaybeError Device::TickImpl() {
    return FromAPI(mApiDevice)->Tick();
}

uint32_t Device::GetOptimalBytesPerRowAlignment() const {
    return FromAPI(mApiDevice)->GetOptimalBytesPerRowAlignment();
}

uint64_t Device::GetOptimalBufferToTextureCopyOffsetAlignment() const {
    return FromAPI(mApiDevice)->GetOptimalBufferToTextureCopyOffsetAlignment();
}

float Device::GetTimestampPeriodInNS() const {
    return FromAPI(mApiDevice)->GetTimestampPeriodInNS();
}

bool Device::CanTextureLoadResolveTargetInTheSameRenderpass() const {
    return FromAPI(mApiDevice)->CanTextureLoadResolveTargetInTheSameRenderpass();
}

// WebGPUBackend objects, mostly placeholders for now.
// TODO(crbug.com/413053623): move the objects to separate files when implementing them.

// Buffer

// static
ResultOrError<Ref<Buffer>> Buffer::Create(Device* device,
                                          const UnpackedPtr<BufferDescriptor>& descriptor) {
    auto desc = ToAPI(*descriptor);
    WGPUBuffer apiBuffer = device->GetFunctions().deviceCreateBuffer(device->GetApiDevice(), desc);

    Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor, apiBuffer));
    return std::move(buffer);
}

Buffer::Buffer(Device* device,
               const UnpackedPtr<BufferDescriptor>& descriptor,
               WGPUBuffer apiBuffer)
    : BufferBase(device, descriptor), mApiBuffer(apiBuffer) {
    // mBackingData = std::unique_ptr<uint8_t[]>(new uint8_t[GetSize()]);
    mAllocatedSize = GetSize();
}

WGPUBuffer Buffer::GetApiBuffer() const {
    return mApiBuffer;
}

bool Buffer::IsCPUWritableAtCreation() const {
    // Only return true for mappable buffers so we can test cases that need / don't need a
    // staging buffer.
    return GetInternalUsage() & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite);
}

MaybeError Buffer::MapAtCreationImpl() {
    return {};
}

void Buffer::CopyFromStaging(BufferBase* staging,
                             uint64_t sourceOffset,
                             uint64_t destinationOffset,
                             uint64_t size) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(staging->GetMappedPointer());
    memcpy(mBackingData.get() + destinationOffset, ptr + sourceOffset, size);
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    GetDevice()->GetQueue()->IncrementLastSubmittedCommandSerial();

    WGPUBufferMapCallbackInfo apiCallbackInfo = {};
    apiCallbackInfo.mode = mCallbackInfo.mode;
    apiCallbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                                  void* callback_param, void* userdata_param) {
        // auto buffer = reinterpret_cast<BufferBase*>(userdata_param);
        // if (status == WGPUMapAsyncStatus_Success) {
        //     // temp, need to set first before CanGetMappedRange
        //     // buffer->mMapOffset = offset;
        //     // buffer->mMapSize = size;
        //     buffer->mState = BufferState::PendingMap;
        //     buffer->mMapOffset = 0;
        //     buffer->mMapSize = 4;   // temp
        // }

        // auto cb = reinterpret_cast<WGPUBufferMapCallbackInfo*>(callback_param);
        // // (*cb)(static_cast<MapAsyncStatus>(status), {detail::StringViewAdapter(message)},
        // static_cast<T>(userdata_param));
        // (*cb->callback)(
        //     status,
        //     // detail::StringViewAdapter(message),
        //     message,
        //     cb->userdata1,
        //     cb->userdata2,
        // );
    };
    apiCallbackInfo.userdata1 = &mCallbackInfo;
    apiCallbackInfo.userdata2 = this;

    // wait on future
    WGPUFutureWaitInfo waitInfo = {};
    waitInfo.future = ToBackend(GetDevice())
                          ->GetFunctions()
                          .bufferMapAsync(mApiBuffer, static_cast<WGPUMapMode>(mode), offset, size,
                                          apiCallbackInfo);
    ToBackend(GetDevice())
        ->GetFunctions()
        .instanceWaitAny(ToAPI(GetDevice()->GetInstance()), 1, &waitInfo, UINT64_MAX);

    return {};
}

void* Buffer::GetMappedPointer() {
    // return mBackingData.get();
    return const_cast<void*>(
        ToBackend(GetDevice())->GetFunctions().bufferGetConstMappedRange(mApiBuffer, 0, GetSize()));
}

void Buffer::UnmapImpl() {}

void Buffer::DestroyImpl() {
    // TODO(crbug.com/dawn/831): DestroyImpl is called from two places.
    // - It may be called if the buffer is explicitly destroyed with APIDestroy.
    //   This case is NOT thread-safe and needs proper synchronization with other
    //   simultaneous uses of the buffer.
    // - It may be called when the last ref to the buffer is dropped and the buffer
    //   is implicitly destroyed. This case is thread-safe because there are no
    //   other threads using the buffer since there are no other live refs.

    if (mApiBuffer) {
        ToBackend(GetDevice())->GetFunctions().bufferRelease(mApiBuffer);
        mApiBuffer = nullptr;
    }

    BufferBase::DestroyImpl();
}

// CommandBuffer

CommandBuffer::CommandBuffer(CommandEncoder* encoder,
                             const CommandBufferDescriptor* descriptor,
                             WGPUCommandEncoder apiEncoder)
    : CommandBufferBase(encoder, descriptor), mApiEncoder(apiEncoder) {}

CommandBuffer::~CommandBuffer() {
    if (mApiEncoder) {
        ToBackend(GetDevice())->GetFunctions().commandEncoderRelease(mApiEncoder);
        mApiEncoder = nullptr;
    }
}

WGPUCommandBuffer CommandBuffer::Execute() {
    Command type;
    while (mCommands.NextCommandId(&type)) {
        switch (type) {
            case Command::CopyBufferToBuffer: {
                CopyBufferToBufferCmd* copy = mCommands.NextCommand<CopyBufferToBufferCmd>();
                ToBackend(GetDevice())
                    ->GetFunctions()
                    .commandEncoderCopyBufferToBuffer(
                        mApiEncoder, ToBackend(copy->source)->GetApiBuffer(), copy->sourceOffset,
                        ToBackend(copy->destination)->GetApiBuffer(), copy->destinationOffset,
                        copy->size);
                break;
            }
            default:
                DAWN_UNREACHABLE();
        }
    }

    return ToBackend(GetDevice())
        ->GetFunctions()
        .commandEncoderFinish(mApiEncoder, nullptr);  // store descriptor?
}

// Queue

Queue::Queue(Device* device, const QueueDescriptor* descriptor) : QueueBase(device, descriptor) {
    mApiQueue = device->GetFunctions().deviceGetQueue(device->GetApiDevice());
}

Queue::~Queue() {
    if (mApiQueue) {
        ToBackend(GetDevice())->GetFunctions().queueRelease(mApiQueue);
        mApiQueue = nullptr;
    }
}

MaybeError Queue::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
    if (commandCount == 0 || commands == nullptr) {
        return {};
    }

    auto procs = ToBackend(GetDevice())->GetFunctions();

    std::vector<WGPUCommandBuffer> apiCommands(commandCount);
    for (uint32_t i = 0; i < commandCount; ++i) {
        apiCommands[i] = ToBackend(commands[i])->Execute();
    }

    ToBackend(GetDevice())->GetFunctions().queueSubmit(mApiQueue, commandCount, apiCommands.data());

    for (uint32_t i = 0; i < commandCount; ++i) {
        procs.commandBufferRelease(apiCommands[i]);
    }

    return {};
}

MaybeError Queue::WriteBufferImpl(BufferBase* buffer,
                                  uint64_t bufferOffset,
                                  const void* data,
                                  size_t size) {
    auto apiBuffer = ToBackend(buffer)->GetApiBuffer();
    ToBackend(GetDevice())
        ->GetFunctions()
        .queueWriteBuffer(mApiQueue, apiBuffer, bufferOffset, data, size);
    return {};
}

ResultOrError<ExecutionSerial> Queue::CheckAndUpdateCompletedSerials() {
    return GetLastSubmittedCommandSerial();
}

void Queue::ForceEventualFlushOfCommands() {}

bool Queue::HasPendingCommands() const {
    return false;
}

MaybeError Queue::SubmitPendingCommands() {
    return {};
}

ResultOrError<bool> Queue::WaitForQueueSerial(ExecutionSerial serial, Nanoseconds timeout) {
    return true;
}

MaybeError Queue::WaitForIdleForDestruction() {
    return FromAPI(mApiQueue)->WaitForIdleForDestruction();
}

// BindGroupDataHolder

BindGroupDataHolder::BindGroupDataHolder(size_t size)
    : mBindingDataAllocation(malloc(size))  // malloc is guaranteed to return a
                                            // pointer aligned enough for the allocation
{}

BindGroupDataHolder::~BindGroupDataHolder() {
    free(mBindingDataAllocation.ExtractAsDangling());
}

// BindGroup

BindGroup::BindGroup(DeviceBase* device, const BindGroupDescriptor* descriptor)
    : BindGroupDataHolder(descriptor->layout->GetInternalBindGroupLayout()->GetBindingDataSize()),
      BindGroupBase(device, descriptor, mBindingDataAllocation) {}

MaybeError BindGroup::InitializeImpl() {
    return {};
}

// BindGroupLayout

BindGroupLayout::BindGroupLayout(DeviceBase* device, const BindGroupLayoutDescriptor* descriptor)
    : BindGroupLayoutInternalBase(device, descriptor) {}

// QuerySet

QuerySet::QuerySet(Device* device, const QuerySetDescriptor* descriptor)
    : QuerySetBase(device, descriptor) {}

// ComputePipeline
MaybeError ComputePipeline::InitializeImpl() {
    const ProgrammableStage& computeStage = GetStage(SingleShaderStage::Compute);

    // Convert the AST program to an IR module.
    auto ir =
        tint::wgsl::reader::ProgramToLoweredIR(computeStage.module->GetTintProgram()->program);
    DAWN_INVALID_IF(ir != tint::Success, "An error occurred while generating Tint IR\n%s",
                    ir.Failure().reason);

    auto singleEntryPointResult =
        tint::core::ir::transform::SingleEntryPoint(ir.Get(), computeStage.entryPoint.c_str());
    DAWN_INVALID_IF(singleEntryPointResult != tint::Success,
                    "Pipeline single entry point (IR) failed:\n%s",
                    singleEntryPointResult.Failure().reason);

    // this needs to run after SingleEntryPoint transform which removes unused
    // overrides for the current entry point.
    tint::core::ir::transform::SubstituteOverridesConfig cfg;
    cfg.map = BuildSubstituteOverridesTransformConfig(computeStage);

    auto substituteOverridesResult = tint::core::ir::transform::SubstituteOverrides(ir.Get(), cfg);
    DAWN_INVALID_IF(substituteOverridesResult != tint::Success,
                    "Pipeline override substitution (IR) failed:\n%s",
                    substituteOverridesResult.Failure().reason);

    auto limits = LimitsForCompilationRequest::Create(GetDevice()->GetLimits().v1);
    auto adapterSupportedLimits =
        LimitsForCompilationRequest::Create(GetDevice()->GetAdapter()->GetLimits().v1);
    auto maxSubgroupSize = GetDevice()->GetAdapter()->GetPhysicalDevice()->GetSubgroupMaxSize();

    //  Workgroup validation has to come after overrides to have been substituted.
    auto wgInfo = tint::core::ir::GetWorkgroupInfo(ir.Get());

    DAWN_INVALID_IF(wgInfo != tint::Success, "Getting workgroup info has failed (IR):\n%s",
                    wgInfo.Failure().reason);

    Extent3D _;
    DAWN_TRY_ASSIGN(_, ValidateComputeStageWorkgroupSize(
                           wgInfo.Get().x, wgInfo.Get().y, wgInfo.Get().z,
                           wgInfo.Get().storage_size, computeStage.metadata->usesSubgroupMatrix,
                           maxSubgroupSize, limits, adapterSupportedLimits));

    return {};
}

// RenderPipeline
MaybeError RenderPipeline::InitializeImpl() {
    return {};
}

// SwapChain

// static
ResultOrError<Ref<SwapChain>> SwapChain::Create(Device* device,
                                                Surface* surface,
                                                SwapChainBase* previousSwapChain,
                                                const SurfaceConfiguration* config) {
    Ref<SwapChain> swapchain = AcquireRef(new SwapChain(device, surface, config));
    DAWN_TRY(swapchain->Initialize(previousSwapChain));
    return swapchain;
}

MaybeError SwapChain::Initialize(SwapChainBase* previousSwapChain) {
    if (previousSwapChain != nullptr) {
        // TODO(crbug.com/dawn/269): figure out what should happen when surfaces are used by
        // multiple backends one after the other. It probably needs to block until the backend
        // and GPU are completely finished with the previous swapchain.
        DAWN_INVALID_IF(previousSwapChain->GetBackendType() != wgpu::BackendType::WebGPU,
                        "webgpu::SwapChain cannot switch between APIs");
    }

    return {};
}

SwapChain::~SwapChain() = default;

MaybeError SwapChain::PresentImpl() {
    mTexture->APIDestroy();
    mTexture = nullptr;
    return {};
}

ResultOrError<SwapChainTextureInfo> SwapChain::GetCurrentTextureImpl() {
    TextureDescriptor textureDesc = GetSwapChainBaseTextureDescriptor(this);
    mTexture = AcquireRef(new Texture(GetDevice(), Unpack(&textureDesc)));
    SwapChainTextureInfo info;
    info.texture = mTexture;
    info.status = wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal;
    return info;
}

void SwapChain::DetachFromSurfaceImpl() {
    if (mTexture != nullptr) {
        mTexture->APIDestroy();
        mTexture = nullptr;
    }
}

// ShaderModule

MaybeError ShaderModule::Initialize(
    ShaderModuleParseResult* parseResult,
    std::unique_ptr<OwnedCompilationMessages>* compilationMessages) {
    return InitializeBase(parseResult, compilationMessages);
}

// Texture

Texture::Texture(DeviceBase* device, const UnpackedPtr<TextureDescriptor>& descriptor)
    : TextureBase(device, descriptor) {}

}  // namespace dawn::native::webgpu
