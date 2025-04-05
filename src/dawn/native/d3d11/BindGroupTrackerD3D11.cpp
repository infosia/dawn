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

#include "dawn/native/d3d11/BindGroupTrackerD3D11.h"

#include <algorithm>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/MatchVariant.h"
#include "dawn/common/Range.h"
#include "dawn/native/Format.h"
#include "dawn/native/d3d/D3DError.h"
#include "dawn/native/d3d11/BindGroupD3D11.h"
#include "dawn/native/d3d11/BufferD3D11.h"
#include "dawn/native/d3d11/CommandRecordingContextD3D11.h"
#include "dawn/native/d3d11/DeviceD3D11.h"
#include "dawn/native/d3d11/PipelineLayoutD3D11.h"
#include "dawn/native/d3d11/SamplerD3D11.h"
#include "dawn/native/d3d11/TextureD3D11.h"

namespace dawn::native::d3d11 {
namespace {

constexpr uint32_t kInvalidSlot = 0xffffffff;

constexpr auto kVertex = SingleShaderStage::Vertex;
constexpr auto kFragment = SingleShaderStage::Fragment;
constexpr auto kCompute = SingleShaderStage::Compute;

bool CheckAllSlotsAreEmpty(const ScopedSwapStateCommandRecordingContext* commandContext) {
    auto* deviceContext = commandContext->GetD3D11DeviceContext3();

    // Reserve one slot for builtin constants.
    constexpr uint32_t kReservedCBVSlots = 1;

    // Check constant buffer slots
    for (UINT slot = 0;
         slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - kReservedCBVSlots; ++slot) {
        ID3D11Buffer* buffer = nullptr;
        deviceContext->VSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
        deviceContext->PSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
        deviceContext->CSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
    }

    // Check resource slots
    for (UINT slot = 0; slot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++slot) {
        ID3D11ShaderResourceView* srv = nullptr;
        deviceContext->VSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
        deviceContext->PSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
        deviceContext->CSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
    }

    // Check sampler slots
    for (UINT slot = 0; slot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ++slot) {
        ID3D11SamplerState* sampler = nullptr;
        deviceContext->VSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
        deviceContext->PSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
        deviceContext->CSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
    }

    // Check UAV slots for compute
    for (UINT slot = 0; slot < D3D11_1_UAV_SLOT_COUNT; ++slot) {
        ID3D11UnorderedAccessView* uav = nullptr;
        deviceContext->CSGetUnorderedAccessViews(slot, 1, &uav);
        DAWN_ASSERT(uav == nullptr);
    }
    // Check UAV slots for render
    for (UINT slot = 0; slot < commandContext->GetDevice()->GetUAVSlotCount(); ++slot) {
        ID3D11UnorderedAccessView* uav = nullptr;
        deviceContext->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, slot, 1,
                                                                 &uav);
        DAWN_ASSERT(uav == nullptr);
    }

    return true;
}

std::tuple<const BindingInfo&, const BufferBindingInfo&, BufferBinding> ExtractBufferBindingInfo(
    BindGroupBase* group,
    BindingIndex bindingIndex,
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets) {
    const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);

    DAWN_ASSERT(std::holds_alternative<BufferBindingInfo>(bindingInfo.bindingLayout));
    const BufferBindingInfo& layout = std::get<BufferBindingInfo>(bindingInfo.bindingLayout);

    BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
    if (layout.hasDynamicOffset) {
        binding.offset += dynamicOffsets[bindingIndex];
    }

    return std::make_tuple(std::cref(bindingInfo), std::cref(layout), std::move(binding));
}

template <SingleShaderStage Stage, typename... Args>
void SetConstantBuffersImpl(ID3D11DeviceContext3* deviceContext, Args&&... args) {
    if constexpr (Stage == kVertex) {
        deviceContext->VSSetConstantBuffers1(std::forward<Args>(args)...);
    } else if constexpr (Stage == kFragment) {
        deviceContext->PSSetConstantBuffers1(std::forward<Args>(args)...);
    } else if constexpr (Stage == kCompute) {
        deviceContext->CSSetConstantBuffers1(std::forward<Args>(args)...);
    }
}

