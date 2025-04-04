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
#include <functional>

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
    ityp::array<ResourceType, PerStage<uint32_t>, static_cast<uint8_t>(ResourceType::Count)>
        counters;

    counters[ResourceType::ConstantBuffer] = PerStage<uint32_t>(0);
    counters[ResourceType::SRV] = PerStage<uint32_t>(0);
    counters[ResourceType::Sampler] = PerStage<uint32_t>(0);

    // For d3d11 pixel shaders, the render targets and unordered-access views share the same
    // resource slots when being written out. So we assign UAV binding index decreasingly here.
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-omsetrendertargetsandunorderedaccessviews
    // TODO(dawn:1818): Support testing on both FL11_0 and FL11_1.
    const uint32_t kUAVSlotCount = device->GetUAVSlotCount();
    counters[ResourceType::UAV] = PerStage<uint32_t>(kUAVSlotCount);

    // Reserve last several UAV slots for Pixel Local Storage attachments.
    counters[ResourceType::UAV][SingleShaderStage::Fragment] -=
        static_cast<uint32_t>(GetStorageAttachmentSlots().size());

    using CountIndexFunc = uint32_t (*)(uint32_t*);
    constexpr CountIndexFunc CountIncreasingIndex = [](uint32_t* counter) { return (*counter)++; };

    constexpr CountIndexFunc CountDecreasingIndex = [](uint32_t* counter) { return --(*counter); };

    for (BindGroupIndex group : IterateBitSet(GetBindGroupLayoutsMask())) {
        const BindGroupLayoutInternalBase* bgl = GetBindGroupLayout(group);
        mBindingTableIndexMap[group].resize(bgl->GetBindingCount());

        PerStage<uint32_t> numBufferSRVs(0);
        PerStage<uint32_t> numTextureUAVs(0);

        for (BindingIndex bindingIndex{0}; bindingIndex < bgl->GetBindingCount(); ++bindingIndex) {
            const BindingInfo& bindingInfo = bgl->GetBindingInfo(bindingIndex);

            ResourceType resourceType = MatchVariant(
                bindingInfo.bindingLayout,
                [&](const BufferBindingInfo& layout) -> ResourceType {
                    switch (layout.type) {
                        case wgpu::BufferBindingType::Uniform:
                            return ResourceType::ConstantBuffer;
                        case wgpu::BufferBindingType::Storage:
                        case kInternalStorageBufferBinding:
                            return ResourceType::UAV;
                        case wgpu::BufferBindingType::ReadOnlyStorage:
                        case kInternalReadOnlyStorageBufferBinding:
                            for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                                numBufferSRVs[stage]++;
                            }
                            return ResourceType::SRV;
                        case wgpu::BufferBindingType::BindingNotUsed:
                        case wgpu::BufferBindingType::Undefined:
                            DAWN_UNREACHABLE();
                    }
                    return ResourceType::Count;
                },
                [&](const SamplerBindingInfo&) -> ResourceType { return ResourceType::Sampler; },
                [&](const StaticSamplerBindingInfo&) -> ResourceType {
                    // Static samplers are implemented in the frontend on
                    // D3D11.
                    DAWN_UNREACHABLE();
                    return ResourceType::Count;
                },
                [&](const TextureBindingInfo&) -> ResourceType { return ResourceType::SRV; },
                [&](const StorageTextureBindingInfo& layout) -> ResourceType {
                    switch (layout.access) {
                        case wgpu::StorageTextureAccess::ReadWrite:
                        case wgpu::StorageTextureAccess::WriteOnly:
                            for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                                numTextureUAVs[stage]++;
                            }
                            return ResourceType::UAV;
                        case wgpu::StorageTextureAccess::ReadOnly:
                            return ResourceType::SRV;
                        case wgpu::StorageTextureAccess::BindingNotUsed:
                        case wgpu::StorageTextureAccess::Undefined:
                            DAWN_UNREACHABLE();
                    }
                    return ResourceType::Count;
                },
                [&](const InputAttachmentBindingInfo&) -> ResourceType {
                    DAWN_UNREACHABLE();
                    return ResourceType::Count;
                });

            auto& indexCounter = counters[resourceType];
            CountIndexFunc indexCountOp = CountIncreasingIndex;
            if (resourceType == ResourceType::UAV) {
                mUAVBindGroups.set(group);
                indexCountOp = CountDecreasingIndex;
            }

            for (SingleShaderStage stage : IterateStages(bindingInfo.visibility)) {
                // Assign binding table's index.
                mBindingTableIndexMap[group][bindingIndex][stage] = indexCountOp(&indexCounter[stage]);
                // Track a list of bindings of this type
                mBindings[resourceType][group][stage].push_back(bindingIndex);
            }
        }  // Per bindingIndex

        for (SingleShaderStage stage : IterateStages(kAllStages)) {
            // Make buffer SRV and texture SRV bindings' span
            // Front-end already guarantees that buffers are packed before textures.
            const auto& srvBindings = mBindings[ResourceType::SRV][group][stage];
            const auto bufferSRVCount = numBufferSRVs[stage];
            mBufferSRVBindings[group][stage] =
                ityp::SpanFromUntyped<size_t>(srvBindings.data(), bufferSRVCount);
            mTextureSRVBindings[group][stage] = ityp::SpanFromUntyped<size_t>(
                srvBindings.data() + bufferSRVCount, srvBindings.size() - bufferSRVCount);

            // Make buffers UAV & texture UAV bindings' span
            // Since we assign UAVs in reversed order, the texture UAVs will appear before the
            // buffer UAVs.
            const auto textureUAVCount = numTextureUAVs[stage];
            auto& uavBindings = mBindings[ResourceType::UAV][group][stage];
            std::reverse(uavBindings.begin(), uavBindings.end());
            const auto* uavBindingsPtr = uavBindings.data();

            mTextureUAVBindings[group][stage] =
                ityp::SpanFromUntyped<size_t>(uavBindingsPtr, textureUAVCount);
            mBufferUAVBindings[group][stage] = ityp::SpanFromUntyped<size_t>(
                uavBindingsPtr + textureUAVCount, uavBindings.size() - textureUAVCount);
        }
    }  // Per group

    mUAVStartIndex = counters[ResourceType::UAV];
    for (SingleShaderStage stage : IterateStages(kAllStages)) {
        mUAVAndPLSCount[stage] = kUAVSlotCount - mUAVStartIndex[stage];
    }

    for (SingleShaderStage stage : IterateStages(kAllStages)) {
        DAWN_ASSERT(counters[ResourceType::ConstantBuffer][stage] <= kReservedConstantBufferSlot);
    }

    return {};
}

