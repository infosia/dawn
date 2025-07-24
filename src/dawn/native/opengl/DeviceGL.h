// Copyright 2018 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_OPENGL_DEVICEGL_H_
#define SRC_DAWN_NATIVE_OPENGL_DEVICEGL_H_

#include <future>
#include <memory>
#include <unordered_map>
#include <vector>

#include "dawn/native/dawn_platform.h"

#include "dawn/common/Platform.h"
#include "dawn/native/Device.h"
#include "dawn/native/QuerySet.h"
#include "dawn/native/opengl/EGLFunctions.h"
#include "dawn/native/opengl/Forward.h"
#include "dawn/native/opengl/GLFormat.h"
#include "dawn/native/opengl/OpenGLFunctions.h"

namespace dawn::native {
class AHBFunctions;
}  // namespace dawn::native

namespace dawn::native::opengl {

class ContextEGL;
class WrappedEGLSync;

class Device final : public DeviceBase {
  public:
    class Context;
    static ResultOrError<Ref<Device>> Create(AdapterBase* adapter,
                                             const UnpackedPtr<DeviceDescriptor>& descriptor,
                                             const OpenGLFunctions& functions,
                                             std::unique_ptr<ContextEGL> context,
                                             const TogglesState& deviceToggles,
                                             Ref<DeviceBase::DeviceLostEvent>&& lostEvent);
    ~Device() override;

    MaybeError Initialize(const UnpackedPtr<DeviceDescriptor>& descriptor);

    // Returns all the OpenGL entry points.
    const OpenGLFunctions& GetGL() const;

    // Helper functions to get access to relevant EGL objects.
    const EGLFunctions& GetEGL() const;
    EGLDisplay GetEGLDisplay() const;
    ContextEGL* GetContext() const;

    const GLFormat& GetGLFormat(const Format& format);

    int GetMaxTextureMaxAnisotropy() const;

    MaybeError ValidateTextureCanBeWrapped(const UnpackedPtr<TextureDescriptor>& descriptor);
    Ref<TextureBase> CreateTextureWrappingEGLImage(const ExternalImageDescriptor* descriptor,
                                                   ::EGLImage image);
    Ref<TextureBase> CreateTextureWrappingGLTexture(const ExternalImageDescriptor* descriptor,
                                                    GLuint texture);

    ResultOrError<Ref<CommandBufferBase>> CreateCommandBuffer(
        CommandEncoder* encoder,
        const CommandBufferDescriptor* descriptor) override;

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

    uint32_t GetOptimalBytesPerRowAlignment() const override;
    uint64_t GetOptimalBufferToTextureCopyOffsetAlignment() const override;

    float GetTimestampPeriodInNS() const override;

    bool MayRequireDuplicationOfIndirectParameters() const override;
    bool ShouldApplyIndexBufferOffsetToFirstIndex() const override;

    bool CanAddStorageUsageToBufferWithoutSideEffects(wgpu::BufferUsage storageUsage,
                                                      wgpu::BufferUsage originalUsage,
                                                      size_t bufferSize) const override;

    const AHBFunctions* GetOrLoadAHBFunctions();

    const Buffer* GetInternalTextureBuiltinsUniformBuffer() const;
    const Buffer* GetInternalArrayLengthUniformBuffer() const;

