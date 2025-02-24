// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/d3d12/BindGroupD3D12.h"

#include <utility>

#include "dawn/common/BitSetIterator.h"
#include "dawn/common/MatchVariant.h"
#include "dawn/common/Range.h"
#include "dawn/native/ExternalTexture.h"
#include "dawn/native/Queue.h"
#include "dawn/native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn/native/d3d12/BufferD3D12.h"
#include "dawn/native/d3d12/DeviceD3D12.h"
#include "dawn/native/d3d12/SamplerHeapCacheD3D12.h"
#include "dawn/native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "dawn/native/d3d12/TextureD3D12.h"

namespace dawn::native::d3d12 {

// static
ResultOrError<Ref<BindGroup>> BindGroup::Create(Device* device,
                                                const BindGroupDescriptor* descriptor) {
    return ToBackend(descriptor->layout->GetInternalBindGroupLayout())
        ->AllocateBindGroup(device, descriptor);
}

BindGroup::BindGroup(Device* device,
                     const BindGroupDescriptor* descriptor,
                     const CPUDescriptorHeapAllocation& viewAllocation)
    : BindGroupBase(this, device, descriptor), mCPUViewAllocation(viewAllocation) {}

BindGroup::~BindGroup() = default;

MaybeError BindGroup::InitializeImpl() {
    return {};
}

void BindGroup::DestroyImpl() {
    BindGroupBase::DestroyImpl();
    ToBackend(GetLayout())->DeallocateBindGroup(this, &mCPUViewAllocation);
    DAWN_ASSERT(!mCPUViewAllocation.IsValid());
}

bool BindGroup::PopulateViews(MutexProtected<ShaderVisibleDescriptorAllocator>& viewAllocator) {
    const BindGroupLayout* bgl = ToBackend(GetLayout());

    const uint32_t descriptorCount = bgl->GetCbvUavSrvDescriptorCount();
    if (descriptorCount == 0 || viewAllocator->IsAllocationStillValid(mGPUViewAllocation)) {
        return true;
    }

    // Attempt to allocate descriptors for the currently bound shader-visible heaps.
    // If either failed, return early to re-allocate and switch the heaps.
    Device* device = ToBackend(GetDevice());

    D3D12_CPU_DESCRIPTOR_HANDLE baseCPUDescriptor;
    if (!viewAllocator->AllocateGPUDescriptors(descriptorCount,
                                               device->GetQueue()->GetPendingCommandSerial(),
                                               &baseCPUDescriptor, &mGPUViewAllocation)) {
        return false;
    }

    // CPU bindgroups are sparsely allocated across CPU heaps. Instead of doing
    // simple copies per bindgroup, a single non-simple copy could be issued.
    // TODO(dawn:155): Consider doing this optimization.
    device->GetD3D12Device()->CopyDescriptorsSimple(descriptorCount, baseCPUDescriptor,
                                                    mCPUViewAllocation.GetBaseDescriptor(),
                                                    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE BindGroup::GetBaseViewDescriptor() const {
    return mGPUViewAllocation.GetBaseDescriptor();
}

D3D12_GPU_DESCRIPTOR_HANDLE BindGroup::GetBaseSamplerDescriptor() const {
    DAWN_ASSERT(mSamplerAllocationEntry != nullptr);
    return mSamplerAllocationEntry->GetBaseDescriptor();
}

bool BindGroup::PopulateSamplers(
    MutexProtected<ShaderVisibleDescriptorAllocator>& samplerAllocator) {
    if (mSamplerAllocationEntry == nullptr) {
        return true;
    }
    return mSamplerAllocationEntry->Populate(samplerAllocator);
}

void BindGroup::SetSamplerAllocationEntry(Ref<SamplerHeapCacheEntry> entry) {
    mSamplerAllocationEntry = std::move(entry);
}

void BindGroup::SetDynamicStorageBufferLengths(
    BindGroup::DynamicStorageBufferLengths dynamicStorageBufferLengths) {
    mDynamicStorageBufferLengths = dynamicStorageBufferLengths;
}

const BindGroup::DynamicStorageBufferLengths& BindGroup::GetDynamicStorageBufferLengths() const {
    return mDynamicStorageBufferLengths;
}

}  // namespace dawn::native::d3d12
