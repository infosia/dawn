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

#include "dawn/native/webgpu/ComputePipelineWGPU.h"

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/PipelineLayoutWGPU.h"
#include "dawn/native/webgpu/ShaderModuleWGPU.h"

namespace dawn::native::webgpu {

ComboComputePipelineDescriptor::ComboComputePipelineDescriptor(
    const ComputePipelineDescriptor* desc) {
    cDesc.nextInChain = nullptr;
    cDesc.label = ToOutputStringView(desc->label);
    cDesc.layout = ToBackend(desc->layout)->GetInnerHandle();

    cDesc.compute.nextInChain = nullptr;
    cDesc.compute.module = ToBackend(desc->compute.module)->GetInnerHandle();
    cDesc.compute.entryPoint = ToOutputStringView(desc->compute.entryPoint);
    cDesc.compute.constantCount = desc->compute.constantCount;
    for (uint32_t i = 0; i < desc->compute.constantCount; ++i) {
        auto& c = desc->compute.constants[i];
        cConstants.push_back(WGPUConstantEntry{
            .nextInChain = nullptr,
            .key = ToOutputStringView(c.key),
            .value = c.value,
        });
    }
    cDesc.compute.constants = cConstants.data();
}

// static
Ref<ComputePipeline> ComputePipeline::CreateUninitialized(
    Device* device,
    const UnpackedPtr<ComputePipelineDescriptor>& descriptor) {
    return AcquireRef(new ComputePipeline(device, descriptor));
}

ComputePipeline::ComputePipeline(Device* device,
                                 const UnpackedPtr<ComputePipelineDescriptor>& descriptor)
    : ComputePipelineBase(device, descriptor),
      ObjectWGPU(device->wgpu.computePipelineRelease),
      mDescriptor(*descriptor) {}

MaybeError ComputePipeline::InitializeImpl() {
    auto device = ToBackend(GetDevice());
    mInnerHandle =
        device->wgpu.deviceCreateComputePipeline(device->GetInnerHandle(), mDescriptor.Get());
    DAWN_ASSERT(mInnerHandle);
    return {};
}

}  // namespace dawn::native::webgpu
