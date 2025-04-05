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

#include "dawn/native/d3d11/PipelineLayoutD3D11.h"

#include <algorithm>

#include "dawn/common/BitSetIterator.h"
#include "dawn/common/MatchVariant.h"
#include "dawn/native/BindGroupLayoutInternal.h"
#include "dawn/native/d3d11/DeviceD3D11.h"

namespace dawn::native::d3d11 {

// static
ResultOrError<Ref<PipelineLayout>> PipelineLayout::Create(
    Device* device,
    const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) {
    Ref<PipelineLayout> pipelineLayout = AcquireRef(new PipelineLayout(device, descriptor));
    DAWN_TRY(pipelineLayout->Initialize(device));
    return pipelineLayout;
}

MaybeError PipelineLayout::Initialize(Device* device) {
    PerStage<uint32_t> constantBufferCounter(0);
    PerStage<uint32_t> samplerCounter(0);
    PerStage<uint32_t> shaderResourceViewCounter(0);

    // For d3d11 pixel shaders, the render targets and unordered-access views share the same
    // resource slots when being written out. So we assign UAV binding index decreasingly here.
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-omsetrendertargetsandunorderedaccessviews
    // TODO(dawn:1818): Support testing on both FL11_0 and FL11_1.
    const uint32_t kUAVSlotCount = device->GetUAVSlotCount();
    PerStage<uint32_t> uavBindingCounter(kUAVSlotCount);

    // Reserve last several UAV slots for Pixel Local Storage attachments.
    uavBindingCounter[SingleShaderStage::Fragment] -=
        static_cast<uint32_t>(GetStorageAttachmentSlots().size());

    for (BindGroupIndex group : IterateBitSet(GetBindGroupLayoutsMask())) {
        const BindGroupLayoutInternalBase* bgl = GetBindGroupLayout(group);
        mIndexToNativeMap[group].resize(bgl->GetBindingCount());

        PerStage<uint32_t> numBufferSRVs(0);
        PerStage<uint32_t> numTextureUAVs(0);

        for (BindingIndex bindingIndex{0}; bindingIndex < bgl->GetBindingCount(); ++bindingIndex) {
            const BindingInfo& bindingInfo = bgl->GetBindingInfo(bindingIndex);

            const auto AssignPerStageNativeNonUAVIndex = [&](PerStage<uint32_t>& counters,
                                                             BindingList& bindings) {
                PerStage<uint32_t> nativeIndex(kInvalidSlot);
                for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                    nativeIndex[stage] = counters[stage]++;
                    bindings[group][stage].push_back(bindingIndex);
                }
                mIndexToNativeMap[group][bindingIndex] = nativeIndex;
            };

            const auto AssignPerStageNativeUAVIndex = [&](BindingList& bindings) {
                PerStage<uint32_t> nativeIndex(kInvalidSlot);
                for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                    nativeIndex[stage] = --uavBindingCounter[stage];
                    bindings[group][stage].push_back(bindingIndex);
                }
                mUAVBindGroups.set(group);
                mIndexToNativeMap[group][bindingIndex] = nativeIndex;
            };

            MatchVariant(
                bindingInfo.bindingLayout,
                [&](const BufferBindingInfo& layout) {
                    switch (layout.type) {
                        case wgpu::BufferBindingType::Uniform:
                            AssignPerStageNativeNonUAVIndex(constantBufferCounter,
                                                            mConstantBufferBindings);
                            break;
                        case wgpu::BufferBindingType::Storage:
                        case kInternalStorageBufferBinding:
                            AssignPerStageNativeUAVIndex(mUAVBindings);
                            break;
                        case wgpu::BufferBindingType::ReadOnlyStorage:
                        case kInternalReadOnlyStorageBufferBinding:
                            AssignPerStageNativeNonUAVIndex(shaderResourceViewCounter,
                                                            mSRVBindings);
                            for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                                numBufferSRVs[stage]++;
                            }
                            break;
                        case wgpu::BufferBindingType::BindingNotUsed:
                        case wgpu::BufferBindingType::Undefined:
                            DAWN_UNREACHABLE();
                    }
                },
                [&](const SamplerBindingInfo&) {
                    AssignPerStageNativeNonUAVIndex(samplerCounter, mSamplerBindings);
                },
                [&](const StaticSamplerBindingInfo&) {
                    // Static samplers are implemented in the frontend on
                    // D3D11.
                    DAWN_UNREACHABLE();
                },
                [&](const TextureBindingInfo&) {
                    AssignPerStageNativeNonUAVIndex(shaderResourceViewCounter, mSRVBindings);
                },
                [&](const StorageTextureBindingInfo& layout) {
                    switch (layout.access) {
                        case wgpu::StorageTextureAccess::ReadWrite:
                        case wgpu::StorageTextureAccess::WriteOnly:
                            AssignPerStageNativeUAVIndex(mUAVBindings);
                            for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                                numTextureUAVs[stage]++;
                            }
                            break;
                        case wgpu::StorageTextureAccess::ReadOnly:
                            AssignPerStageNativeNonUAVIndex(shaderResourceViewCounter,
                                                            mSRVBindings);
                            break;
                        case wgpu::StorageTextureAccess::BindingNotUsed:
                        case wgpu::StorageTextureAccess::Undefined:
                            DAWN_UNREACHABLE();
                    }
                },
                [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
        }