template <SingleShaderStage Stage, typename... Args>
void SetShaderResourcesImpl(ID3D11DeviceContext3* deviceContext, Args&&... args) {
    if constexpr (Stage == kVertex) {
        deviceContext->VSSetShaderResources(std::forward<Args>(args)...);
    } else if constexpr (Stage == kFragment) {
        deviceContext->PSSetShaderResources(std::forward<Args>(args)...);
    } else if constexpr (Stage == kCompute) {
        deviceContext->CSSetShaderResources(std::forward<Args>(args)...);
    }
}

template <SingleShaderStage Stage, typename... Args>
void SetSamplersImpl(ID3D11DeviceContext3* deviceContext, Args&&... args) {
    if constexpr (Stage == kVertex) {
        deviceContext->VSSetSamplers(std::forward<Args>(args)...);
    } else if constexpr (Stage == kFragment) {
        deviceContext->PSSetSamplers(std::forward<Args>(args)...);
    } else if constexpr (Stage == kCompute) {
        deviceContext->CSSetSamplers(std::forward<Args>(args)...);
    }
}

template <SingleShaderStage Stage, typename... Args>
void SetUnorderedAccessViewsImpl(ID3D11DeviceContext3* deviceContext, Args&&... args) {
    if constexpr (Stage == kFragment) {
        deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
            D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
            std::forward<Args>(args)..., nullptr);
    } else if constexpr (Stage == kCompute) {
        deviceContext->CSSetUnorderedAccessViews(std::forward<Args>(args)..., nullptr);
    } else {
        DAWN_UNREACHABLE();
    }
}

}  // namespace

BindGroupTracker::BindGroupTracker(const ScopedSwapStateCommandRecordingContext* commandContext)
    : mCommandContext(commandContext) {
    mLastAppliedPipelineLayout = mCommandContext->GetDevice()->GetEmptyPipelineLayout();
}

