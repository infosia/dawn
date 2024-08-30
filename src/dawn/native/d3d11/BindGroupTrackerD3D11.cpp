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

// Track the used slots in a bitset and collect them into spans.
template <uint32_t Size>
class SlotSpans {
  public:
    // Mark the slot as used.
    void SetSlot(uint32_t slot) {
        mBitset.set(slot);
        if (slot < mSlotMin) {
            mSlotMin = slot;
        }
        if (slot > mSlotMax) {
            mSlotMax = slot;
        }
    }

    // Get the used slots, which are consecutive, into spans.
    std::vector<std::pair<uint32_t, uint32_t>> Spans() const {
        std::vector<std::pair<uint32_t, uint32_t>> spans;
        uint32_t pos = mSlotMin, start = mSlotMin, count = 0, end = mSlotMax + 1;
        while (pos < end) {
            while (pos < end && !mBitset.test(pos)) {
                pos++;
            }
            start = pos;
            while (pos < end && mBitset.test(pos)) {
                pos++;
            }
            count = pos - start;
            if (count > 0) {
                spans.push_back({start, count});
            }
        }
        return spans;
    }

  private:
    std::bitset<Size> mBitset{};
    uint32_t mSlotMin{0};
    uint32_t mSlotMax{0};
};

constexpr uint32_t kReservedCBVSlots = 1;
constexpr uint32_t kCBVSlots =
    D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - kReservedCBVSlots;
constexpr uint32_t kSRVSlots = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
constexpr uint32_t kSamplerSlots = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr uint32_t kUAVSlots = D3D11_1_UAV_SLOT_COUNT;

bool CheckAllSlotsAreEmpty(const ScopedSwapStateCommandRecordingContext* commandContext) {
    auto* deviceContext = commandContext->GetD3D11DeviceContext4();

    // Check constant buffer slots
    for (UINT slot = 0; slot < kCBVSlots; ++slot) {
        ID3D11Buffer* buffer = nullptr;
        deviceContext->VSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
        deviceContext->PSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
        deviceContext->CSGetConstantBuffers1(slot, 1, &buffer, nullptr, nullptr);
        DAWN_ASSERT(buffer == nullptr);
    }

    // Check resource slots
    for (UINT slot = 0; slot < kSRVSlots; ++slot) {
        ID3D11ShaderResourceView* srv = nullptr;
        deviceContext->VSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
        deviceContext->PSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
        deviceContext->CSGetShaderResources(slot, 1, &srv);
        DAWN_ASSERT(srv == nullptr);
    }

    // Check sampler slots
    for (UINT slot = 0; slot < kSamplerSlots; ++slot) {
        ID3D11SamplerState* sampler = nullptr;
        deviceContext->VSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
        deviceContext->PSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
        deviceContext->CSGetSamplers(slot, 1, &sampler);
        DAWN_ASSERT(sampler == nullptr);
    }

    // Check UAV slots for compute
    for (UINT slot = 0; slot < kUAVSlots; ++slot) {
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

void ResetAllRenderSlots(const ScopedSwapStateCommandRecordingContext* commandContext) {
    auto* deviceContext = commandContext->GetD3D11DeviceContext4();

    ID3D11Buffer* d3d11Buffers[kCBVSlots] = {};
    uint32_t num = kCBVSlots;
    deviceContext->VSSetConstantBuffers1(0, num, d3d11Buffers, nullptr, nullptr);
    deviceContext->PSSetConstantBuffers1(0, num, d3d11Buffers, nullptr, nullptr);

    ID3D11ShaderResourceView* d3d11SRVs[kSRVSlots] = {};
    num = kSRVSlots;
    deviceContext->VSSetShaderResources(0, num, d3d11SRVs);
    deviceContext->PSSetShaderResources(0, num, d3d11SRVs);

    ID3D11SamplerState* d3d11Samplers[kSamplerSlots] = {};
    num = kSamplerSlots;
    deviceContext->VSSetSamplers(0, num, d3d11Samplers);
    deviceContext->PSSetSamplers(0, num, d3d11Samplers);

    ID3D11UnorderedAccessView* d3d11UAVs[kUAVSlots] = {};
    num = commandContext->GetDevice()->GetUAVSlotCount();
    deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, num, d3d11UAVs, nullptr);
}

}  // namespace

