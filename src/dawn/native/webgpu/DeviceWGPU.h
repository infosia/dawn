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

#ifndef SRC_DAWN_NATIVE_WEBGPU_DEVICEWGPU_H_
#define SRC_DAWN_NATIVE_WEBGPU_DEVICEWGPU_H_

#include <memory>
#include <vector>

#include "dawn/native/BindGroup.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/Buffer.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/ComputePipeline.h"
#include "dawn/native/Device.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/PipelineLayout.h"
#include "dawn/native/QuerySet.h"
#include "dawn/native/Queue.h"
#include "dawn/native/RenderPipeline.h"
#include "dawn/native/RingBufferAllocator.h"
#include "dawn/native/Sampler.h"
#include "dawn/native/ShaderModule.h"
#include "dawn/native/SwapChain.h"
#include "dawn/native/Texture.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/webgpu/Forward.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native::webgpu {

class Device final : public DeviceBase {
  public:
    static ResultOrError<Ref<Device>> Create(AdapterBase* adapter,
                                             WGPUAdapter innerAdapter,
                                             const UnpackedPtr<DeviceDescriptor>& descriptor,
                                             const TogglesState& deviceToggles,
                                             Ref<DeviceBase::DeviceLostEvent>&& lostEvent);
    ~Device() override;

    MaybeError Initialize(const UnpackedPtr<DeviceDescriptor>& descriptor);

    uint32_t GetOptimalBytesPerRowAlignment() const override;
    uint64_t GetOptimalBufferToTextureCopyOffsetAlignment() const override;

    float GetTimestampPeriodInNS() const override;

    WGPUDevice GetInnerHandle() const;

    const DawnProcTable& GetFunctions() const;

  private:
    Device(AdapterBase* adapter,
           WGPUAdapter innerAdapter,
           const UnpackedPtr<DeviceDescriptor>& descriptor,
           const TogglesState& deviceToggles,
           Ref<DeviceBase::DeviceLostEvent>&& lostEvent);

    ResultOrError<Ref<BindGroupBase>> CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) override;
    ResultOrError<Ref<BindGroupLayoutInternalBase>> CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) override;
    ResultOrError<Ref<BufferBase>> CreateBufferImpl(
        const UnpackedPtr<BufferDescriptor>& descriptor) override;
    ResultOrError<Ref<CommandBufferBase>> CreateCommandBuffer(
        CommandEncoder* encoder,
        const CommandBufferDescriptor* descriptor) override;
    Ref<ComputePipelineBase> CreateUninitializedComputePipelineImpl(
        const UnpackedPtr<ComputePipelineDescriptor>& descriptor) override;
    ResultOrError<Ref<PipelineLayoutBase>> CreatePipelineLayoutImpl(
        const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) override;
    ResultOrError<Ref<QuerySetBase>> CreateQuerySetImpl(
        const QuerySetDescriptor* descriptor) override;
    Ref<RenderPipelineBase> CreateUninitializedRenderPipelineImpl(
        const UnpackedPtr<RenderPipelineDescriptor>& descriptor) override;
    ResultOrError<Ref<SamplerBase>> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
    ResultOrError<Ref<ShaderModuleBase>> CreateShaderModuleImpl(
        const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
        const std::vector<tint::wgsl::Extension>& internalExtensions,
        ShaderModuleParseResult* parseResult,
        std::unique_ptr<OwnedCompilationMessages>* compilationMessages) override;
    ResultOrError<Ref<SwapChainBase>> CreateSwapChainImpl(
        Surface* surface,
        SwapChainBase* previousSwapChain,
        const SurfaceConfiguration* config) override;
    ResultOrError<Ref<TextureBase>> CreateTextureImpl(
        const UnpackedPtr<TextureDescriptor>& descriptor) override;
    ResultOrError<Ref<TextureViewBase>> CreateTextureViewImpl(
        TextureBase* texture,
        const UnpackedPtr<TextureViewDescriptor>& descriptor) override;

    MaybeError TickImpl() override;

    MaybeError CopyFromStagingToBufferImpl(BufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;
    MaybeError CopyFromStagingToTextureImpl(const BufferBase* source,
                                            const TexelCopyBufferLayout& src,
                                            const TextureCopy& dst,
                                            const Extent3D& copySizePixels) override;

    void DestroyImpl() override;

    // The "lower layer" implementation device that actually manages calls to GPU driver.
    WGPUDevice mInnerDevice = nullptr;

    const DawnProcTable& mWGPU;
};