BindGroupTracker::~BindGroupTracker() {
    // Unbind constant buffers.
    // Note: We already track max bound slots so we can precisely unbind the correct number of used
    // slots. It should be faster than unbinding everything from
    // [0 - D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT).
#define UNBIND_CONST_BUFFERS(Stage)                  \
    UnbindConstantBuffers<SingleShaderStage::Stage>( \
        0, mMaxConstantBufferSlot[SingleShaderStage::Stage])

    UNBIND_CONST_BUFFERS(Vertex);
    UNBIND_CONST_BUFFERS(Fragment);
    UNBIND_CONST_BUFFERS(Compute);

    // Unbind SRVs.
#define UNBIND_SRVS(Stage) \
    UnbindShaderResources<SingleShaderStage::Stage>(0, mMaxSRVSlot[SingleShaderStage::Stage])

    UNBIND_SRVS(Vertex);
    UNBIND_SRVS(Fragment);
    UNBIND_SRVS(Compute);

    // Unbind samplers.
#define UNBIND_SAMPLERS(Stage) \
    UnbindSamplers<SingleShaderStage::Stage>(0, mMaxSamplerSlot[SingleShaderStage::Stage])

    UNBIND_SAMPLERS(Vertex);
    UNBIND_SAMPLERS(Fragment);
    UNBIND_SAMPLERS(Compute);

    // Unbind UAVs.
#define UNBIND_UAVS(Stage)                                                                      \
    UnbindUnorderedAccessViews<SingleShaderStage::Stage>(mMinUAVSlot[SingleShaderStage::Stage], \
                                                         mMaxUAVSlot[SingleShaderStage::Stage])

    UNBIND_UAVS(Fragment);
    UNBIND_UAVS(Compute);

    DAWN_ASSERT(CheckAllSlotsAreEmpty(mCommandContext));
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetConstantBuffers(uint32_t idx,
                                          uint32_t count,
                                          ID3D11Buffer* const* d3d11Buffer,
                                          const uint32_t* firstConstant,
                                          const uint32_t* numConstants) {
    mMaxConstantBufferSlot[Stage] = std::max(mMaxConstantBufferSlot[Stage], idx + count);
    SetConstantBuffersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), idx, count,
                                  d3d11Buffer, firstConstant, numConstants);
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindConstantBuffers(uint32_t start, uint32_t slots) {
    if (slots == 0) {
        return;
    }

    mScratchBufferSlots.resize(0);
    mScratchBufferSlots.insert(mScratchBufferSlots.end(), slots, nullptr);

    SetConstantBuffersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), start, slots,
                                  mScratchBufferSlots.data(), nullptr, nullptr);
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetShaderResources(uint32_t idx,
                                          uint32_t count,
                                          ID3D11ShaderResourceView* const* srv) {
    mMaxSRVSlot[Stage] = std::max(mMaxSRVSlot[Stage], idx + count);
    SetShaderResourcesImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), idx, count, srv);
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindShaderResources(uint32_t start, uint32_t slots) {
    if (slots == 0) {
        return;
    }

    mScratchSRVSlots.resize(0);
    mScratchSRVSlots.insert(mScratchSRVSlots.end(), slots, nullptr);

    SetShaderResourcesImpl<Stage>(
        mCommandContext->GetD3D11DeviceContext3(), start, slots,
        reinterpret_cast<ID3D11ShaderResourceView**>(mScratchSRVSlots.data()));
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetSamplers(uint32_t idx,
                                   uint32_t count,
                                   ID3D11SamplerState* const* sampler) {
    mMaxSamplerSlot[Stage] = std::max(mMaxSamplerSlot[Stage], idx + count);
    SetSamplersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), idx, count, sampler);
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindSamplers(uint32_t start, uint32_t slots) {
    if (slots == 0) {
        return;
    }

    mScratchSamplerSlots.resize(0);
    mScratchSamplerSlots.insert(mScratchSamplerSlots.end(), slots, nullptr);

    SetSamplersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), start, slots,
                           mScratchSamplerSlots.data());
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetUnorderedAccessViews(uint32_t idx,
                                               uint32_t count,
                                               ID3D11UnorderedAccessView* const* uavs) {
    mMinUAVSlot[Stage] = std::min(mMinUAVSlot[Stage], idx);
    mMaxUAVSlot[Stage] = std::max(mMaxUAVSlot[Stage], idx + count);

    SetUnorderedAccessViewsImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), idx, count, uavs);
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindUnorderedAccessViews(uint32_t start, uint32_t end) {
    if (start >= end) {
        return;
    }
    uint32_t count = end - start;

    mScratchUAVSlots.resize(0);
    mScratchUAVSlots.insert(mScratchUAVSlots.end(), count, nullptr);

    SetUnorderedAccessViewsImpl<Stage>(
        mCommandContext->GetD3D11DeviceContext3(), start, count,
        reinterpret_cast<ID3D11UnorderedAccessView**>(mScratchUAVSlots.data()));
}

ResultOrError<BindGroupTracker::ConstantBufferBinding> BindGroupTracker::GetConstantBufferBinding(
    BindGroupBase* group,
    BindingIndex bindingIndex,
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets) {
    const auto& [bindingInfo, layout, binding] =
        ExtractBufferBindingInfo(group, bindingIndex, dynamicOffsets);

    DAWN_ASSERT(layout.type == wgpu::BufferBindingType::Uniform);

    ID3D11Buffer* d3d11Buffer;
    DAWN_TRY_ASSIGN(d3d11Buffer,
                    ToGPUUsableBuffer(binding.buffer)->GetD3D11ConstantBuffer(mCommandContext));
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-vssetconstantbuffers1
    // Offset and size are measured in shader constants, which are 16 bytes
    // (4*32-bit components). And the offsets and counts must be multiples
    // of 16.
    // WebGPU's minUniformBufferOffsetAlignment is 256.
    DAWN_ASSERT(IsAligned(binding.offset, 256));
    UINT firstConstant = static_cast<UINT>(binding.offset / 16);
    UINT size = static_cast<UINT>(Align(binding.size, 16) / 16);
    UINT numConstants = Align(size, 16);
    DAWN_ASSERT(binding.offset + numConstants * 16 <= binding.buffer->GetAllocatedSize());

    return ConstantBufferBinding{d3d11Buffer, firstConstant, numConstants};
}