    // Fake ID system
    GLuint NextBufferFakeId();
    std::shared_future<GLuint> GetBufferFuture(GLuint fakeId);
    void TrackBuffer(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackBuffer(GLuint fakeId);

    GLuint NextTextureFakeId();
    std::shared_future<GLuint> GetTextureFuture(GLuint fakeId);
    void TrackTexture(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackTexture(GLuint fakeId);

    GLuint NextProgramFakeId();
    std::shared_future<GLuint> GetProgramFuture(GLuint fakeId);
    void TrackProgram(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackProgram(GLuint fakeId);

    GLuint NextSamplerFakeId();
    std::shared_future<GLuint> GetSamplerFuture(GLuint fakeId);
    void TrackSampler(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackSampler(GLuint fakeId);

    GLuint NextQueryFakeId();
    std::shared_future<GLuint> GetQueryFuture(GLuint fakeId);
    void TrackQuery(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackQuery(GLuint fakeId);

    GLuint NextVAOFakeId();
    std::shared_future<GLuint> GetVAOFuture(GLuint fakeId);
    void TrackVAO(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackVAO(GLuint fakeId);

    GLuint NextTextureViewFakeId();
    std::shared_future<GLuint> GetTextureViewFuture(GLuint fakeId);
    void TrackTextureView(GLuint fakeId, std::shared_future<GLuint> future);
    void UntrackTextureView(GLuint fakeId);

    GLuint NextFenceFakeId();
    std::shared_future<Ref<WrappedEGLSync>> GetFenceFuture(GLuint fakeId);
    void TrackFence(GLuint fakeId, std::shared_future<Ref<WrappedEGLSync>> future);
    void UntrackFence(GLuint fakeId);

  private:
    Device(AdapterBase* adapter,
           const UnpackedPtr<DeviceDescriptor>& descriptor,
           const OpenGLFunctions& functions,
           std::unique_ptr<ContextEGL> context,
           const TogglesState& deviceToggles,
           Ref<DeviceBase::DeviceLostEvent>&& lostEvent);

    ResultOrError<Ref<BindGroupBase>> CreateBindGroupImpl(
        const BindGroupDescriptor* descriptor) override;
    ResultOrError<Ref<BindGroupLayoutInternalBase>> CreateBindGroupLayoutImpl(
        const BindGroupLayoutDescriptor* descriptor) override;
    ResultOrError<Ref<BufferBase>> CreateBufferImpl(
        const UnpackedPtr<BufferDescriptor>& descriptor) override;
    ResultOrError<Ref<PipelineLayoutBase>> CreatePipelineLayoutImpl(
        const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) override;
    ResultOrError<Ref<QuerySetBase>> CreateQuerySetImpl(
        const QuerySetDescriptor* descriptor) override;
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
    Ref<ComputePipelineBase> CreateUninitializedComputePipelineImpl(
        const UnpackedPtr<ComputePipelineDescriptor>& descriptor) override;
    Ref<RenderPipelineBase> CreateUninitializedRenderPipelineImpl(
        const UnpackedPtr<RenderPipelineDescriptor>& descriptor) override;
    ResultOrError<Ref<SharedTextureMemoryBase>> ImportSharedTextureMemoryImpl(
        const SharedTextureMemoryDescriptor* descriptor) override;
    ResultOrError<Ref<SharedFenceBase>> ImportSharedFenceImpl(
        const SharedFenceDescriptor* descriptor) override;
    ResultOrError<Ref<TextureBase>> CreateTextureWrappingEGLImageImpl(
        const ExternalImageDescriptor* descriptor,
        ::EGLImage image);
    ResultOrError<Ref<TextureBase>> CreateTextureWrappingGLTextureImpl(
        const ExternalImageDescriptor* descriptor,
        GLuint texture);

    void DestroyImpl() override;

    const OpenGLFunctions mGL;

    GLFormatTable mFormatTable;
    std::unique_ptr<ContextEGL> mContext;
    int mMaxTextureMaxAnisotropy = 0;

    // Maintain an internal uniform buffer to store extra information needed by shader emulation for
    // certain texture builtins.
    Ref<Buffer> mTextureBuiltinsBuffer;

    // Maintain an internal uniform buffer to store extra array length information if needed.
    Ref<Buffer> mArrayLengthBuffer;

#if DAWN_PLATFORM_IS(ANDROID)
    std::unique_ptr<AHBFunctions> mAHBFunctions;
#endif  // DAWN_PLATFORM_IS(ANDROID)

    // Fake ID system
    std::atomic<GLuint> mNextBufferFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mBufferIDFutures;

    std::atomic<GLuint> mNextTextureFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mTextureIDFutures;

    std::atomic<GLuint> mNextProgramFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mProgramIDFutures;

    std::atomic<GLuint> mNextSamplerFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mSamplerIDFutures;

    std::atomic<GLuint> mNextQueryFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mQueryIDFutures;

    std::atomic<GLuint> mNextVAOFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mVAOIDFutures;

    std::atomic<GLuint> mNextTextureViewFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<GLuint>>> mTextureViewIDFutures;

    std::atomic<GLuint> mNextFenceFakeId{1};
    MutexProtected<std::unordered_map<GLuint, std::shared_future<Ref<WrappedEGLSync>>>>
        mFenceIDFutures;
};

}  // namespace dawn::native::opengl

#endif  // SRC_DAWN_NATIVE_OPENGL_DEVICEGL_H_
