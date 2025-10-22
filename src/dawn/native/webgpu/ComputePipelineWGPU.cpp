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

#include <limits>
#include <string>
#include <vector>
#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/PipelineLayoutWGPU.h"
#include "dawn/native/webgpu/ShaderModuleWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

// static
Ref<ComputePipeline> ComputePipeline::CreateUninitialized(
    Device* device,
    const UnpackedPtr<ComputePipelineDescriptor>& descriptor) {
    return AcquireRef(new ComputePipeline(device, descriptor));
}

ComputePipeline::ComputePipeline(Device* device,
                                 const UnpackedPtr<ComputePipelineDescriptor>& descriptor)
    : ComputePipelineBase(device, descriptor), ObjectWGPU(device->wgpu.computePipelineRelease) {}

MaybeError ComputePipeline::InitializeImpl() {
    WGPUComputePipelineDescriptor desc;
    desc.nextInChain = nullptr;
    desc.label = ToOutputStringView(GetLabel());
    const PipelineLayoutBase* layout = GetLayout();
    if (layout != nullptr) {
        desc.layout = ToBackend(layout)->GetInnerHandle();
    } else {
        desc.layout = nullptr;
    }

    const ProgrammableStage& stage = GetStage(SingleShaderStage::Compute);
    desc.compute.nextInChain = nullptr;
    desc.compute.module = ToBackend(stage.module.Get())->GetInnerHandle();
    desc.compute.entryPoint = ToOutputStringView(stage.entryPoint);

    std::vector<WGPUConstantEntry> constants;
    std::vector<std::string> keys;
    PopulateWGPUConstants(&constants, &keys, stage.constants);
    desc.compute.constants = constants.data();
    desc.compute.constantCount = constants.size();

    auto device = ToBackend(GetDevice());

    // Call async version of create compute pipeline so that we know the status and will return an
    // internal error to surface up if the inner pipeline is an error.
    struct CreateComputePipelineAsyncUserData {
        WGPUComputePipeline pipeline = nullptr;
        WGPUCreatePipelineAsyncStatus status = WGPUCreatePipelineAsyncStatus_Success;
        std::string message;
    };
    CreateComputePipelineAsyncUserData userdata;

    WGPUCreateComputePipelineAsyncCallbackInfo callbackInfo =
        WGPU_CREATE_COMPUTE_PIPELINE_ASYNC_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = [](WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline,
                               WGPUStringView message, void* userdata1, void* userdata2) {
        auto* data = static_cast<CreateComputePipelineAsyncUserData*>(userdata1);
        data->pipeline = pipeline;
        data->status = status;
        data->message = ToString(message);
    };
    callbackInfo.userdata1 = &userdata;

    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = device->wgpu.deviceCreateComputePipelineAsync(device->GetInnerHandle(), &desc,
                                                                    callbackInfo);
    // Wait time use 5s instead of UINT64_MAX to avoid a failure on dawn-cq-linux-x86 swiftshader.
    device->wgpu.instanceWaitAny(device->GetInnerInstance(), 1, &waitInfo, 5000000000);

    if (userdata.status != WGPUCreatePipelineAsyncStatus_Success) {
        return DAWN_INTERNAL_ERROR(userdata.message);
    }

    mInnerHandle = userdata.pipeline;
    DAWN_ASSERT(mInnerHandle);
    return {};
}

}  // namespace dawn::native::webgpu