// This immediately binds resources to D3D11 device context.
class D3D11SlotTracker {
  public:
    virtual void BindComputeUAV(ID3D11DeviceContext4* deviceContext,
                                uint32_t slot,
                                ComPtr<ID3D11UnorderedAccessView> view) {
        deviceContext->CSSetUnorderedAccessViews(slot, 1, view.GetAddressOf(), nullptr);
    }

    virtual void BindVertexCBV(ID3D11DeviceContext4* deviceContext,
                               uint32_t slot,
                               ComPtr<ID3D11Buffer> buffer,
                               uint32_t firstConstant,
                               uint32_t numConstants) {
        deviceContext->VSSetConstantBuffers1(slot, 1, buffer.GetAddressOf(), &firstConstant,
                                             &numConstants);
    }
    virtual void BindPixelCBV(ID3D11DeviceContext4* deviceContext,
                              uint32_t slot,
                              ComPtr<ID3D11Buffer> buffer,
                              uint32_t firstConstant,
                              uint32_t numConstants) {
        deviceContext->PSSetConstantBuffers1(slot, 1, buffer.GetAddressOf(), &firstConstant,
                                             &numConstants);
    }
    virtual void BindComputeCBV(ID3D11DeviceContext4* deviceContext,
                                uint32_t slot,
                                ComPtr<ID3D11Buffer> buffer,
                                uint32_t firstConstant,
                                uint32_t numConstants) {
        deviceContext->CSSetConstantBuffers1(slot, 1, buffer.GetAddressOf(), &firstConstant,
                                             &numConstants);
    }

    virtual void BindVertexSRV(ID3D11DeviceContext4* deviceContext,
                               uint32_t slot,
                               ComPtr<ID3D11ShaderResourceView> view) {
        deviceContext->VSSetShaderResources(slot, 1, view.GetAddressOf());
    }
    virtual void BindPixelSRV(ID3D11DeviceContext4* deviceContext,
                              uint32_t slot,
                              ComPtr<ID3D11ShaderResourceView> view) {
        deviceContext->PSSetShaderResources(slot, 1, view.GetAddressOf());
    }
    virtual void BindComputeSRV(ID3D11DeviceContext4* deviceContext,
                                uint32_t slot,
                                ComPtr<ID3D11ShaderResourceView> view) {
        deviceContext->CSSetShaderResources(slot, 1, view.GetAddressOf());
    }

    virtual void BindVertexSampler(ID3D11DeviceContext4* deviceContext,
                                   uint32_t slot,
                                   ComPtr<ID3D11SamplerState> sampler) {
        deviceContext->VSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
    virtual void BindPixelSampler(ID3D11DeviceContext4* deviceContext,
                                  uint32_t slot,
                                  ComPtr<ID3D11SamplerState> sampler) {
        deviceContext->PSSetSamplers(slot, 1, sampler.GetAddressOf());
    }
    virtual void BindComputeSampler(ID3D11DeviceContext4* deviceContext,
                                    uint32_t slot,
                                    ComPtr<ID3D11SamplerState> sampler) {
        deviceContext->CSSetSamplers(slot, 1, sampler.GetAddressOf());
    }

    virtual void Commit(ID3D11DeviceContext4* deviceContext, bool isRenderPass) {}
};

// This class lazily records the slot bindings and commits them all at once. However it doesn't
// work if the backend validation is enabled.
class D3D11SlotTrackerLazy : public D3D11SlotTracker {
  public:
    void BindComputeUAV(ID3D11DeviceContext4* deviceContext,
                        uint32_t slot,
                        ComPtr<ID3D11UnorderedAccessView> view) override {
        mComputeUAVSlotSpans.SetSlot(slot);
        mComputeUAVViews[slot] = view;
        mComputeUAVRawViews[slot] = view.Get();
    }