        for (SingleShaderStage stage : IterateStages(kAllStages)) {
            // Make buffer SRV and texture SRV bindings' span
            // Front-end already guarantees that buffers are packed before textures.
            const auto& srvBindings = mSRVBindings[group][stage];
            const auto bufferSRVCount = numBufferSRVs[stage];
            mBufferSRVBindings[group][stage] =
                ityp::SpanFromUntyped<size_t>(srvBindings.data(), bufferSRVCount);
            mTextureSRVBindings[group][stage] = ityp::SpanFromUntyped<size_t>(
                srvBindings.data() + bufferSRVCount, srvBindings.size() - bufferSRVCount);

            // Make buffers UAV & texture UAV bindings' span
            // Since we assign UAVs in reversed order, the texture UAVs will appear before the
            // buffer UAVs.
            const auto textureUAVCount = numTextureUAVs[stage];
            auto& uavBindings = mUAVBindings[group][stage];
            std::reverse(uavBindings.begin(), uavBindings.end());
            const auto* uavBindingsPtr = uavBindings.data();

            mTextureUAVBindings[group][stage] =
                ityp::SpanFromUntyped<size_t>(uavBindingsPtr, textureUAVCount);
            mBufferUAVBindings[group][stage] = ityp::SpanFromUntyped<size_t>(
                uavBindingsPtr + textureUAVCount, uavBindings.size() - textureUAVCount);
        }
    }

    mUAVStartIndex = uavBindingCounter;
    for (SingleShaderStage stage : IterateStages(kAllStages)) {
        mUAVCount[stage] = kUAVSlotCount - mUAVStartIndex[stage];
    }

    DAWN_ASSERT([constantBufferCounter] {
        for (SingleShaderStage stage : IterateStages(kAllStages)) {
            if (constantBufferCounter[stage] > kReservedConstantBufferSlot) {
                return false;
            }
        }
        return true;
    }());

    return {};
}

const PipelineLayout::BindingIndexToNativeMap& PipelineLayout::GetBindingIndexToNativeMap() const {
    return mIndexToNativeMap;
}

const BindGroupMask& PipelineLayout::GetUAVBindGroupLayoutsMask() const {
    return mUAVBindGroups;
}

const PipelineLayout::BindingList& PipelineLayout::GetConstantBufferBindings() const {
    return mConstantBufferBindings;
}
const PipelineLayout::BindingList& PipelineLayout::GetSRVBindings() const {
    return mSRVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetBufferSRVBindings() const {
    return mBufferSRVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetTextureSRVBindings() const {
    return mTextureSRVBindings;
}
const PipelineLayout::BindingList& PipelineLayout::GetSamplerBindings() const {
    return mSamplerBindings;
}
const PipelineLayout::BindingList& PipelineLayout::GetUAVBindings() const {
    return mUAVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetBufferUAVBindings() const {
    return mBufferUAVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetTextureUAVBindings() const {
    return mTextureUAVBindings;
}

}  // namespace dawn::native::d3d11
