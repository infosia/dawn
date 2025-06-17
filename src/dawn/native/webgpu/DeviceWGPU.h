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
#include <mutex>
#include <vector>

#include "dawn/native/Device.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/webgpu/Forward.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "dawn/wire/ChunkedCommandSerializer.h"
#include "dawn/wire/ObjectHandle.h"
#include "dawn/wire/WireCmd_autogen.h"

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

    WGPUInstance GetInnerInstance() const;

    const DawnProcTable& wgpu;

    // For serialization/logging
    // dawn::wire::ObjectHandle GetOrCreateHandle(const ApiObjectBase* obj);

    std::unique_ptr<dawn::wire::CommandSerializer> mCommandSerializerImpl; // The actual serializer (e.g., file/stdout)
    std::unique_ptr<dawn::wire::ChunkedCommandSerializer> mSerializer;

    mutable std::mutex mObjectHandleMutex;
    absl::flat_hash_map<const ApiObjectBase*, dawn::wire::ObjectHandle> mObjectHandles;
    dawn::wire::ObjectId mNextObjectId = 1;

    dawn::wire::ObjectHandle mDeviceHandle;

    dawn::wire::ObjectHandle ReserveHandle();
    

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
        ShaderModuleParseResult* parseResult) override;
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

    MaybeError CopyFromStagingToBuffer(BufferBase* source,
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

    // // Simple ObjectIdProvider implementation for logging purposes
    // class DeviceObjectIdProvider : public dawn::wire::ObjectIdProvider {
    //   public:
    //     explicit DeviceObjectIdProvider(Device* device) : mDevice(device) {}
    //     dawn::wire::WireResult GetId(WGPUBuffer buffer, dawn::wire::ObjectId* out) const override;


    //     dawn::wire::WireResult GetId(WGPUAdapter object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUAdapter object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUBindGroup object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUBindGroup object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUBindGroupLayout object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUBindGroupLayout object, ObjectId* out) const override;
    //     // dawn::wire::WireResult GetId(WGPUBuffer object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUBuffer object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUCommandBuffer object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUCommandBuffer object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUCommandEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUCommandEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUComputePassEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUComputePassEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUComputePipeline object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUComputePipeline object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUDevice object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUDevice object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUExternalTexture object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUExternalTexture object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUInstance object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUInstance object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUPipelineLayout object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUPipelineLayout object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUQuerySet object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUQuerySet object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUQueue object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUQueue object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPURenderBundle object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPURenderBundle object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPURenderBundleEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPURenderBundleEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPURenderPassEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPURenderPassEncoder object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPURenderPipeline object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPURenderPipeline object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUSampler object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUSampler object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUShaderModule object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUShaderModule object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUSurface object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUSurface object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUTexture object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUTexture object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetId(WGPUTextureView object, ObjectId* out) const override;
    //     dawn::wire::WireResult GetOptionalId(WGPUTextureView object, ObjectId* out) const override;

    //     // Add other GetId overloads as needed for other types...
    //     // For simplicity, other GetId methods will return error or assert.
    //     // Default implementation for other types:
    //     template <typename T>
    //     dawn::wire::WireResult GetIdImpl(T object, dawn::wire::ObjectId* out) const;

        
    //   private:
    //     raw_ptr<Device> mDevice;
    // };
    // std::unique_ptr<DeviceObjectIdProvider> mObjectIdProvider;
};

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_DEVICEWGPU_H_