    void BindVertexCBV(ID3D11DeviceContext4* deviceContext,
                       uint32_t slot,
                       ComPtr<ID3D11Buffer> buffer,
                       uint32_t firstConstant,
                       uint32_t numConstants) override {
        mVertexCBVSlotSpans.SetSlot(slot);
        mVertexCBVBuffers[slot] = buffer;
        mVertexCBVRawBuffers[slot] = buffer.Get();
        mVertexCBVFirstConstants[slot] = firstConstant;
        mVertexCBVNumConstants[slot] = numConstants;
    }
    void BindPixelCBV(ID3D11DeviceContext4* deviceContext,
                      uint32_t slot,
                      ComPtr<ID3D11Buffer> buffer,
                      uint32_t firstConstant,
                      uint32_t numConstants) override {
        mPixelCBVSlotSpans.SetSlot(slot);
        mPixelCBVBuffers[slot] = buffer;
        mPixelCBVRawBuffers[slot] = buffer.Get();
        mPixelCBVFirstConstants[slot] = firstConstant;
        mPixelCBVNumConstants[slot] = numConstants;
    }
    void BindComputeCBV(ID3D11DeviceContext4* deviceContext,
                        uint32_t slot,
                        ComPtr<ID3D11Buffer> buffer,
                        uint32_t firstConstant,
                        uint32_t numConstants) override {
        mComputeCBVSlotSpans.SetSlot(slot);
        mComputeCBVBuffers[slot] = buffer;
        mComputeCBVRawBuffers[slot] = buffer.Get();
        mComputeCBVFirstConstants[slot] = firstConstant;
        mComputeCBVNumConstants[slot] = numConstants;
    }

    void BindVertexSRV(ID3D11DeviceContext4* deviceContext,
                       uint32_t slot,
                       ComPtr<ID3D11ShaderResourceView> view) override {
        mVertexSRVSlotSpans.SetSlot(slot);
        mVertexSRVViews[slot] = view;
        mVertexSRVRawViews[slot] = view.Get();
    }

    void BindPixelSRV(ID3D11DeviceContext4* deviceContext,
                      uint32_t slot,
                      ComPtr<ID3D11ShaderResourceView> view) override {
        mPixelSRVSlotSpans.SetSlot(slot);
        mPixelSRVViews[slot] = view;
        mPixelSRVRawViews[slot] = view.Get();
    }

    void BindComputeSRV(ID3D11DeviceContext4* deviceContext,
                        uint32_t slot,
                        ComPtr<ID3D11ShaderResourceView> view) override {
        mComputeSRVSlotSpans.SetSlot(slot);
        mComputeSRVViews[slot] = view;
        mComputeSRVRawViews[slot] = view.Get();
    }

    void BindVertexSampler(ID3D11DeviceContext4* deviceContext,
                           uint32_t slot,
                           ComPtr<ID3D11SamplerState> sampler) override {
        mVertexSamplerSlotSpans.SetSlot(slot);
        mVertexSamplers[slot] = sampler;
        mVertexRawSamplers[slot] = sampler.Get();
    }

    void BindPixelSampler(ID3D11DeviceContext4* deviceContext,
                          uint32_t slot,
                          ComPtr<ID3D11SamplerState> sampler) override {
        mPixelSamplerSlotSpans.SetSlot(slot);
        mPixelSamplers[slot] = sampler;
        mPixelRawSamplers[slot] = sampler.Get();
    }

    void BindComputeSampler(ID3D11DeviceContext4* deviceContext,
                            uint32_t slot,
                            ComPtr<ID3D11SamplerState> sampler) override {
        mComputeSamplerSlotSpans.SetSlot(slot);
        mComputeSamplers[slot] = sampler;
        mComputeRawSamplers[slot] = sampler.Get();
    }

    void Commit(ID3D11DeviceContext4* deviceContext, bool isRenderPass) override {
        if (isRenderPass) {
            for (const auto& [start, count] : mVertexCBVSlotSpans.Spans()) {
                deviceContext->VSSetConstantBuffers1(start, count, &mVertexCBVRawBuffers[start],
                                                     &mVertexCBVFirstConstants[start],
                                                     &mVertexCBVNumConstants[start]);
            }
            for (const auto& [start, count] : mPixelCBVSlotSpans.Spans()) {
                deviceContext->PSSetConstantBuffers1(start, count, &mPixelCBVRawBuffers[start],
                                                     &mPixelCBVFirstConstants[start],
                                                     &mPixelCBVNumConstants[start]);
            }

            for (const auto& [start, count] : mVertexSRVSlotSpans.Spans()) {
                deviceContext->VSSetShaderResources(start, count, &mVertexSRVRawViews[start]);
            }
            for (const auto& [start, count] : mPixelSRVSlotSpans.Spans()) {
                deviceContext->PSSetShaderResources(start, count, &mPixelSRVRawViews[start]);
            }

            for (const auto& [start, count] : mVertexSamplerSlotSpans.Spans()) {
                deviceContext->VSSetSamplers(start, count, &mVertexRawSamplers[start]);
            }
            for (const auto& [start, count] : mPixelSamplerSlotSpans.Spans()) {
                deviceContext->PSSetSamplers(start, count, &mPixelRawSamplers[start]);
            }
        } else {
            for (const auto& [start, count] : mComputeUAVSlotSpans.Spans()) {
                deviceContext->CSSetUnorderedAccessViews(start, count, &mComputeUAVRawViews[start],
                                                         nullptr);
            }
            for (const auto& [start, count] : mComputeCBVSlotSpans.Spans()) {
                deviceContext->CSSetConstantBuffers1(start, count, &mComputeCBVRawBuffers[start],
                                                     &mComputeCBVFirstConstants[start],
                                                     &mComputeCBVNumConstants[start]);
            }
            for (const auto& [start, count] : mComputeSRVSlotSpans.Spans()) {
                deviceContext->CSSetShaderResources(start, count, &mComputeSRVRawViews[start]);
            }
            for (const auto& [start, count] : mComputeSamplerSlotSpans.Spans()) {
                deviceContext->CSSetSamplers(start, count, &mComputeRawSamplers[start]);
            }
        }
    }

