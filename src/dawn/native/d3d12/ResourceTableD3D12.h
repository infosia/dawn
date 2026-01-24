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

#ifndef SRC_DAWN_NATIVE_D3D12_RESOURCETABLED3D12_H_
#define SRC_DAWN_NATIVE_D3D12_RESOURCETABLED3D12_H_

#include "dawn/common/MutexProtected.h"
#include "dawn/native/Error.h"
#include "dawn/native/ResourceTable.h"
#include "dawn/native/d3d12/CPUDescriptorHeapAllocationD3D12.h"
#include "dawn/native/d3d12/GPUDescriptorHeapAllocationD3D12.h"

namespace dawn::native::d3d12 {

class Device;
class CommandRecordingContext;
class ShaderVisibleDescriptorAllocator;

class ResourceTable final : public ResourceTableBase {
  public:
    static ResultOrError<Ref<ResourceTable>> Create(Device* device,
                                                    const ResourceTableDescriptor* descriptor);

    // Apply updates to resources or to the metadata buffers that are pending.
    MaybeError ApplyPendingUpdates(CommandRecordingContext* recordingContext);

    // Returns true if the ResourceTable was successfully populated (now or on a previous call)
    // in the current allocator's heap. If false is returned, caller should
    // AllocateAndSwitchShaderVisibleHeap and populate again.
    bool PopulateViews(MutexProtected<ShaderVisibleDescriptorAllocator>& viewAllocator);

  protected:
    void DestroyImpl(DestroyReason reason) override;
    void SetLabelImpl() override;

  private:
    ~ResourceTable() override;

    using ResourceTableBase::ResourceTableBase;
    MaybeError Initialize();
    MaybeError UpdateMetadataBuffer(CommandRecordingContext* recordingContext,
                                    const std::vector<MetadataUpdate>& updates);
    void UpdateResourceBindings(const std::vector<ResourceUpdate>& updates);

    GPUDescriptorHeapAllocation mGPUViewAllocation;
    CPUDescriptorHeapAllocation mCPUViewAllocation;
    uint32_t mViewSizeIncrement = 0;
};

}  // namespace dawn::native::d3d12

#endif  // SRC_DAWN_NATIVE_D3D12_RESOURCETABLED3D12_H_