template <typename T>
ResultOrError<ComPtr<T>> BindGroupTracker::GetBufferD3DView(
    BindGroupBase* group,
    BindingIndex bindingIndex,
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets) {
    const auto& [bindingInfo, layout, binding] =
        ExtractBufferBindingInfo(group, bindingIndex, dynamicOffsets);

    if constexpr (std::is_same_v<T, ID3D11ShaderResourceView>) {
        DAWN_ASSERT(layout.type == wgpu::BufferBindingType::ReadOnlyStorage ||
                    layout.type == kInternalReadOnlyStorageBufferBinding);

        ComPtr<ID3D11ShaderResourceView> d3d11SRV;
        DAWN_TRY_ASSIGN(d3d11SRV, ToGPUUsableBuffer(binding.buffer)
                                      ->UseAsSRV(mCommandContext, binding.offset, binding.size));

        return d3d11SRV;
    } else if constexpr (std::is_same_v<T, ID3D11UnorderedAccessView>) {
        DAWN_ASSERT(layout.type == wgpu::BufferBindingType::Storage ||
                    layout.type == kInternalStorageBufferBinding);

        ComPtr<ID3D11UnorderedAccessView> d3d11UAV;
        DAWN_TRY_ASSIGN(d3d11UAV, ToGPUUsableBuffer(binding.buffer)
                                      ->UseAsUAV(mCommandContext, binding.offset, binding.size));
        return d3d11UAV;
    }

    DAWN_UNREACHABLE();
    return ComPtr<T>();
}

ResultOrError<ComPtr<ID3D11ShaderResourceView>> BindGroupTracker::GetTextureShaderResourceView(
    BindGroupBase* group,
    BindingIndex bindingIndex) {
    const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);

    DAWN_ASSERT(std::holds_alternative<TextureBindingInfo>(bindingInfo.bindingLayout) ||
                std::holds_alternative<StorageTextureBindingInfo>(bindingInfo.bindingLayout));

    if (std::holds_alternative<StorageTextureBindingInfo>(bindingInfo.bindingLayout)) {
        DAWN_ASSERT(std::get<StorageTextureBindingInfo>(bindingInfo.bindingLayout).access ==
                    wgpu::StorageTextureAccess::ReadOnly);
    }

    TextureView* view = ToBackend(group->GetBindingAsTextureView(bindingIndex));
    ComPtr<ID3D11ShaderResourceView> srv;

    if (DAWN_UNLIKELY(view->GetAspects() == Aspect::Stencil)) {
        // For sampling from stencil, we have to use an internal mirror 'R8Uint' texture.
        DAWN_TRY_ASSIGN(srv, ToBackend(view->GetTexture())->GetStencilSRV(mCommandContext, view));
    } else {
        DAWN_TRY_ASSIGN(srv, view->GetOrCreateD3D11ShaderResourceView());
    }

    return srv;
}

ResultOrError<ComPtr<ID3D11UnorderedAccessView>> BindGroupTracker::GetTextureUnorderedAccessView(
    BindGroupBase* group,
    BindingIndex bindingIndex) {
    const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);

    DAWN_ASSERT(std::holds_alternative<StorageTextureBindingInfo>(bindingInfo.bindingLayout));

    [[maybe_unused]] const auto& layout =
        std::get<StorageTextureBindingInfo>(bindingInfo.bindingLayout);

    DAWN_ASSERT(layout.access == wgpu::StorageTextureAccess::ReadWrite ||
                layout.access == wgpu::StorageTextureAccess::WriteOnly);

    TextureView* view = ToBackend(group->GetBindingAsTextureView(bindingIndex));
    ComPtr<ID3D11UnorderedAccessView> uav;

    DAWN_TRY_ASSIGN(uav, view->GetOrCreateD3D11UnorderedAccessView());

    return uav;
}