  private:
    SlotSpans<kUAVSlots> mComputeUAVSlotSpans{};
    std::array<ComPtr<ID3D11UnorderedAccessView>, kUAVSlots> mComputeUAVViews{};
    std::array<ID3D11UnorderedAccessView*, kUAVSlots> mComputeUAVRawViews{};

    SlotSpans<kCBVSlots> mVertexCBVSlotSpans{};
    std::array<ComPtr<ID3D11Buffer>, kCBVSlots> mVertexCBVBuffers{};
    std::array<ID3D11Buffer*, kCBVSlots> mVertexCBVRawBuffers{};
    std::array<uint32_t, kCBVSlots> mVertexCBVFirstConstants{};
    std::array<uint32_t, kCBVSlots> mVertexCBVNumConstants{};

    SlotSpans<kCBVSlots> mPixelCBVSlotSpans{};
    std::array<ComPtr<ID3D11Buffer>, kCBVSlots> mPixelCBVBuffers{};
    std::array<ID3D11Buffer*, kCBVSlots> mPixelCBVRawBuffers{};
    std::array<uint32_t, kCBVSlots> mPixelCBVFirstConstants{};
    std::array<uint32_t, kCBVSlots> mPixelCBVNumConstants{};

    SlotSpans<kCBVSlots> mComputeCBVSlotSpans{};
    std::array<ComPtr<ID3D11Buffer>, kCBVSlots> mComputeCBVBuffers{};
    std::array<ID3D11Buffer*, kCBVSlots> mComputeCBVRawBuffers{};
    std::array<uint32_t, kCBVSlots> mComputeCBVFirstConstants{};
    std::array<uint32_t, kCBVSlots> mComputeCBVNumConstants{};

    SlotSpans<kSRVSlots> mVertexSRVSlotSpans{};
    std::array<ComPtr<ID3D11ShaderResourceView>, kSRVSlots> mVertexSRVViews{};
    std::array<ID3D11ShaderResourceView*, kSRVSlots> mVertexSRVRawViews{};

    SlotSpans<kSRVSlots> mPixelSRVSlotSpans{};
    std::array<ComPtr<ID3D11ShaderResourceView>, kSRVSlots> mPixelSRVViews{};
    std::array<ID3D11ShaderResourceView*, kSRVSlots> mPixelSRVRawViews{};

    SlotSpans<kSRVSlots> mComputeSRVSlotSpans{};
    std::array<ComPtr<ID3D11ShaderResourceView>, kSRVSlots> mComputeSRVViews{};
    std::array<ID3D11ShaderResourceView*, kSRVSlots> mComputeSRVRawViews{};

    SlotSpans<kSamplerSlots> mVertexSamplerSlotSpans{};
    std::array<ComPtr<ID3D11SamplerState>, kSamplerSlots> mVertexSamplers{};
    std::array<ID3D11SamplerState*, kSamplerSlots> mVertexRawSamplers{};

    SlotSpans<kSamplerSlots> mPixelSamplerSlotSpans{};
    std::array<ComPtr<ID3D11SamplerState>, kSamplerSlots> mPixelSamplers{};
    std::array<ID3D11SamplerState*, kSamplerSlots> mPixelRawSamplers{};