// Helper class so |BindGroup| can allocate memory for its binding data,
// before calling the BindGroupBase base class constructor.
class BindGroupDataHolder {
  protected:
    explicit BindGroupDataHolder(size_t size);
    ~BindGroupDataHolder();

    raw_ptr<void> mBindingDataAllocation;
};

class BindGroup final : private BindGroupDataHolder, public BindGroupBase {
  public:
    BindGroup(DeviceBase* device, const BindGroupDescriptor* descriptor);

  private:
    ~BindGroup() override = default;

    MaybeError InitializeImpl() override;
};

class BindGroupLayout final : public BindGroupLayoutInternalBase {
  public:
    BindGroupLayout(DeviceBase* device, const BindGroupLayoutDescriptor* descriptor);

  private:
    ~BindGroupLayout() override = default;
};

class Buffer final : public BufferBase {
  public:
    static ResultOrError<Ref<Buffer>> Create(Device* device,
                                             const UnpackedPtr<BufferDescriptor>& descriptor);
    Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor);

    void CopyFromStaging(BufferBase* staging,
                         uint64_t sourceOffset,
                         uint64_t destinationOffset,
                         uint64_t size);

  private:
    MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
    void UnmapImpl() override;
    void DestroyImpl() override;
    bool IsCPUWritableAtCreation() const override;
    MaybeError MapAtCreationImpl() override;
    void* GetMappedPointer() override;
};

class CommandBuffer final : public CommandBufferBase {
  public:
    CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor);

  private:
    ~CommandBuffer() override;
};

class Queue final : public QueueBase {
  public:
    Queue(Device* device, const QueueDescriptor* descriptor);

  private:
    ~Queue() override;
    MaybeError SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) override;
    MaybeError WriteBufferImpl(BufferBase* buffer,
                               uint64_t bufferOffset,
                               const void* data,
                               size_t size) override;
    ResultOrError<ExecutionSerial> CheckAndUpdateCompletedSerials() override;
    void ForceEventualFlushOfCommands() override;
    bool HasPendingCommands() const override;
    MaybeError SubmitPendingCommands() override;
    ResultOrError<bool> WaitForQueueSerial(ExecutionSerial serial, Nanoseconds timeout) override;
    MaybeError WaitForIdleForDestruction() override;
};

class QuerySet final : public QuerySetBase {
  public:
    QuerySet(Device* device, const QuerySetDescriptor* descriptor);
};

class PipelineLayout final : public PipelineLayoutBase {
  public:
    PipelineLayout(DeviceBase* device, const UnpackedPtr<PipelineLayoutDescriptor>& descriptor);
};

class ComputePipeline final : public ComputePipelineBase {
  public:
    using ComputePipelineBase::ComputePipelineBase;

    MaybeError InitializeImpl() override;
};

class RenderPipeline final : public RenderPipelineBase {
  public:
    using RenderPipelineBase::RenderPipelineBase;

    MaybeError InitializeImpl() override;
};

class ShaderModule final : public ShaderModuleBase {
  public:
    using ShaderModuleBase::ShaderModuleBase;

    MaybeError Initialize(ShaderModuleParseResult* parseResult,
                          std::unique_ptr<OwnedCompilationMessages>* compilationMessages);
};

class SwapChain final : public SwapChainBase {
  public:
    static ResultOrError<Ref<SwapChain>> Create(Device* device,
                                                Surface* surface,
                                                SwapChainBase* previousSwapChain,
                                                const SurfaceConfiguration* config);
    ~SwapChain() override;

  private:
    using SwapChainBase::SwapChainBase;
    MaybeError Initialize(SwapChainBase* previousSwapChain);

    Ref<Texture> mTexture;

    MaybeError PresentImpl() override;
    ResultOrError<SwapChainTextureInfo> GetCurrentTextureImpl() override;
    void DetachFromSurfaceImpl() override;
};

class Texture : public TextureBase {
  public:
    Texture(DeviceBase* device, const UnpackedPtr<TextureDescriptor>& descriptor);
};

class TextureView : public TextureViewBase {
  public:
    TextureView(TextureBase* texture, const UnpackedPtr<TextureViewDescriptor>& descriptor);
};

class Sampler : public SamplerBase {
  public:
    Sampler(DeviceBase* device, const SamplerDescriptor* descriptor);
};

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_DEVICEWGPU_H_
