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

template <typename T, uint32_t InitialCapacity>
template <typename Fn>
void BindGroupTracker::BindingSlot<T, InitialCapacity>::Bind(uint32_t idx,
                                                             T binding,
                                                             Fn&& bindFunc) {
    if (MaxBoundSlots() <= idx) {
        mBoundSlots.resize(idx + 1);
    }
    if (mBoundSlots[idx] == binding) {
        // redundant binding, return
        return;
    }

    bindFunc(idx, binding);

    mBoundSlots[idx] = std::move(binding);
}

BindGroupTracker::BindGroupTracker(const ScopedSwapStateCommandRecordingContext* commandContext)
    : mCommandContext(commandContext) {
    mLastAppliedPipelineLayout = mCommandContext->GetDevice()->GetEmptyPipelineLayout();
}

BindGroupTracker::~BindGroupTracker() {
    UnbindConstantBuffers<kVertex>();
    UnbindConstantBuffers<kFragment>();
    UnbindConstantBuffers<kCompute>();

    UnbindShaderResources<kVertex>();
    UnbindShaderResources<kFragment>();
    UnbindShaderResources<kCompute>();

    UnbindSamplers<kVertex>();
    UnbindSamplers<kFragment>();
    UnbindSamplers<kCompute>();

    UnbindUnorderedAccessViews<kFragment>();
    UnbindUnorderedAccessViews<kCompute>();

    DAWN_ASSERT(CheckAllSlotsAreEmpty(mCommandContext));
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetConstantBuffer(uint32_t idx,
                                         ID3D11Buffer* d3d11Buffer,
                                         uint32_t firstConstant,
                                         uint32_t numConstants) {
    mConstantBufferSlots[Stage].Bind(idx, {d3d11Buffer, firstConstant, numConstants},
                                     [this](size_t idx, const ConstantBufferBinding& binding) {
                                         SetConstantBuffersImpl<Stage>(
                                             mCommandContext->GetD3D11DeviceContext3(), idx, 1,
                                             binding.buffer.GetAddressOf(), &binding.firstConstant,
                                             &binding.numConstants);
                                     });
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindConstantBuffers() {
    const auto slots = mConstantBufferSlots[Stage].MaxBoundSlots();
    if (slots == 0) {
        return;
    }

    static constexpr ID3D11Buffer* kNullBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] =
        {};

    // Note: We already track max bound slots so we can precisely unbind the correct number of used
    // slots. It should be faster than unbinding everything from
    // [0 - D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT).
    SetConstantBuffersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), 0, slots, kNullBuffers,
                                  nullptr, nullptr);
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetShaderResource(uint32_t idx, ID3D11ShaderResourceView* srv) {
    mSRVSlots[Stage].Bind(
        idx, std::move(srv), [this](size_t idx, const ComPtr<ID3D11ShaderResourceView>& binding) {
            SetShaderResourcesImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), idx, 1,
                                          binding.GetAddressOf());
        });
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindShaderResources() {
    const auto slots = mSRVSlots[Stage].MaxBoundSlots();
    if (slots == 0) {
        return;
    }

    static constexpr ID3D11ShaderResourceView*
        kNullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};

    SetShaderResourcesImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), 0, slots, kNullSRVs);
}

template <SingleShaderStage Stage>
void BindGroupTracker::SetSampler(uint32_t idx, ID3D11SamplerState* sampler) {
    mSamplerSlots[Stage].Bind(idx, sampler,
                              [this](size_t idx, const ComPtr<ID3D11SamplerState>& binding) {
                                  SetSamplersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(),
                                                         idx, 1, binding.GetAddressOf());
                              });
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindSamplers() {
    const auto slots = mSamplerSlots[Stage].MaxBoundSlots();
    if (slots == 0) {
        return;
    }

    static constexpr ID3D11SamplerState* kNullSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {};

    SetSamplersImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), 0, slots, kNullSamplers);
}

void BindGroupTracker::CSSetUnorderedAccessView(uint32_t idx, ID3D11UnorderedAccessView* uav) {
    mMinUAVSlots[kCompute] = std::min(mMinUAVSlots[kCompute], idx);

    mCSUAVSlots.Bind(
        idx, std::move(uav), [this](size_t idx, const ComPtr<ID3D11UnorderedAccessView>& binding) {
            SetUnorderedAccessViewsImpl<kCompute>(mCommandContext->GetD3D11DeviceContext3(), idx, 1,
                                                  binding.GetAddressOf());
        });
}

void BindGroupTracker::OMSetUnorderedAccessViews(uint32_t idx,
                                                 uint32_t count,
                                                 ID3D11UnorderedAccessView* const* uavs) {
    mMinUAVSlots[kFragment] = std::min(mMinUAVSlots[kFragment], idx);
    mPSMaxUAVSlot = std::max(mPSMaxUAVSlot, idx + count);

    SetUnorderedAccessViewsImpl<kFragment>(mCommandContext->GetD3D11DeviceContext3(), idx, count,
                                           uavs);
}