    SlotSpans<kSamplerSlots> mComputeSamplerSlotSpans{};
    std::array<ComPtr<ID3D11SamplerState>, kSamplerSlots> mComputeSamplers{};
    std::array<ID3D11SamplerState*, kSamplerSlots> mComputeRawSamplers{};
};

BindGroupTracker::BindGroupTracker(
    const ScopedSwapStateCommandRecordingContext* commandContext,
    bool isRenderPass,
    std::vector<ComPtr<ID3D11UnorderedAccessView>> pixelLocalStorageUAVs)
    : mCommandContext(commandContext),
      mIsRenderPass(isRenderPass),
      mVisibleStages(isRenderPass ? wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment
                                  : wgpu::ShaderStage::Compute),
      mPixelLocalStorageUAVs(std::move(pixelLocalStorageUAVs)) {
    mLastAppliedPipelineLayout = mCommandContext->GetDevice()->GetEmptyPipelineLayout();
}

BindGroupTracker::~BindGroupTracker() {
    if (mIsRenderPass) {
        ResetAllRenderSlots(mCommandContext);
    } else {
        D3D11SlotTrackerLazy d3d11SlotTracker{};
        for (BindGroupIndex index :
             IterateBitSet(mLastAppliedPipelineLayout->GetBindGroupLayoutsMask())) {
            UnApplyBindGroup(index, &d3d11SlotTracker);
        }
        d3d11SlotTracker.Commit(mCommandContext->GetD3D11DeviceContext4(), /*IsRenderPass=*/false);
    }
    // All slots should be unbound here.
    DAWN_ASSERT(CheckAllSlotsAreEmpty(mCommandContext));
}

MaybeError BindGroupTracker::Apply() {
    BeforeApply();

    // TODO: We can only use the D3D11SlotTrackerLazy when the backend validation is not enabled.
    D3D11SlotTrackerLazy d3d11SlotTracker{};

    if (mIsRenderPass) {
        // As D3d11 requires to bind all UAVs slots at the same time for pixel shaders, we record
        // all UAV slot assignments in the bind groups, and then bind them all together.
        const BindGroupMask uavBindGroups =
            ToBackend(mPipelineLayout)->GetUAVBindGroupLayoutsMask();
        std::vector<ComPtr<ID3D11UnorderedAccessView>> uavsInBindGroup;
        for (BindGroupIndex index : IterateBitSet(uavBindGroups)) {
            BindGroupBase* group = mBindGroups[index];
            const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets = mDynamicOffsets[index];

            for (BindingIndex bindingIndex : Range(group->GetLayout()->GetBindingCount())) {
                const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);

                DAWN_TRY(MatchVariant(
                    bindingInfo.bindingLayout,
                    [&](const BufferBindingInfo& layout) -> MaybeError {
                        BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
                        auto offset = binding.offset;
                        if (layout.hasDynamicOffset) {
                            // Dynamic buffers are packed at the front of BindingIndices.
                            offset += dynamicOffsets[bindingIndex];
                        }

                        switch (layout.type) {
                            case wgpu::BufferBindingType::Storage:
                            case kInternalStorageBufferBinding: {
                                DAWN_ASSERT(IsSubset(
                                    bindingInfo.visibility,
                                    wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute));
                                ComPtr<ID3D11UnorderedAccessView> d3d11UAV;
                                DAWN_TRY_ASSIGN(d3d11UAV, ToGPUUsableBuffer(binding.buffer)
                                                              ->UseAsUAV(mCommandContext, offset,
                                                                         binding.size));
                                uavsInBindGroup.insert(uavsInBindGroup.begin(),
                                                       std::move(d3d11UAV));
                                break;
                            }
                            case wgpu::BufferBindingType::Uniform:
                            case wgpu::BufferBindingType::ReadOnlyStorage:
                            case wgpu::BufferBindingType::Undefined: {
                                break;
                            }
                        }
                        return {};
                    },
                    [&](const StorageTextureBindingInfo& layout) -> MaybeError {
                        switch (layout.access) {
                            case wgpu::StorageTextureAccess::WriteOnly:
                            case wgpu::StorageTextureAccess::ReadWrite: {
                                ComPtr<ID3D11UnorderedAccessView> d3d11UAV;
                                TextureView* view =
                                    ToBackend(group->GetBindingAsTextureView(bindingIndex));
                                DAWN_TRY_ASSIGN(d3d11UAV,
                                                view->GetOrCreateD3D11UnorderedAccessView());
                                uavsInBindGroup.insert(uavsInBindGroup.begin(),
                                                       std::move(d3d11UAV));
                                break;
                            }
                            case wgpu::StorageTextureAccess::ReadOnly:
                                break;
                            default:
                                DAWN_UNREACHABLE();
                                break;
                        }
                        return {};
                    },
                    [](const TextureBindingInfo&) -> MaybeError { return {}; },
                    [](const SamplerBindingInfo&) -> MaybeError { return {}; },
                    [](const StaticSamplerBindingInfo&) -> MaybeError {
                        // Static samplers are implemented in the frontend on
                        // D3D11.
                        DAWN_UNREACHABLE();
                        return {};
                    },
                    [](const InputAttachmentBindingInfo&) -> MaybeError {
                        DAWN_UNREACHABLE();
                        return {};
                    }));
            }
        }

        uint32_t uavSlotCount = ToBackend(mPipelineLayout->GetDevice())->GetUAVSlotCount();
        std::vector<ID3D11UnorderedAccessView*> views;
        views.reserve(uavsInBindGroup.size() + mPixelLocalStorageUAVs.size());
        for (auto& uav : uavsInBindGroup) {
            views.push_back(uav.Get());
        }
        for (auto& uav : mPixelLocalStorageUAVs) {
            views.push_back(uav.Get());
        }
        if (!views.empty()) {
            DAWN_ASSERT(uavSlotCount >= views.size());
            mCommandContext->GetD3D11DeviceContext4()->OMSetRenderTargetsAndUnorderedAccessViews(
                D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
                uavSlotCount - views.size(), views.size(), views.data(), nullptr);
        }
    } else {
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
            UnApplyBindGroup(index, &d3d11SlotTracker);
        }
    }

    for (BindGroupIndex index : IterateBitSet(mDirtyBindGroupsObjectChangedOrIsDynamic)) {
        DAWN_TRY(ApplyBindGroup(index, &d3d11SlotTracker));
    }
    d3d11SlotTracker.Commit(mCommandContext->GetD3D11DeviceContext4(), mIsRenderPass);

    AfterApply();

    return {};
}