const PipelineLayout::BindingTableIndexMap& PipelineLayout::GetBindingTableIndexMap() const {
    return mBindingTableIndexMap;
}

const BindGroupMask& PipelineLayout::GetUAVBindGroupLayoutsMask() const {
    return mUAVBindGroups;
}

const PipelineLayout::BindingList& PipelineLayout::GetConstantBufferBindings() const {
    return mBindings[ResourceType::ConstantBuffer];
}
const PipelineLayout::BindingList& PipelineLayout::GetSRVBindings() const {
    return mBindings[ResourceType::SRV];
}
const PipelineLayout::BindingSpan& PipelineLayout::GetBufferSRVBindings() const {
    return mBufferSRVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetTextureSRVBindings() const {
    return mTextureSRVBindings;
}
const PipelineLayout::BindingList& PipelineLayout::GetSamplerBindings() const {
    return mBindings[ResourceType::Sampler];
}
const PipelineLayout::BindingList& PipelineLayout::GetUAVBindings() const {
    return mBindings[ResourceType::UAV];
}
const PipelineLayout::BindingSpan& PipelineLayout::GetBufferUAVBindings() const {
    return mBufferUAVBindings;
}
const PipelineLayout::BindingSpan& PipelineLayout::GetTextureUAVBindings() const {
    return mTextureUAVBindings;
}

}  // namespace dawn::native::d3d11