template <typename T>
ResultOrError<ComPtr<T>> BindGroupTracker::GetTextureD3DView(BindGroupBase* group,
                                                             BindingIndex bindingIndex) {
    if constexpr (std::is_same_v<T, ID3D11ShaderResourceView>) {
        return GetTextureShaderResourceView(group, bindingIndex);
    } else if constexpr (std::is_same_v<T, ID3D11UnorderedAccessView>) {
        return GetTextureUnorderedAccessView(group, bindingIndex);
    }

    DAWN_UNREACHABLE();
    return ComPtr<T>();
}

ID3D11SamplerState* BindGroupTracker::GetSamplerState(BindGroupBase* group,
                                                      BindingIndex bindingIndex) {
    const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);
    DAWN_ASSERT(std::holds_alternative<SamplerBindingInfo>(bindingInfo.bindingLayout));

    Sampler* sampler = ToBackend(group->GetBindingAsSampler(bindingIndex));
    return sampler->GetD3D11SamplerState();
}

template <SingleShaderStage Stage, typename D3DType>
ResultOrError<uint32_t> BindGroupTracker::CollectD3DViewBindings(
    BindGroupIndex index,
    absl::InlinedVector<ComPtr<D3DType>, 1>* viewsOut) {
    BindGroupMask mask;
    mask.set(index);
    return CollectD3DViewBindings<Stage, D3DType>(mask, viewsOut);
}

template <SingleShaderStage Stage, typename D3DType>
ResultOrError<uint32_t> BindGroupTracker::CollectD3DViewBindings(
    BindGroupMask bindGroups,
    absl::InlinedVector<ComPtr<D3DType>, 1>* viewsOut) {
    using GetBindingsFunc = const PipelineLayout::BindingSpan& (*)(PipelineLayout* pipelineLayout);
    constexpr GetBindingsFunc GetBufferBindings =
        [](PipelineLayout* pipelineLayout) -> const PipelineLayout::BindingSpan& {
        if constexpr (std::is_same_v<D3DType, ID3D11UnorderedAccessView>) {
            return pipelineLayout->GetBufferUAVBindings();
        } else {
            return pipelineLayout->GetBufferSRVBindings();
        }
    };

    constexpr GetBindingsFunc GetTextureBindings =
        [](PipelineLayout* pipelineLayout) -> const PipelineLayout::BindingSpan& {
        if constexpr (std::is_same_v<D3DType, ID3D11UnorderedAccessView>) {
            return pipelineLayout->GetTextureUAVBindings();
        } else {
            return pipelineLayout->GetTextureSRVBindings();
        }
    };

    using GetViewFunc = ResultOrError<ComPtr<D3DType>> (*)(
        BindGroupTracker* thiz, BindGroupBase* group, BindingIndex bindingIndex,
        const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets);
    constexpr GetViewFunc GetBufferView =
        [](BindGroupTracker* thiz, BindGroupBase* group, BindingIndex bindingIndex,
           const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets)
        -> ResultOrError<ComPtr<D3DType>> {
        return thiz->GetBufferD3DView<D3DType>(group, bindingIndex, dynamicOffsets);
    };

    constexpr GetViewFunc GetTextureView =
        [](BindGroupTracker* thiz, BindGroupBase* group, BindingIndex bindingIndex,
           const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets)
        -> ResultOrError<ComPtr<D3DType>> {
        return thiz->GetTextureD3DView<D3DType>(group, bindingIndex);
    };

    const std::tuple<GetBindingsFunc, GetViewFunc> kFunctions[] = {
        {GetBufferBindings, GetBufferView}, {GetTextureBindings, GetTextureView}};

    auto pipelineLayout = ToBackend(mPipelineLayout);

    uint32_t startSlot = kInvalidSlot;

    viewsOut->resize(0);
    for (BindGroupIndex index : IterateBitSet(bindGroups)) {
        for (const auto [getBindingsFunc, getViewFunc] : kFunctions) {
            const auto& bindings = getBindingsFunc(pipelineLayout)[index][Stage];

            if (bindings.size() == 0) {
                continue;
            }
            const uint32_t prevViewsCount = viewsOut->size();
            uint32_t curStartSlot;
            DAWN_TRY_ASSIGN(curStartSlot,
                            IteratePerTypeBindings<Stage>(
                                index, bindings,
                                [&](BindGroupBase* group, BindingIndex bindingIndex,
                                    const auto& dynamicOffsets) -> MaybeError {
                                    ComPtr<D3DType> d3dView;
                                    DAWN_TRY_ASSIGN(d3dView, getViewFunc(this, group, bindingIndex,
                                                                         dynamicOffsets));

                                    viewsOut->emplace_back(std::move(d3dView));
                                    return {};
                                }));

            // Check that native indices are continuous.
            DAWN_ASSERT(startSlot == kInvalidSlot || prevViewsCount == curStartSlot - startSlot);
            startSlot = std::min(startSlot, curStartSlot);
        }
    }

    return startSlot;
}