MaybeError BindGroupTracker::ApplyBindGroup(BindGroupIndex index,
                                            D3D11SlotTracker* d3d11SlotTracker) {
    auto* deviceContext = mCommandContext->GetD3D11DeviceContext4();
    BindGroupBase* group = mBindGroups[index];
    const ityp::vector<BindingIndex, uint64_t>& dynamicOffsets = mDynamicOffsets[index];
    const auto& indices = ToBackend(mPipelineLayout)->GetBindingIndexInfo()[index];

    for (BindingIndex bindingIndex : Range(group->GetLayout()->GetBindingCount())) {
        const BindingInfo& bindingInfo = group->GetLayout()->GetBindingInfo(bindingIndex);
        const uint32_t bindingSlot = indices[bindingIndex];
        const auto bindingVisibility = bindingInfo.visibility & mVisibleStages;

        DAWN_TRY(MatchVariant(
            bindingInfo.bindingLayout,
            [&](const BufferBindingInfo& layout) -> MaybeError {
                BufferBinding binding = group->GetBindingAsBufferBinding(bindingIndex);
                auto offset = binding.offset;
                if (layout.hasDynamicOffset) {
                    // Dynamic buffers are packed at the front of BindingIndices.
                    offset += dynamicOffsets[bindingIndex];
                }

                switch (layout.type) {
                    case wgpu::BufferBindingType::Uniform: {
                        ID3D11Buffer* d3d11Buffer;
                        DAWN_TRY_ASSIGN(d3d11Buffer, ToGPUUsableBuffer(binding.buffer)
                                                         ->GetD3D11ConstantBuffer(mCommandContext));
                        // https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-vssetconstantbuffers1
                        // Offset and size are measured in shader constants, which are 16 bytes
                        // (4*32-bit components). And the offsets and counts must be multiples
                        // of 16.
                        // WebGPU's minUniformBufferOffsetAlignment is 256.
                        DAWN_ASSERT(IsAligned(offset, 256));
                        uint32_t firstConstant = static_cast<uint32_t>(offset / 16);
                        uint32_t size = static_cast<uint32_t>(Align(binding.size, 16) / 16);
                        uint32_t numConstants = Align(size, 16);
                        DAWN_ASSERT(offset + numConstants * 16 <=
                                    binding.buffer->GetAllocatedSize());

                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexCBV(deviceContext, bindingSlot, d3d11Buffer,
                                                            firstConstant, numConstants);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelCBV(deviceContext, bindingSlot, d3d11Buffer,
                                                           firstConstant, numConstants);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeCBV(deviceContext, bindingSlot,
                                                             d3d11Buffer, firstConstant,
                                                             numConstants);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::Storage:
                    case kInternalStorageBufferBinding: {
                        DAWN_ASSERT(
                            IsSubset(bindingInfo.visibility,
                                     wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute));
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            ComPtr<ID3D11UnorderedAccessView> d3d11UAV;
                            DAWN_TRY_ASSIGN(d3d11UAV,
                                            ToGPUUsableBuffer(binding.buffer)
                                                ->UseAsUAV(mCommandContext, offset, binding.size));
                            d3d11SlotTracker->BindComputeUAV(deviceContext, bindingSlot, d3d11UAV);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::ReadOnlyStorage: {
                        ComPtr<ID3D11ShaderResourceView> d3d11SRV;
                        DAWN_TRY_ASSIGN(d3d11SRV,
                                        ToGPUUsableBuffer(binding.buffer)
                                            ->UseAsSRV(mCommandContext, offset, binding.size));
                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::Undefined:
                        DAWN_UNREACHABLE();
                }
                return {};
            },
            [&](const StaticSamplerBindingInfo&) -> MaybeError {
                // Static samplers are implemented in the frontend on
                // D3D11.
                DAWN_UNREACHABLE();
                return {};
            },
            [&](const SamplerBindingInfo&) -> MaybeError {
                Sampler* sampler = ToBackend(group->GetBindingAsSampler(bindingIndex));
                ID3D11SamplerState* d3d11SamplerState = sampler->GetD3D11SamplerState();
                if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                    d3d11SlotTracker->BindVertexSampler(deviceContext, bindingSlot,
                                                        d3d11SamplerState);
                }
                if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                    d3d11SlotTracker->BindPixelSampler(deviceContext, bindingSlot,
                                                       d3d11SamplerState);
                }
                if (bindingVisibility & wgpu::ShaderStage::Compute) {
                    d3d11SlotTracker->BindComputeSampler(deviceContext, bindingSlot,
                                                         d3d11SamplerState);
                }
                return {};
            },
            [&](const TextureBindingInfo&) -> MaybeError {
                TextureView* view = ToBackend(group->GetBindingAsTextureView(bindingIndex));
                ComPtr<ID3D11ShaderResourceView> srv;
                // For sampling from stencil, we have to use an internal mirror 'R8Uint' texture.
                if (view->GetAspects() == Aspect::Stencil) {
                    DAWN_TRY_ASSIGN(
                        srv, ToBackend(view->GetTexture())->GetStencilSRV(mCommandContext, view));
                } else {
                    DAWN_TRY_ASSIGN(srv, view->GetOrCreateD3D11ShaderResourceView());
                }
                if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                    d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, srv);
                }
                if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                    d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, srv);
                }
                if (bindingVisibility & wgpu::ShaderStage::Compute) {
                    d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, srv);
                }
                return {};
            },
            [&](const StorageTextureBindingInfo& layout) -> MaybeError {
                TextureView* view = ToBackend(group->GetBindingAsTextureView(bindingIndex));
                switch (layout.access) {
                    case wgpu::StorageTextureAccess::WriteOnly:
                    case wgpu::StorageTextureAccess::ReadWrite: {
                        ID3D11UnorderedAccessView* d3d11UAV = nullptr;
                        DAWN_TRY_ASSIGN(d3d11UAV, view->GetOrCreateD3D11UnorderedAccessView());
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeUAV(deviceContext, bindingSlot, d3d11UAV);
                        }
                        break;
                    }
                    case wgpu::StorageTextureAccess::ReadOnly: {
                        ID3D11ShaderResourceView* d3d11SRV = nullptr;
                        DAWN_TRY_ASSIGN(d3d11SRV, view->GetOrCreateD3D11ShaderResourceView());
                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, d3d11SRV);
                        }
                        break;
                    }
                    default:
                        DAWN_UNREACHABLE();
                }
                return {};
            },
            [](const InputAttachmentBindingInfo&) -> MaybeError {
                DAWN_UNREACHABLE();
                return {};
            }));
    }
    return {};
}

