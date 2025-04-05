// Copyright 2023 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_D3D11_BINDGROUPTRACKERD3D11_H_
#define SRC_DAWN_NATIVE_D3D11_BINDGROUPTRACKERD3D11_H_

#include <vector>

#include "absl/container/inlined_vector.h"
#include "dawn/common/ityp_span.h"
#include "dawn/native/BindGroupTracker.h"
#include "dawn/native/PerStage.h"
#include "dawn/native/d3d/d3d_platform.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native::d3d11 {

class ScopedSwapStateCommandRecordingContext;

// We need convert WebGPU bind slot to d3d11 bind slot according a map in PipelineLayout, so we
// cannot inherit BindGroupTrackerGroups. Currently we arrange all the RTVs and UAVs when calling
// OMSetRenderTargetsAndUnorderedAccessViews() with below rules:
// - RTVs from the first register (r0)
// - UAVs in bind groups
// - Pixel Local Storage UAVs
class BindGroupTracker : public BindGroupTrackerBase</*CanInheritBindGroups=*/true, uint64_t> {
  public:
    explicit BindGroupTracker(const ScopedSwapStateCommandRecordingContext* commandContext);
    virtual ~BindGroupTracker();

  protected:
    template <wgpu::ShaderStage kVisibleStage>
    MaybeError ApplyBindGroupNonUAVBindings(BindGroupIndex index);

    template <SingleShaderStage Stage>
    void SetConstantBuffers(uint32_t idx,
                            uint32_t count,
                            ID3D11Buffer* const* d3d11Buffer,
                            const uint32_t* firstConstant,
                            const uint32_t* numConstants);

    template <SingleShaderStage Stage>
    void SetShaderResources(uint32_t idx, uint32_t count, ID3D11ShaderResourceView* const* srv);

    template <SingleShaderStage Stage>
    void SetSamplers(uint32_t idx, uint32_t count, ID3D11SamplerState* const* sampler);

    template <SingleShaderStage Stage>
    void SetUnorderedAccessViews(uint32_t idx,
                                 uint32_t count,
                                 ID3D11UnorderedAccessView* const* uavs);

    template <SingleShaderStage Stage>
    void UnbindConstantBuffers(uint32_t startSlot, uint32_t numSlots);
    template <SingleShaderStage Stage>
    void UnbindShaderResources(uint32_t startSlot, uint32_t numSlots);
    template <SingleShaderStage Stage>
    void UnbindSamplers(uint32_t startSlot, uint32_t numSlots);
    template <SingleShaderStage Stage>
    void UnbindUnorderedAccessViews(uint32_t startSlot, uint32_t endSlot);

    template <SingleShaderStage Stage, typename D3DType, typename Fn>
    MaybeError IterateD3DViewBindings(BindGroupMask bindGroups, Fn&& processFunc);
    template <SingleShaderStage Stage, typename D3DType, typename Fn>
    MaybeError IterateD3DViewBindings(BindGroupIndex index, Fn&& processFunc);

    static_assert(sizeof(ComPtr<ID3D11UnorderedAccessView>) == sizeof(ID3D11UnorderedAccessView*));
    absl::InlinedVector<ComPtr<ID3D11UnorderedAccessView>, 1> mScratchUAVSlots;

  private:
    struct ConstantBufferBinding {
        ID3D11Buffer* buffer;
        UINT firstConstant = 0;
        UINT numConstants = 0;
    };

    template <SingleShaderStage Stage>
    MaybeError ApplyBindGroupPerStage(BindGroupIndex index);

    template <SingleShaderStage Stage, typename Fn>
    ResultOrError<uint32_t> IteratePerTypeBindings(BindGroupIndex index,
                                                   const std::vector<BindingIndex>& bindings,
                                                   Fn&& processFunc);
    template <SingleShaderStage Stage, typename Fn>
    ResultOrError<uint32_t> IteratePerTypeBindings(BindGroupIndex index,
                                                   ityp::span<size_t, const BindingIndex> bindings,
                                                   Fn&& processFunc);

    ResultOrError<ConstantBufferBinding> GetConstantBufferBinding(
        BindGroupBase* group,
        BindingIndex bindingIndex,
        const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets);
    template <typename T>
    ResultOrError<ComPtr<T>> GetBufferD3DView(
        BindGroupBase* group,
        BindingIndex bindingIndex,
        const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets);

    ResultOrError<ComPtr<ID3D11ShaderResourceView>> GetTextureShaderResourceView(
        BindGroupBase* group,
        BindingIndex bindingIndex);
    ResultOrError<ComPtr<ID3D11UnorderedAccessView>> GetTextureUnorderedAccessView(
        BindGroupBase* group,
        BindingIndex bindingIndex);
    template <typename T>
    ResultOrError<ComPtr<T>> GetTextureD3DView(BindGroupBase* group, BindingIndex bindingIndex);

    ID3D11SamplerState* GetSamplerState(BindGroupBase* group, BindingIndex bindingIndex);

    raw_ptr<const ScopedSwapStateCommandRecordingContext> mCommandContext;

    absl::InlinedVector<ID3D11Buffer*, 1> mScratchBufferSlots;
    absl::InlinedVector<UINT, 1> mScratchStartConstants;
    absl::InlinedVector<UINT, 1> mScratchNumConstants;
    static_assert(sizeof(ComPtr<ID3D11ShaderResourceView>) == sizeof(ID3D11ShaderResourceView*));
    absl::InlinedVector<ComPtr<ID3D11ShaderResourceView>, 1> mScratchSRVSlots;
    absl::InlinedVector<ID3D11SamplerState*, 1> mScratchSamplerSlots;

    PerStage<uint32_t> mMaxConstantBufferSlot = PerStage<uint32_t>(0);
    PerStage<uint32_t> mMaxSRVSlot = PerStage<uint32_t>(0);
    PerStage<uint32_t> mMaxSamplerSlot = PerStage<uint32_t>(0);

    PerStage<uint32_t> mMinUAVSlot = PerStage<uint32_t>(0xffffffff);
    PerStage<uint32_t> mMaxUAVSlot = PerStage<uint32_t>(0);
};

class ComputePassBindGroupTracker final : public BindGroupTracker {
  public:
    explicit ComputePassBindGroupTracker(
        const ScopedSwapStateCommandRecordingContext* commandContext);
    ~ComputePassBindGroupTracker() override;

    MaybeError Apply();

  private:
    void UnapplyComputeBindings(BindGroupIndex index);
};

class RenderPassBindGroupTracker final : public BindGroupTracker {
  public:
    explicit RenderPassBindGroupTracker(
        const ScopedSwapStateCommandRecordingContext* commandContext,
        std::vector<ComPtr<ID3D11UnorderedAccessView>> pixelLocalStorageUAVs = {});
    ~RenderPassBindGroupTracker() override;

    MaybeError Apply();

  private:
    // All the pixel local storage UAVs
    const std::vector<ComPtr<ID3D11UnorderedAccessView>> mPixelLocalStorageUAVs;
};

}  // namespace dawn::native::d3d11

#endif  // SRC_DAWN_NATIVE_D3D11_BINDGROUPTRACKERD3D11_H_