template <SingleShaderStage Stage, typename Fn>
ResultOrError<uint32_t> BindGroupTracker::IteratePerTypeBindings(
    BindGroupIndex index,
    const std::vector<BindingIndex>& bindings,
    Fn&& processFunc) {
    return IteratePerTypeBindings<Stage>(
        index, ityp::SpanFromUntyped<size_t>(bindings.data(), bindings.size()),
        std::forward<Fn>(processFunc));
}

template <SingleShaderStage Stage, typename Fn>
ResultOrError<uint32_t> BindGroupTracker::IteratePerTypeBindings(
    BindGroupIndex index,
    ityp::span<size_t, const BindingIndex> bindings,
    Fn&& processFunc) {
    BindGroupBase* group = mBindGroups[index];
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets = mDynamicOffsets[index];
    const auto& indices = ToBackend(mPipelineLayout)->GetBindingIndexToNativeMap()[index];

    const uint32_t startSlot = indices[bindings.front()][Stage];
    [[maybe_unused]] uint32_t prevSlot = 0;
    for (BindingIndex bindingIndex : bindings) {
#if defined(DAWN_ENABLE_ASSERTS)
        // Make sure the bindings are continuous
        uint32_t nativeSlot = indices[bindingIndex][Stage];
        DAWN_ASSERT(nativeSlot == startSlot || nativeSlot == prevSlot + 1);
        prevSlot = nativeSlot;
#endif
        DAWN_TRY(processFunc(group, bindingIndex, dynamicOffsets));
    }

    return startSlot;
}

template <wgpu::ShaderStage kVisibleStage>
MaybeError BindGroupTracker::ApplyBindGroupNonUAVBindings(BindGroupIndex index) {
    if constexpr (wgpu::ShaderStage::Vertex & kVisibleStage) {
        DAWN_TRY(ApplyBindGroupPerStage<kVertex>(index));
    }
    if constexpr (wgpu::ShaderStage::Fragment & kVisibleStage) {
        DAWN_TRY(ApplyBindGroupPerStage<kFragment>(index));
    }
    if constexpr (wgpu::ShaderStage::Compute & kVisibleStage) {
        DAWN_TRY(ApplyBindGroupPerStage<kCompute>(index));
    }
    return {};
}