void BindGroupTracker::UnApplyBindGroup(BindGroupIndex index, D3D11SlotTracker* d3d11SlotTracker) {
    auto* deviceContext = mCommandContext->GetD3D11DeviceContext4();
    BindGroupLayoutInternalBase* groupLayout =
        mLastAppliedPipelineLayout->GetBindGroupLayout(index);
    const auto& indices = ToBackend(mLastAppliedPipelineLayout)->GetBindingIndexInfo()[index];

    for (BindingIndex bindingIndex : Range(groupLayout->GetBindingCount())) {
        const BindingInfo& bindingInfo = groupLayout->GetBindingInfo(bindingIndex);
        const uint32_t bindingSlot = indices[bindingIndex];
        const auto bindingVisibility = bindingInfo.visibility & mVisibleStages;

        MatchVariant(
            bindingInfo.bindingLayout,
            [&](const BufferBindingInfo& layout) {
                switch (layout.type) {
                    case wgpu::BufferBindingType::Uniform: {
                        ID3D11Buffer* nullBuffer = nullptr;
                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexCBV(deviceContext, bindingSlot, nullBuffer,
                                                            0, 0);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelCBV(deviceContext, bindingSlot, nullBuffer,
                                                           0, 0);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeCBV(deviceContext, bindingSlot, nullBuffer,
                                                             0, 0);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::Storage:
                    case kInternalStorageBufferBinding: {
                        DAWN_ASSERT(
                            IsSubset(bindingInfo.visibility,
                                     wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute));
                        ID3D11UnorderedAccessView* nullUAV = nullptr;
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
                                D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
                                bindingSlot, 1, &nullUAV, nullptr);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeUAV(deviceContext, bindingSlot, nullUAV);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::ReadOnlyStorage: {
                        ID3D11ShaderResourceView* nullSRV = nullptr;
                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        break;
                    }
                    case wgpu::BufferBindingType::Undefined:
                        DAWN_UNREACHABLE();
                }
            },
            [&](const StaticSamplerBindingInfo&) {
                // Static samplers are implemented in the frontend on
                // D3D11.
                DAWN_UNREACHABLE();
            },
            [&](const SamplerBindingInfo&) {
                ID3D11SamplerState* nullSampler = nullptr;
                if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                    d3d11SlotTracker->BindVertexSampler(deviceContext, bindingSlot, nullSampler);
                }
                if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                    d3d11SlotTracker->BindPixelSampler(deviceContext, bindingSlot, nullSampler);
                }
                if (bindingVisibility & wgpu::ShaderStage::Compute) {
                    d3d11SlotTracker->BindComputeSampler(deviceContext, bindingSlot, nullSampler);
                }
            },
            [&](const TextureBindingInfo&) {
                ID3D11ShaderResourceView* nullSRV = nullptr;
                if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                    d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, nullSRV);
                }
                if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                    d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, nullSRV);
                }
                if (bindingVisibility & wgpu::ShaderStage::Compute) {
                    d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, nullSRV);
                }
            },
            [&](const StorageTextureBindingInfo& layout) {
                switch (layout.access) {
                    case wgpu::StorageTextureAccess::WriteOnly:
                    case wgpu::StorageTextureAccess::ReadWrite: {
                        ID3D11UnorderedAccessView* nullUAV = nullptr;
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
                                D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr,
                                bindingSlot, 1, &nullUAV, nullptr);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeUAV(deviceContext, bindingSlot, nullUAV);
                        }
                        break;
                    }
                    case wgpu::StorageTextureAccess::ReadOnly: {
                        ID3D11ShaderResourceView* nullSRV = nullptr;
                        if (bindingVisibility & wgpu::ShaderStage::Vertex) {
                            d3d11SlotTracker->BindVertexSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Fragment) {
                            d3d11SlotTracker->BindPixelSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        if (bindingVisibility & wgpu::ShaderStage::Compute) {
                            d3d11SlotTracker->BindComputeSRV(deviceContext, bindingSlot, nullSRV);
                        }
                        break;
                    }
                    default:
                        DAWN_UNREACHABLE();
                }
            },
            [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
    }
}

}  // namespace dawn::native::d3d11
