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
        mDesc.nextInChain = nullptr;
        mDesc.label = ToOutputStringView(desc->label);
        mDesc.bindGroupLayoutCount = desc->bindGroupLayoutCount;
        DAWN_ASSERT(mDesc.bindGroupLayoutCount <= kMaxBindGroups);
        for (uint32_t i = 0; i < desc->bindGroupLayoutCount; ++i) {
            auto layout = desc->bindGroupLayouts[i];
            if (layout == nullptr) {
                // It is allowed to have null bindGroupLayout.
                mBindGroupLayouts[i] = nullptr;
                continue;
            }
            mBindGroupLayouts[i] =
                ToBackend(layout->GetInternalBindGroupLayout())->GetInnerHandle();
        }
        mDesc.bindGroupLayouts = mBindGroupLayouts.data();
        mDesc.immediateSize = desc->immediateSize;
    }

    explicit ComboPipelineLayoutDescriptor(const PipelineLayout* layout) {
        mDesc.nextInChain = nullptr;
        mDesc.label = ToOutputStringView(layout->GetLabel());

        mBindGroupLayouts.fill(nullptr);
        size_t bindGroupLayoutCount = 0;
        for (BindGroupIndex group : layout->GetBindGroupLayoutsMask()) {
            auto bgl = layout->GetFrontendBindGroupLayout(group);
            auto idx = static_cast<size_t>(group);
            bindGroupLayoutCount = idx + 1;
            if (bgl->IsEmpty()) {
                // It is allowed to have null bindGroupLayout.
                mBindGroupLayouts[idx] = nullptr;
                continue;
            }
            mBindGroupLayouts[idx] = ToBackend(layout->GetBindGroupLayout(group))->GetInnerHandle();
        }
        mDesc.bindGroupLayouts = mBindGroupLayouts.data();
        mDesc.bindGroupLayoutCount = bindGroupLayoutCount;
        mDesc.immediateSize = layout->GetImmediateDataRangeByteSize();
    }

    const WGPUPipelineLayoutDescriptor* Get() const { return &mDesc; }

  private:
    WGPUPipelineLayoutDescriptor mDesc;
    std::array<WGPUBindGroupLayout, kMaxBindGroups> mBindGroupLayouts;
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
    ComboPipelineLayoutDescriptor desc(this);
    mInnerHandle = device->wgpu.deviceCreatePipelineLayout(device->GetInnerHandle(), desc.Get());
    DAWN_ASSERT(mInnerHandle);
}

}  // namespace dawn::native::webgpu
