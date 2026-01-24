// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/native/d3d12/ResourceTableD3D12.h"

#include <vector>

#include "dawn/common/Enumerator.h"
#include "dawn/native/DynamicUploader.h"
#include "dawn/native/Queue.h"
#include "dawn/native/d3d12/DeviceD3D12.h"
#include "dawn/native/d3d12/ShaderVisibleDescriptorAllocatorD3D12.h"
#include "dawn/native/d3d12/StagingDescriptorAllocatorD3D12.h"

namespace dawn::native::d3d12 {

// static
ResultOrError<Ref<ResourceTable>> ResourceTable::Create(Device* device,
                                                        const ResourceTableDescriptor* descriptor) {
    Ref<ResourceTable> table = AcquireRef(new ResourceTable(device, descriptor));
    DAWN_TRY(table->Initialize());
    return table;
}

ResourceTable::~ResourceTable() = default;

MaybeError ResourceTable::Initialize() {
    DAWN_TRY(ResourceTableBase::InitializeBase());

    Device* device = ToBackend(GetDevice());
    ID3D12Device* d3d12Device = device->GetD3D12Device();

    // Allocate the CPU descriptor heap
    // TODO(amaiorano): This heap is created with the max limit size, rather than the table size,
    // because StagingDescriptorAllocator is configured with a fixed size. Fix this.
    {
        auto& viewAllocator = device->GetResourceTableViewAllocator();
        DAWN_TRY(viewAllocator.Use([&](auto viewAllocator) -> MaybeError {
            DAWN_TRY_ASSIGN(mCPUViewAllocation, viewAllocator->AllocateCPUDescriptors());
            return {};
        }));
        mViewSizeIncrement = viewAllocator->GetSizeIncrement();
    }

    // Only write the metadata buffer to the heap initially, all the other bindings will be written
    // as needed when they are inserted in the ResourceTable.
    {
        Buffer* metadataBuffer = ToBackend(GetMetadataBuffer());
        ID3D12Resource* resource = metadataBuffer->GetD3D12Resource();
        DAWN_ASSERT(resource != nullptr);

        // TODO(amaiorano): copied from BindGroupD3D12.cpp; move to utility instead
        D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
        desc.Buffer.NumElements = metadataBuffer->GetSize() / 4;
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.StructureByteStride = 0;
        desc.Buffer.CounterOffsetInBytes = 0;
        desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

        uint32_t offsetInDescriptorCount = 0;  // Metadata buffer is the first element in the table
        d3d12Device->CreateUnorderedAccessView(
            resource, nullptr, &desc,
            mCPUViewAllocation.OffsetFrom(mViewSizeIncrement, offsetInDescriptorCount));
    }

    return {};
}

// Apply updates to resources or to the metadata buffers that are pending.
MaybeError ResourceTable::ApplyPendingUpdates(CommandRecordingContext* recordingContext) {
    Updates updates = AcquireDirtySlotUpdates();

    if (!updates.metadataUpdates.empty()) {
        DAWN_TRY(UpdateMetadataBuffer(recordingContext, updates.metadataUpdates));
    }
    if (!updates.resourceUpdates.empty()) {
        UpdateResourceBindings(updates.resourceUpdates);
    }

    return {};
}

MaybeError ResourceTable::UpdateMetadataBuffer(CommandRecordingContext* recordingContext,
                                               const std::vector<MetadataUpdate>& updates) {
    Device* device = ToBackend(GetDevice());

    // Allocate enough space for all the data to modify and schedule the copies.
    return device->GetDynamicUploader()->WithUploadReservation(
        sizeof(uint32_t) * updates.size(), kCopyBufferToBufferOffsetAlignment,
        [&](UploadReservation reservation) -> MaybeError {
            uint32_t* stagedData = static_cast<uint32_t*>(reservation.mappedPointer);

            // The metadata buffer will be copied to.
            Buffer* metadataBuffer = ToBackend(GetMetadataBuffer());
            DAWN_ASSERT(metadataBuffer->IsInitialized());
            metadataBuffer->TrackUsageAndTransitionNow(recordingContext,
                                                       wgpu::BufferUsage::CopyDst);

            // Record a CopyBufferRegion for each update
            // TODO(amaiorano): reduce number of calls by copying contiguous regions
            for (auto [i, update] : Enumerate(updates)) {
                stagedData[i] = update.data;  // Copy to staged
                // Copy staged to metadata buffer
                recordingContext->GetCommandList1()->CopyBufferRegion(
                    metadataBuffer->GetD3D12Resource(), update.offset,
                    ToBackend(reservation.buffer)->GetD3D12Resource(),
                    reservation.offsetInBuffer + i * sizeof(uint32_t), sizeof(uint32_t));
            }

            // Transition the buffer back to be used as storage as that's how it will be used for
            // shader-side validation.
            // TODO(amaiorano): Vulkan version passed in kAllStages - something like that on d3d12?
            metadataBuffer->TrackUsageAndTransitionNow(recordingContext, kReadOnlyStorageBuffer);

            return {};
        });
}

void ResourceTable::UpdateResourceBindings(const std::vector<ResourceUpdate>& updates) {
    Device* device = ToBackend(GetDevice());
    ID3D12Device* d3d12Device = device->GetD3D12Device();

    for (const ResourceUpdate& update : updates) {
        // TODO(https://issues.chromium.org/473354063): Support samplers updates.
        // TODO(https://issues.chromium.org/473444515): Support buffer, texel buffers and storage
        // textures.

        auto* view = ToBackend(update.textureView);

        ID3D12Resource* resource = ToBackend(view->GetTexture())->GetD3D12Resource();
        // TODO(amaiorano): See BindGroup::InitializeImpl where this could be nullptr if it was
        // destroyed. Is this possible here?
        DAWN_ASSERT(resource != nullptr);

        // Add 1 to skip metadata descriptor
        uint32_t offsetInDescriptorCount = 1 + static_cast<uint32_t>(update.slot);

        d3d12Device->CreateShaderResourceView(
            resource, &view->GetSRVDescriptor(),
            mCPUViewAllocation.OffsetFrom(mViewSizeIncrement, offsetInDescriptorCount));
    }
}

bool ResourceTable::PopulateViews(MutexProtected<ShaderVisibleDescriptorAllocator>& viewAllocator) {
    if (viewAllocator->IsAllocationStillValid(mGPUViewAllocation)) {
        return true;
    }

    // Attempt to allocate descriptors for the currently bound shader-visible heaps.
    // Return false if allocation fails to indicate that AllocateAndSwitchShaderVisibleHeap should
    // be called.
    Device* device = ToBackend(GetDevice());

    // Copy metadata + resource descriptors
    const uint32_t descriptorCount = 1u + static_cast<uint32_t>(GetSizeWithDefaultResources());

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

void ResourceTable::DestroyImpl(DestroyReason reason) {
    ResourceTableBase::DestroyImpl(reason);

    // TODO(amaiorano): Copied from BindGroup::DestroyImpl and
    // BindGroupLayout::DeallocateDescriptor. Move to shared utility.
    if (mCPUViewAllocation.IsValid()) {
        Device* device = ToBackend(GetDevice());
        auto& viewAllocator = device->GetResourceTableViewAllocator();
        viewAllocator->Deallocate(&mCPUViewAllocation);
    }
    DAWN_ASSERT(!mCPUViewAllocation.IsValid());
}

void ResourceTable::SetLabelImpl() {
    // TODO(crbug.com/473354062): SetDebugName
}

}  // namespace dawn::native::d3d12
