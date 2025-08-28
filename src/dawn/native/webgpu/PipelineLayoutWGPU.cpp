// Copyright 2025 The Dawn & Tint Authors
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

#include "dawn/native/webgpu/PipelineLayoutWGPU.h"

#include <array>

#include "dawn/common/Constants.h"
#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/BindGroupLayoutWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"

namespace dawn::native::webgpu {

namespace {

class ComboPipelineLayoutDescriptor {
  public:
    explicit ComboPipelineLayoutDescriptor(const PipelineLayoutDescriptor* desc) {
        cDesc.nextInChain = nullptr;
        cDesc.label = ToOutputStringView(desc->label);
        cDesc.bindGroupLayoutCount = desc->bindGroupLayoutCount;
        DAWN_ASSERT(cDesc.bindGroupLayoutCount <= kMaxBindGroups);
        for (uint32_t i = 0; i < desc->bindGroupLayoutCount; ++i) {
            cBindGroupLayouts[i] =
                ToBackend(desc->bindGroupLayouts[i]->GetInternalBindGroupLayout())
                    ->GetInnerHandle();
        }
        cDesc.bindGroupLayouts = cBindGroupLayouts.data();
        cDesc.immediateSize = desc->immediateSize;
    }

    const WGPUPipelineLayoutDescriptor* Get() const { return &cDesc; }

  private:
    WGPUPipelineLayoutDescriptor cDesc;
    std::array<WGPUBindGroupLayout, kMaxBindGroups> cBindGroupLayouts;
};

}  // namespace

// static
Ref<PipelineLayout> PipelineLayout::Create(
    Device* device,
    const UnpackedPtr<PipelineLayoutDescriptor>& descriptor) {
    return AcquireRef(new PipelineLayout(device, descriptor));
}

PipelineLayout::PipelineLayout(Device* device,
                               const UnpackedPtr<PipelineLayoutDescriptor>& descriptor)
    : PipelineLayoutBase(device, descriptor), ObjectWGPU(device->wgpu.pipelineLayoutRelease) {
    ComboPipelineLayoutDescriptor desc(*descriptor);
    mInnerHandle = device->wgpu.deviceCreatePipelineLayout(device->GetInnerHandle(), desc.Get());
    DAWN_ASSERT(mInnerHandle);
}

}  // namespace dawn::native::webgpu