template <SingleShaderStage Stage>
void BindGroupTracker::UnbindUnorderedAccessViews() {
    const auto start = mMinUAVSlots[Stage];
    uint32_t end;

    if constexpr (Stage == kFragment) {
        end = mPSMaxUAVSlot;
    } else if constexpr (Stage == kCompute) {
        end = mCSUAVSlots.MaxBoundSlots();
    } else {
        DAWN_UNREACHABLE();
        return;
    }

    if (start >= end) {
        return;
    }

    static constexpr ID3D11UnorderedAccessView* kNullUAVs[D3D11_1_UAV_SLOT_COUNT] = {};

    const auto count = end - start;

    SetUnorderedAccessViewsImpl<Stage>(mCommandContext->GetD3D11DeviceContext3(), start, count,
                                       kNullUAVs);
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

    [[maybe_unused]]
    const auto& layout = std::get<StorageTextureBindingInfo>(bindingInfo.bindingLayout);

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

template <SingleShaderStage Stage, typename D3DType, typename Fn>
MaybeError BindGroupTracker::IterateD3DViewBindings(BindGroupIndex index, Fn&& processFunc) {
    BindGroupMask mask;
    mask.set(index);
    return IterateD3DViewBindings<Stage, D3DType>(mask, std::forward<Fn>(processFunc));
}

template <SingleShaderStage Stage, typename D3DType, typename Fn>
MaybeError BindGroupTracker::IterateD3DViewBindings(BindGroupMask bindGroups, Fn&& processFunc) {
    constexpr const auto GetBufferBindings = [](PipelineLayout* pipelineLayout,
                                                BindGroupIndex index) {
        if constexpr (std::is_same_v<D3DType, ID3D11UnorderedAccessView>) {
            return pipelineLayout->GetBufferUAVBindings()[index][Stage];
        } else {
            return pipelineLayout->GetBufferSRVBindings()[index][Stage];
        }
    };

    constexpr const auto GetTextureBindings = [](PipelineLayout* pipelineLayout,
                                                 BindGroupIndex index) {
        if constexpr (std::is_same_v<D3DType, ID3D11UnorderedAccessView>) {
            return pipelineLayout->GetTextureUAVBindings()[index][Stage];
        } else {
            return pipelineLayout->GetTextureSRVBindings()[index][Stage];
        }
    };

    auto pipelineLayout = ToBackend(mPipelineLayout);

    for (BindGroupIndex index : IterateBitSet(bindGroups)) {
        // buffer bindings
        const auto& bufBindings = GetBufferBindings(pipelineLayout, index);
        if (bufBindings.size()) {
            DAWN_TRY(IteratePerTypeBindings<Stage>(
                index, bufBindings,
                [&](BindGroupBase* group, BindingIndex bindingIndex, const auto& dynamicOffsets,
                    uint32_t nativeIdx) -> MaybeError {
                    ComPtr<D3DType> d3dView;
                    DAWN_TRY_ASSIGN(d3dView,
                                    GetBufferD3DView<D3DType>(group, bindingIndex, dynamicOffsets));

                    return processFunc(nativeIdx, std::move(d3dView));
                }));
        }

        // Texture bindings.
        const auto& texBindings = GetTextureBindings(pipelineLayout, index);
        if (texBindings.size()) {
            DAWN_TRY(IteratePerTypeBindings<Stage>(
                index, texBindings,
                [&](BindGroupBase* group, BindingIndex bindingIndex, const auto& dynamicOffsets,
                    uint32_t nativeIdx) -> MaybeError {
                    ComPtr<D3DType> d3dView;
                    DAWN_TRY_ASSIGN(d3dView, GetTextureD3DView<D3DType>(group, bindingIndex));

                    return processFunc(nativeIdx, std::move(d3dView));
                }));
        }
    }

    return {};
}

template <SingleShaderStage Stage, typename Fn>
MaybeError BindGroupTracker::IteratePerTypeBindings(BindGroupIndex index,
                                                    const std::vector<BindingIndex>& bindings,
                                                    Fn&& processFunc) {
    return IteratePerTypeBindings<Stage>(
        index, ityp::SpanFromUntyped<size_t>(bindings.data(), bindings.size()),
        std::forward<Fn>(processFunc));
}

template <SingleShaderStage Stage, typename Fn>
MaybeError BindGroupTracker::IteratePerTypeBindings(BindGroupIndex index,
                                                    ityp::span<size_t, const BindingIndex> bindings,
                                                    Fn&& processFunc) {
    BindGroupBase* group = mBindGroups[index];
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets = mDynamicOffsets[index];
    const auto& indices = ToBackend(mPipelineLayout)->GetBindingIndexToNativeMap()[index];

    for (BindingIndex bindingIndex : bindings) {
        uint32_t nativeIndex = indices[bindingIndex][Stage];
        DAWN_TRY(processFunc(group, bindingIndex, dynamicOffsets, nativeIndex));
    }

    return {};
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
        DAWN_TRY(IteratePerTypeBindings<Stage>(
            index, cbBindings,
            [&](BindGroupBase* group, BindingIndex bindingIndex, const auto& dynamicOffsets,
                uint32_t nativeIdx) -> MaybeError {
                ConstantBufferBinding bufferBinding;
                DAWN_TRY_ASSIGN(bufferBinding, this->GetConstantBufferBinding(group, bindingIndex,
                                                                              dynamicOffsets));

                this->SetConstantBuffer<Stage>(nativeIdx, bufferBinding.buffer.Get(),
                                               bufferBinding.firstConstant,
                                               bufferBinding.numConstants);
                return {};
            }));
    }

    // SRV bindings.
    DAWN_TRY((IterateD3DViewBindings<Stage, ID3D11ShaderResourceView>(
        index, [&](uint32_t nativeIdx, ComPtr<ID3D11ShaderResourceView> srv) -> MaybeError {
            this->SetShaderResource<Stage>(nativeIdx, srv.Get());
            return {};
        })));

    // Sampler bindings.
    const auto& samplerBindings = pipelineLayout->GetSamplerBindings()[index][Stage];
    if (!samplerBindings.empty()) {
        DAWN_TRY(IteratePerTypeBindings<Stage>(
            index, samplerBindings,
            [&](BindGroupBase* group, BindingIndex bindingIndex, const auto& dynamicOffsets,
                uint32_t nativeIdx) -> MaybeError {
                this->SetSampler<Stage>(nativeIdx, GetSamplerState(group, bindingIndex));
                return {};
            }));
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
    for (auto bindingIndex : pipelineLayout->GetConstantBufferBindings()[index][kCompute]) {
        uint32_t startSlot = indices[bindingIndex][kCompute];
        this->SetConstantBuffer<kCompute>(startSlot, nullptr, 0, 0);
    }

    // SRV bindings.
    for (auto bindingIndex : pipelineLayout->GetSRVBindings()[index][kCompute]) {
        uint32_t startSlot = indices[bindingIndex][kCompute];
        this->SetShaderResource<kCompute>(startSlot, nullptr);
    }

    // Sampler bindings.
    for (auto bindingIndex : pipelineLayout->GetSamplerBindings()[index][kCompute]) {
        uint32_t startSlot = indices[bindingIndex][kCompute];
        this->SetSampler<kCompute>(startSlot, nullptr);
    }

    // UAV bindings
    for (auto bindingIndex : pipelineLayout->GetUAVBindings()[index][kCompute]) {
        uint32_t startSlot = indices[bindingIndex][kCompute];
        this->CSSetUnorderedAccessView(startSlot, nullptr);
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

        DAWN_TRY((IterateD3DViewBindings<kCompute, ID3D11UnorderedAccessView>(
            index, [&](uint32_t nativeIdx, ComPtr<ID3D11UnorderedAccessView> uav) -> MaybeError {
                CSSetUnorderedAccessView(nativeIdx, uav.Get());
                return {};
            })));
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
        const auto uavInBGCount = uavCount - static_cast<uint32_t>(mPixelLocalStorageUAVs.size());
        std::vector<ComPtr<ID3D11UnorderedAccessView>> plsAndUavs(uavCount);

        const BindGroupMask uavBindGroups =
            ToBackend(mPipelineLayout)->GetUAVBindGroupLayoutsMask();
        DAWN_TRY((IterateD3DViewBindings<kFragment, ID3D11UnorderedAccessView>(
            uavBindGroups,
            [&](uint32_t nativeIdx, ComPtr<ID3D11UnorderedAccessView> uav) -> MaybeError {
                DAWN_ASSERT(nativeIdx >= uavStartSlot);
                plsAndUavs[nativeIdx - uavStartSlot] = std::move(uav);
                return {};
            })));

        for (size_t i = 0; i < mPixelLocalStorageUAVs.size(); ++i) {
            plsAndUavs[uavInBGCount + i] = mPixelLocalStorageUAVs[i];
        }

        static_assert(sizeof(ComPtr<ID3D11UnorderedAccessView>) ==
                      sizeof(ID3D11UnorderedAccessView*));
        this->OMSetUnorderedAccessViews(
            /*startSlot=*/uavStartSlot, plsAndUavs.size(),
            reinterpret_cast<ID3D11UnorderedAccessView**>(plsAndUavs.data()));
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
