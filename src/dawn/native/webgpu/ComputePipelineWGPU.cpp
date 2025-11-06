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
#include "dawn/native/webgpu/BindGroupLayoutWGPU.h"
#include "dawn/native/webgpu/CaptureContext.h"
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
    : ComputePipelineBase(device, descriptor),
      RecordableObject(schema::ObjectType::ComputePipeline),
      ObjectWGPU(device->wgpu.computePipelineRelease) {}

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

    device->wgpu.devicePushErrorScope(device->GetInnerHandle(), WGPUErrorFilter_Validation);

    mInnerHandle = device->wgpu.deviceCreateComputePipeline(device->GetInnerHandle(), &desc);

    struct ErrorScopeCallbackUserData {
        WGPUPopErrorScopeStatus status = WGPUPopErrorScopeStatus_Success;
        WGPUErrorType type = WGPUErrorType_NoError;
        std::string message;
    } userdata;

    WGPUPopErrorScopeCallbackInfo popErrorscopeCallbackInfo = {
        .nextInChain = nullptr,
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback =
            [](WGPUPopErrorScopeStatus status, WGPUErrorType type, WGPUStringView message,
               void* userdata, void*) {
                auto* data = static_cast<ErrorScopeCallbackUserData*>(userdata);
                data->status = status;
                data->type = type;
                data->message = ToString(message);
            },
        .userdata1 = &userdata,
        .userdata2 = nullptr,
    };

    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future =
        device->wgpu.devicePopErrorScope(device->GetInnerHandle(), popErrorscopeCallbackInfo);
    device->wgpu.instanceWaitAny(device->GetInnerInstance(), 1, &waitInfo,
                                 std::numeric_limits<uint64_t>::max());

    if (userdata.status == WGPUPopErrorScopeStatus_Success) {
        switch (userdata.type) {
            case WGPUErrorType_NoError:
                break;
            case WGPUErrorType_Validation:
                return DAWN_MAKE_ERROR(InternalErrorType::Validation, userdata.message);
            case WGPUErrorType_OutOfMemory:
                return DAWN_OUT_OF_MEMORY_ERROR(userdata.message);
            default:
                return DAWN_INTERNAL_ERROR(userdata.message);
        }
    }
    DAWN_ASSERT(mInnerHandle);
    return {};
}

MaybeError ComputePipeline::AddReferenced(CaptureContext& captureContext) {
    DAWN_TRY(captureContext.AddResource(GetStage(SingleShaderStage::Compute).module.Get()));
    PipelineLayoutBase* pipelineLayout = GetLayout();
    if (!pipelineLayout->IsImplicit()) {
        DAWN_TRY(captureContext.AddResource(pipelineLayout));
    }
    return {};
}

MaybeError ComputePipeline::CaptureCreationParameters(CaptureContext& captureContext) {
    schema::ObjectId layoutId = 0;
    std::vector<schema::BindGroupLayoutIndexIdPair> groupIndexIds;

    PipelineLayoutBase* pipelineLayout = GetLayout();
    if (pipelineLayout->IsImplicit()) {
        // If it's an implicit layout then, on playback, we need to add the bind group layouts
        // to the id to resource map as there is no other connection.
        for (BindGroupIndex groupIndex : pipelineLayout->GetBindGroupLayoutsMask()) {
            BindGroupLayoutBase* bgl = pipelineLayout->GetFrontendBindGroupLayout(groupIndex);

            groupIndexIds.push_back(schema::BindGroupLayoutIndexIdPair{{
                .groupIndex = uint32_t(groupIndex),
                .bindGroupLayoutId = captureContext.AddAndGetIdForImplicitResource(bgl),
            }});
        }
    } else {
        layoutId = captureContext.GetId(pipelineLayout);
    }

    auto& stage = GetStage(SingleShaderStage::Compute);
    schema::ComputePipeline data{{
        .layoutId = layoutId,
        .compute = ToSchema(captureContext, stage),
        .groupIndexIds = groupIndexIds,
    }};
    Serialize(captureContext, data);
    return {};
}

}  // namespace dawn::native::webgpu