template <SingleShaderStage Stage>
MaybeError BindGroupTracker::ApplyBindGroupPerStage(BindGroupIndex index) {
    auto pipelineLayout = ToBackend(mPipelineLayout);

    // Constant buffers' bindings
    const auto& cbBindings = pipelineLayout->GetConstantBufferBindings()[index][Stage];
    if (!cbBindings.empty()) {
        mScratchBufferSlots.resize(0);
        mScratchStartConstants.resize(0);
        mScratchNumConstants.resize(0);

        uint32_t startSlot;
        DAWN_TRY_ASSIGN(startSlot,
                        IteratePerTypeBindings<Stage>(
                            index, cbBindings,
                            [&](BindGroupBase* group, BindingIndex bindingIndex,
                                const auto& dynamicOffsets) -> MaybeError {
                                ConstantBufferBinding bufferBinding;
                                DAWN_TRY_ASSIGN(
                                    bufferBinding,
                                    GetConstantBufferBinding(group, bindingIndex, dynamicOffsets));

                                mScratchBufferSlots.push_back(bufferBinding.buffer);
                                mScratchStartConstants.push_back(bufferBinding.firstConstant);
                                mScratchNumConstants.push_back(bufferBinding.numConstants);
                                return {};
                            }));

        this->SetConstantBuffers<Stage>(startSlot, mScratchBufferSlots.size(),
                                        mScratchBufferSlots.data(), mScratchStartConstants.data(),
                                        mScratchNumConstants.data());
    }

    // SRV bindings.
    uint32_t startSRVSlot;
    DAWN_TRY_ASSIGN(startSRVSlot, CollectD3DViewBindings<Stage>(index, &mScratchSRVSlots));
    if (!mScratchSRVSlots.empty()) {
        this->SetShaderResources<Stage>(
            startSRVSlot, mScratchSRVSlots.size(),
            reinterpret_cast<ID3D11ShaderResourceView**>(mScratchSRVSlots.data()));
        mScratchSRVSlots.resize(0);
    }

    // Sampler bindings.
    const auto& samplerBindings = pipelineLayout->GetSamplerBindings()[index][Stage];
    if (!samplerBindings.empty()) {
        mScratchSamplerSlots.resize(0);

        uint32_t startSlot;
        DAWN_TRY_ASSIGN(
            startSlot, IteratePerTypeBindings<Stage>(
                           index, samplerBindings,
                           [&](BindGroupBase* group, BindingIndex bindingIndex,
                               const auto& dynamicOffsets) -> MaybeError {
                               mScratchSamplerSlots.push_back(GetSamplerState(group, bindingIndex));
                               return {};
                           }));

        this->SetSamplers<Stage>(startSlot, mScratchSamplerSlots.size(),
                                 mScratchSamplerSlots.data());
    }

    return {};
}

ComputePassBindGroupTracker::ComputePassBindGroupTracker(
    const ScopedSwapStateCommandRecordingContext* commandContext)
    : BindGroupTracker(commandContext) {}

ComputePassBindGroupTracker::~ComputePassBindGroupTracker() = default;

void ComputePassBindGroupTracker::UnapplyComputeBindings(BindGroupIndex index) {
    auto pipelineLayout = ToBackend(mLastAppliedPipelineLayout);
    const auto& indices = pipelineLayout->GetBindingIndexToNativeMap()[index];

    // Constant buffers' bindings
    const auto& cbBindings = pipelineLayout->GetConstantBufferBindings()[index][kCompute];
    if (!cbBindings.empty()) {
        uint32_t startSlot = indices[cbBindings.front()][kCompute];
        this->UnbindConstantBuffers<kCompute>(startSlot, cbBindings.size());
    }

    // SRV bindings.
    const auto& srvBindings = pipelineLayout->GetSRVBindings()[index][kCompute];
    if (!srvBindings.empty()) {
        uint32_t startSlot = indices[srvBindings.front()][kCompute];
        this->UnbindShaderResources<kCompute>(startSlot, srvBindings.size());
    }

    // Sampler bindings.
    const auto& samplerBindings = pipelineLayout->GetSamplerBindings()[index][kCompute];
    if (!samplerBindings.empty()) {
        uint32_t startSlot = indices[samplerBindings.front()][kCompute];
        this->UnbindSamplers<kCompute>(startSlot, samplerBindings.size());
    }

    // UAV bindings
    const auto& uavBindings = pipelineLayout->GetUAVBindings()[index][kCompute];
    if (!uavBindings.empty()) {
        uint32_t startSlot = indices[uavBindings.front()][kCompute];
        this->UnbindUnorderedAccessViews<kCompute>(startSlot, uavBindings.size());
    }
}

MaybeError ComputePassBindGroupTracker::Apply() {
    BeforeApply();

    BindGroupMask inheritedGroups =
        mPipelineLayout->InheritedGroupsMask(mLastAppliedPipelineLayout);
    BindGroupMask previousGroups = mLastAppliedPipelineLayout->GetBindGroupLayoutsMask();

    // To avoid UAV / SRV conflicts with bindings in previously bind groups, we unset the bind
    // groups that aren't reused by the current pipeline.
    // We also need to unset the inherited bind groups which are dirty as the group may have
    // both SRV and UAV, and the same resource may change its binding from UAV to SRV next
    // dispatch in the same group.
    //
    // Note: WebGPU API guarantees that resources are not used both as UAV and SRV in the same
    // render pass. So we don't need to do this inside render passes.
    BindGroupMask groupsToUnset = previousGroups & (~inheritedGroups | mDirtyBindGroups);
    for (BindGroupIndex index : IterateBitSet(groupsToUnset)) {
        UnapplyComputeBindings(index);
    }

    for (BindGroupIndex index : IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
        DAWN_TRY(ApplyBindGroupNonUAVBindings<wgpu::ShaderStage::Compute>(index));

        const auto& uavBindings = ToBackend(mPipelineLayout)->GetUAVBindings()[index][kCompute];
        if (!uavBindings.empty()) {
            // Collect buffer & texture UAVs and bind them all in one call.
            uint32_t startSlot;
            DAWN_TRY_ASSIGN(startSlot, CollectD3DViewBindings<kCompute>(index, &mScratchUAVSlots));

            this->SetUnorderedAccessViews<kCompute>(
                startSlot, mScratchUAVSlots.size(),
                reinterpret_cast<ID3D11UnorderedAccessView**>(mScratchUAVSlots.data()));
            mScratchUAVSlots.resize(0);
        }
    }

    AfterApply();

    return {};
}

RenderPassBindGroupTracker::RenderPassBindGroupTracker(
    const ScopedSwapStateCommandRecordingContext* commandContext,
    std::vector<ComPtr<ID3D11UnorderedAccessView>> pixelLocalStorageUAVs)
    : BindGroupTracker(commandContext), mPixelLocalStorageUAVs(std::move(pixelLocalStorageUAVs)) {}

RenderPassBindGroupTracker::~RenderPassBindGroupTracker() = default;

MaybeError RenderPassBindGroupTracker::Apply() {
    BeforeApply();

    if (!mDirtyBindGroupsObjectChangedOrIsDynamic.any() &&
        mLastAppliedPipelineLayout == mPipelineLayout) {
        AfterApply();
        return {};
    }

    const auto uavCount = ToBackend(mPipelineLayout)->GetUAVCount(kFragment);
    if (uavCount > 0) {
        // As D3d11 requires to bind all UAVs slots at the same time for pixel shaders, we
        // record all UAV slot assignments in the bind groups, and then bind them all together.
        const auto uavStartSlot = ToBackend(mPipelineLayout)->GetUAVStartIndex(kFragment);

        const BindGroupMask uavBindGroups =
            ToBackend(mPipelineLayout)->GetUAVBindGroupLayoutsMask();
        if (uavBindGroups.any()) {
            uint32_t uavInBGStartSlot;
            DAWN_TRY_ASSIGN(uavInBGStartSlot,
                            CollectD3DViewBindings<kFragment>(uavBindGroups, &mScratchUAVSlots));

            DAWN_ASSERT(uavInBGStartSlot == uavStartSlot);
        }

        for (auto& uav : mPixelLocalStorageUAVs) {
            mScratchUAVSlots.push_back(uav);
        }

        this->SetUnorderedAccessViews<kFragment>(
            uavStartSlot, mScratchUAVSlots.size(),
            reinterpret_cast<ID3D11UnorderedAccessView**>(mScratchUAVSlots.data()));
        mScratchUAVSlots.resize(0);
    }

    for (BindGroupIndex index : IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
        DAWN_TRY(
            ApplyBindGroupNonUAVBindings<wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment>(
                index));
    }

    AfterApply();

    return {};
}

}  // namespace dawn::native::d3d11
