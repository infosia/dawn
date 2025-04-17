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

// WebGPUBackend.cpp: contains the definition of symbols exported by WebGPUBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

#include "dawn/native/WebGPUBackend.h"

#include "dawn/dawn_proc.h"
#include "dawn/dawn_proc_table.h"

#include "dawn/native/webgpu/DeviceWGPU.h"

namespace dawn::native::webgpu {

// void Set{{Prefix}}ProcTableFromDynamicLib(const dawn::DynamicLib& lib, {{Prefix}}ProcTable*
// table) {
//     {% for function in by_category["function"] %}
//         lib.GetProc(&table->{{as_varName(function.name)}}, "{{ as_cMethod(None, function.name)
//         }}");
//     {% endfor %}

//     {% for type in by_category["object"] %}
//         {% for method in c_methods(type) %}
//             lib.GetProc(&table->{{as_varName(type.name, method.name)}}, "{{ as_cMethod(type.name,
//             method.name) }}");
//         {% endfor %}
//     {% endfor %}
// }

void SetDawnProcTableFromDynamicLib(const dawn::DynamicLib& lib, DawnProcTable* table) {
    lib.GetProc(&table->adapterInfoFreeMembers, "wgpuAdapterInfoFreeMembers");
    lib.GetProc(&table->adapterPropertiesMemoryHeapsFreeMembers,
                "wgpuAdapterPropertiesMemoryHeapsFreeMembers");
    lib.GetProc(&table->adapterPropertiesSubgroupMatrixConfigsFreeMembers,
                "wgpuAdapterPropertiesSubgroupMatrixConfigsFreeMembers");
    lib.GetProc(&table->createInstance, "wgpuCreateInstance");
    lib.GetProc(&table->dawnDrmFormatCapabilitiesFreeMembers,
                "wgpuDawnDrmFormatCapabilitiesFreeMembers");
    lib.GetProc(&table->getInstanceCapabilities, "wgpuGetInstanceCapabilities");
    lib.GetProc(&table->getProcAddress, "wgpuGetProcAddress");
    lib.GetProc(&table->sharedBufferMemoryEndAccessStateFreeMembers,
                "wgpuSharedBufferMemoryEndAccessStateFreeMembers");
    lib.GetProc(&table->sharedTextureMemoryEndAccessStateFreeMembers,
                "wgpuSharedTextureMemoryEndAccessStateFreeMembers");
    lib.GetProc(&table->supportedWGSLLanguageFeaturesFreeMembers,
                "wgpuSupportedWGSLLanguageFeaturesFreeMembers");
    lib.GetProc(&table->supportedFeaturesFreeMembers, "wgpuSupportedFeaturesFreeMembers");
    lib.GetProc(&table->surfaceCapabilitiesFreeMembers, "wgpuSurfaceCapabilitiesFreeMembers");

    lib.GetProc(&table->adapterCreateDevice, "wgpuAdapterCreateDevice");
    lib.GetProc(&table->adapterGetFeatures, "wgpuAdapterGetFeatures");
    lib.GetProc(&table->adapterGetFormatCapabilities, "wgpuAdapterGetFormatCapabilities");
    lib.GetProc(&table->adapterGetInfo, "wgpuAdapterGetInfo");
    lib.GetProc(&table->adapterGetInstance, "wgpuAdapterGetInstance");
    lib.GetProc(&table->adapterGetLimits, "wgpuAdapterGetLimits");
    lib.GetProc(&table->adapterHasFeature, "wgpuAdapterHasFeature");
    lib.GetProc(&table->adapterRequestDevice, "wgpuAdapterRequestDevice");
    lib.GetProc(&table->adapterAddRef, "wgpuAdapterAddRef");
    lib.GetProc(&table->adapterRelease, "wgpuAdapterRelease");
    lib.GetProc(&table->bindGroupSetLabel, "wgpuBindGroupSetLabel");
    lib.GetProc(&table->bindGroupAddRef, "wgpuBindGroupAddRef");
    lib.GetProc(&table->bindGroupRelease, "wgpuBindGroupRelease");
    lib.GetProc(&table->bindGroupLayoutSetLabel, "wgpuBindGroupLayoutSetLabel");
    lib.GetProc(&table->bindGroupLayoutAddRef, "wgpuBindGroupLayoutAddRef");
    lib.GetProc(&table->bindGroupLayoutRelease, "wgpuBindGroupLayoutRelease");
    lib.GetProc(&table->bufferDestroy, "wgpuBufferDestroy");
    lib.GetProc(&table->bufferGetConstMappedRange, "wgpuBufferGetConstMappedRange");
    lib.GetProc(&table->bufferGetMapState, "wgpuBufferGetMapState");
    lib.GetProc(&table->bufferGetMappedRange, "wgpuBufferGetMappedRange");
    lib.GetProc(&table->bufferGetSize, "wgpuBufferGetSize");
    lib.GetProc(&table->bufferGetUsage, "wgpuBufferGetUsage");
    lib.GetProc(&table->bufferMapAsync, "wgpuBufferMapAsync");
    lib.GetProc(&table->bufferReadMappedRange, "wgpuBufferReadMappedRange");
    lib.GetProc(&table->bufferSetLabel, "wgpuBufferSetLabel");
    lib.GetProc(&table->bufferUnmap, "wgpuBufferUnmap");
    lib.GetProc(&table->bufferWriteMappedRange, "wgpuBufferWriteMappedRange");
    lib.GetProc(&table->bufferAddRef, "wgpuBufferAddRef");
    lib.GetProc(&table->bufferRelease, "wgpuBufferRelease");
    lib.GetProc(&table->commandBufferSetLabel, "wgpuCommandBufferSetLabel");
    lib.GetProc(&table->commandBufferAddRef, "wgpuCommandBufferAddRef");
    lib.GetProc(&table->commandBufferRelease, "wgpuCommandBufferRelease");
    lib.GetProc(&table->commandEncoderBeginComputePass, "wgpuCommandEncoderBeginComputePass");
    lib.GetProc(&table->commandEncoderBeginRenderPass, "wgpuCommandEncoderBeginRenderPass");
    lib.GetProc(&table->commandEncoderClearBuffer, "wgpuCommandEncoderClearBuffer");
    lib.GetProc(&table->commandEncoderCopyBufferToBuffer, "wgpuCommandEncoderCopyBufferToBuffer");
    lib.GetProc(&table->commandEncoderCopyBufferToTexture, "wgpuCommandEncoderCopyBufferToTexture");
    lib.GetProc(&table->commandEncoderCopyTextureToBuffer, "wgpuCommandEncoderCopyTextureToBuffer");
    lib.GetProc(&table->commandEncoderCopyTextureToTexture,
                "wgpuCommandEncoderCopyTextureToTexture");
    lib.GetProc(&table->commandEncoderFinish, "wgpuCommandEncoderFinish");
    lib.GetProc(&table->commandEncoderInjectValidationError,
                "wgpuCommandEncoderInjectValidationError");
    lib.GetProc(&table->commandEncoderInsertDebugMarker, "wgpuCommandEncoderInsertDebugMarker");
    lib.GetProc(&table->commandEncoderPopDebugGroup, "wgpuCommandEncoderPopDebugGroup");
    lib.GetProc(&table->commandEncoderPushDebugGroup, "wgpuCommandEncoderPushDebugGroup");
    lib.GetProc(&table->commandEncoderResolveQuerySet, "wgpuCommandEncoderResolveQuerySet");
    lib.GetProc(&table->commandEncoderSetLabel, "wgpuCommandEncoderSetLabel");
    lib.GetProc(&table->commandEncoderWriteBuffer, "wgpuCommandEncoderWriteBuffer");
    lib.GetProc(&table->commandEncoderWriteTimestamp, "wgpuCommandEncoderWriteTimestamp");
    lib.GetProc(&table->commandEncoderAddRef, "wgpuCommandEncoderAddRef");
    lib.GetProc(&table->commandEncoderRelease, "wgpuCommandEncoderRelease");
    lib.GetProc(&table->computePassEncoderDispatchWorkgroups,
                "wgpuComputePassEncoderDispatchWorkgroups");
    lib.GetProc(&table->computePassEncoderDispatchWorkgroupsIndirect,
                "wgpuComputePassEncoderDispatchWorkgroupsIndirect");
    lib.GetProc(&table->computePassEncoderEnd, "wgpuComputePassEncoderEnd");
    lib.GetProc(&table->computePassEncoderInsertDebugMarker,
                "wgpuComputePassEncoderInsertDebugMarker");
    lib.GetProc(&table->computePassEncoderPopDebugGroup, "wgpuComputePassEncoderPopDebugGroup");
    lib.GetProc(&table->computePassEncoderPushDebugGroup, "wgpuComputePassEncoderPushDebugGroup");
    lib.GetProc(&table->computePassEncoderSetBindGroup, "wgpuComputePassEncoderSetBindGroup");
    lib.GetProc(&table->computePassEncoderSetImmediateData,
                "wgpuComputePassEncoderSetImmediateData");
    lib.GetProc(&table->computePassEncoderSetLabel, "wgpuComputePassEncoderSetLabel");
    lib.GetProc(&table->computePassEncoderSetPipeline, "wgpuComputePassEncoderSetPipeline");
    lib.GetProc(&table->computePassEncoderWriteTimestamp, "wgpuComputePassEncoderWriteTimestamp");
    lib.GetProc(&table->computePassEncoderAddRef, "wgpuComputePassEncoderAddRef");
    lib.GetProc(&table->computePassEncoderRelease, "wgpuComputePassEncoderRelease");
    lib.GetProc(&table->computePipelineGetBindGroupLayout, "wgpuComputePipelineGetBindGroupLayout");
    lib.GetProc(&table->computePipelineSetLabel, "wgpuComputePipelineSetLabel");
    lib.GetProc(&table->computePipelineAddRef, "wgpuComputePipelineAddRef");
    lib.GetProc(&table->computePipelineRelease, "wgpuComputePipelineRelease");
    lib.GetProc(&table->deviceCreateBindGroup, "wgpuDeviceCreateBindGroup");
    lib.GetProc(&table->deviceCreateBindGroupLayout, "wgpuDeviceCreateBindGroupLayout");
    lib.GetProc(&table->deviceCreateBuffer, "wgpuDeviceCreateBuffer");
    lib.GetProc(&table->deviceCreateCommandEncoder, "wgpuDeviceCreateCommandEncoder");
    lib.GetProc(&table->deviceCreateComputePipeline, "wgpuDeviceCreateComputePipeline");
    lib.GetProc(&table->deviceCreateComputePipelineAsync, "wgpuDeviceCreateComputePipelineAsync");
    lib.GetProc(&table->deviceCreateErrorBuffer, "wgpuDeviceCreateErrorBuffer");
    lib.GetProc(&table->deviceCreateErrorExternalTexture, "wgpuDeviceCreateErrorExternalTexture");
    lib.GetProc(&table->deviceCreateErrorShaderModule, "wgpuDeviceCreateErrorShaderModule");
    lib.GetProc(&table->deviceCreateErrorTexture, "wgpuDeviceCreateErrorTexture");
    lib.GetProc(&table->deviceCreateExternalTexture, "wgpuDeviceCreateExternalTexture");
    lib.GetProc(&table->deviceCreatePipelineLayout, "wgpuDeviceCreatePipelineLayout");
    lib.GetProc(&table->deviceCreateQuerySet, "wgpuDeviceCreateQuerySet");
    lib.GetProc(&table->deviceCreateRenderBundleEncoder, "wgpuDeviceCreateRenderBundleEncoder");
    lib.GetProc(&table->deviceCreateRenderPipeline, "wgpuDeviceCreateRenderPipeline");
    lib.GetProc(&table->deviceCreateRenderPipelineAsync, "wgpuDeviceCreateRenderPipelineAsync");
    lib.GetProc(&table->deviceCreateSampler, "wgpuDeviceCreateSampler");
    lib.GetProc(&table->deviceCreateShaderModule, "wgpuDeviceCreateShaderModule");
    lib.GetProc(&table->deviceCreateTexture, "wgpuDeviceCreateTexture");
    lib.GetProc(&table->deviceDestroy, "wgpuDeviceDestroy");
    lib.GetProc(&table->deviceForceLoss, "wgpuDeviceForceLoss");
    lib.GetProc(&table->deviceGetAHardwareBufferProperties,
                "wgpuDeviceGetAHardwareBufferProperties");
    lib.GetProc(&table->deviceGetAdapter, "wgpuDeviceGetAdapter");
    lib.GetProc(&table->deviceGetAdapterInfo, "wgpuDeviceGetAdapterInfo");
    lib.GetProc(&table->deviceGetFeatures, "wgpuDeviceGetFeatures");
    lib.GetProc(&table->deviceGetLimits, "wgpuDeviceGetLimits");
    lib.GetProc(&table->deviceGetLostFuture, "wgpuDeviceGetLostFuture");
    lib.GetProc(&table->deviceGetQueue, "wgpuDeviceGetQueue");
    lib.GetProc(&table->deviceHasFeature, "wgpuDeviceHasFeature");
    lib.GetProc(&table->deviceImportSharedBufferMemory, "wgpuDeviceImportSharedBufferMemory");
    lib.GetProc(&table->deviceImportSharedFence, "wgpuDeviceImportSharedFence");
    lib.GetProc(&table->deviceImportSharedTextureMemory, "wgpuDeviceImportSharedTextureMemory");
    lib.GetProc(&table->deviceInjectError, "wgpuDeviceInjectError");
    lib.GetProc(&table->devicePopErrorScope, "wgpuDevicePopErrorScope");
    lib.GetProc(&table->devicePushErrorScope, "wgpuDevicePushErrorScope");
    lib.GetProc(&table->deviceSetLabel, "wgpuDeviceSetLabel");
    lib.GetProc(&table->deviceSetLoggingCallback, "wgpuDeviceSetLoggingCallback");
    lib.GetProc(&table->deviceTick, "wgpuDeviceTick");
    lib.GetProc(&table->deviceValidateTextureDescriptor, "wgpuDeviceValidateTextureDescriptor");
    lib.GetProc(&table->deviceAddRef, "wgpuDeviceAddRef");
    lib.GetProc(&table->deviceRelease, "wgpuDeviceRelease");
    lib.GetProc(&table->externalTextureDestroy, "wgpuExternalTextureDestroy");
    lib.GetProc(&table->externalTextureExpire, "wgpuExternalTextureExpire");
    lib.GetProc(&table->externalTextureRefresh, "wgpuExternalTextureRefresh");
    lib.GetProc(&table->externalTextureSetLabel, "wgpuExternalTextureSetLabel");
    lib.GetProc(&table->externalTextureAddRef, "wgpuExternalTextureAddRef");
    lib.GetProc(&table->externalTextureRelease, "wgpuExternalTextureRelease");
    lib.GetProc(&table->instanceCreateSurface, "wgpuInstanceCreateSurface");
    lib.GetProc(&table->instanceGetWGSLLanguageFeatures, "wgpuInstanceGetWGSLLanguageFeatures");
    lib.GetProc(&table->instanceHasWGSLLanguageFeature, "wgpuInstanceHasWGSLLanguageFeature");
    lib.GetProc(&table->instanceProcessEvents, "wgpuInstanceProcessEvents");
    lib.GetProc(&table->instanceRequestAdapter, "wgpuInstanceRequestAdapter");
    lib.GetProc(&table->instanceWaitAny, "wgpuInstanceWaitAny");
    lib.GetProc(&table->instanceAddRef, "wgpuInstanceAddRef");
    lib.GetProc(&table->instanceRelease, "wgpuInstanceRelease");
    lib.GetProc(&table->pipelineLayoutSetLabel, "wgpuPipelineLayoutSetLabel");
    lib.GetProc(&table->pipelineLayoutAddRef, "wgpuPipelineLayoutAddRef");
    lib.GetProc(&table->pipelineLayoutRelease, "wgpuPipelineLayoutRelease");
    lib.GetProc(&table->querySetDestroy, "wgpuQuerySetDestroy");
    lib.GetProc(&table->querySetGetCount, "wgpuQuerySetGetCount");
    lib.GetProc(&table->querySetGetType, "wgpuQuerySetGetType");
    lib.GetProc(&table->querySetSetLabel, "wgpuQuerySetSetLabel");
    lib.GetProc(&table->querySetAddRef, "wgpuQuerySetAddRef");
    lib.GetProc(&table->querySetRelease, "wgpuQuerySetRelease");
    lib.GetProc(&table->queueCopyExternalTextureForBrowser,
                "wgpuQueueCopyExternalTextureForBrowser");
    lib.GetProc(&table->queueCopyTextureForBrowser, "wgpuQueueCopyTextureForBrowser");
    lib.GetProc(&table->queueOnSubmittedWorkDone, "wgpuQueueOnSubmittedWorkDone");
    lib.GetProc(&table->queueSetLabel, "wgpuQueueSetLabel");
    lib.GetProc(&table->queueSubmit, "wgpuQueueSubmit");
    lib.GetProc(&table->queueWriteBuffer, "wgpuQueueWriteBuffer");
    lib.GetProc(&table->queueWriteTexture, "wgpuQueueWriteTexture");
    lib.GetProc(&table->queueAddRef, "wgpuQueueAddRef");
    lib.GetProc(&table->queueRelease, "wgpuQueueRelease");
    lib.GetProc(&table->renderBundleSetLabel, "wgpuRenderBundleSetLabel");
    lib.GetProc(&table->renderBundleAddRef, "wgpuRenderBundleAddRef");
    lib.GetProc(&table->renderBundleRelease, "wgpuRenderBundleRelease");
    lib.GetProc(&table->renderBundleEncoderDraw, "wgpuRenderBundleEncoderDraw");
    lib.GetProc(&table->renderBundleEncoderDrawIndexed, "wgpuRenderBundleEncoderDrawIndexed");
    lib.GetProc(&table->renderBundleEncoderDrawIndexedIndirect,
                "wgpuRenderBundleEncoderDrawIndexedIndirect");
    lib.GetProc(&table->renderBundleEncoderDrawIndirect, "wgpuRenderBundleEncoderDrawIndirect");
    lib.GetProc(&table->renderBundleEncoderFinish, "wgpuRenderBundleEncoderFinish");
    lib.GetProc(&table->renderBundleEncoderInsertDebugMarker,
                "wgpuRenderBundleEncoderInsertDebugMarker");
    lib.GetProc(&table->renderBundleEncoderPopDebugGroup, "wgpuRenderBundleEncoderPopDebugGroup");
    lib.GetProc(&table->renderBundleEncoderPushDebugGroup, "wgpuRenderBundleEncoderPushDebugGroup");
    lib.GetProc(&table->renderBundleEncoderSetBindGroup, "wgpuRenderBundleEncoderSetBindGroup");
    lib.GetProc(&table->renderBundleEncoderSetImmediateData,
                "wgpuRenderBundleEncoderSetImmediateData");
    lib.GetProc(&table->renderBundleEncoderSetIndexBuffer, "wgpuRenderBundleEncoderSetIndexBuffer");
    lib.GetProc(&table->renderBundleEncoderSetLabel, "wgpuRenderBundleEncoderSetLabel");
    lib.GetProc(&table->renderBundleEncoderSetPipeline, "wgpuRenderBundleEncoderSetPipeline");
    lib.GetProc(&table->renderBundleEncoderSetVertexBuffer,
                "wgpuRenderBundleEncoderSetVertexBuffer");
    lib.GetProc(&table->renderBundleEncoderAddRef, "wgpuRenderBundleEncoderAddRef");
    lib.GetProc(&table->renderBundleEncoderRelease, "wgpuRenderBundleEncoderRelease");
    lib.GetProc(&table->renderPassEncoderBeginOcclusionQuery,
                "wgpuRenderPassEncoderBeginOcclusionQuery");
    lib.GetProc(&table->renderPassEncoderDraw, "wgpuRenderPassEncoderDraw");
    lib.GetProc(&table->renderPassEncoderDrawIndexed, "wgpuRenderPassEncoderDrawIndexed");
    lib.GetProc(&table->renderPassEncoderDrawIndexedIndirect,
                "wgpuRenderPassEncoderDrawIndexedIndirect");
    lib.GetProc(&table->renderPassEncoderDrawIndirect, "wgpuRenderPassEncoderDrawIndirect");
    lib.GetProc(&table->renderPassEncoderEnd, "wgpuRenderPassEncoderEnd");
    lib.GetProc(&table->renderPassEncoderEndOcclusionQuery,
                "wgpuRenderPassEncoderEndOcclusionQuery");
    lib.GetProc(&table->renderPassEncoderExecuteBundles, "wgpuRenderPassEncoderExecuteBundles");
    lib.GetProc(&table->renderPassEncoderInsertDebugMarker,
                "wgpuRenderPassEncoderInsertDebugMarker");
    lib.GetProc(&table->renderPassEncoderMultiDrawIndexedIndirect,
                "wgpuRenderPassEncoderMultiDrawIndexedIndirect");
    lib.GetProc(&table->renderPassEncoderMultiDrawIndirect,
                "wgpuRenderPassEncoderMultiDrawIndirect");
    lib.GetProc(&table->renderPassEncoderPixelLocalStorageBarrier,
                "wgpuRenderPassEncoderPixelLocalStorageBarrier");
    lib.GetProc(&table->renderPassEncoderPopDebugGroup, "wgpuRenderPassEncoderPopDebugGroup");
    lib.GetProc(&table->renderPassEncoderPushDebugGroup, "wgpuRenderPassEncoderPushDebugGroup");
    lib.GetProc(&table->renderPassEncoderSetBindGroup, "wgpuRenderPassEncoderSetBindGroup");
    lib.GetProc(&table->renderPassEncoderSetBlendConstant, "wgpuRenderPassEncoderSetBlendConstant");
    lib.GetProc(&table->renderPassEncoderSetImmediateData, "wgpuRenderPassEncoderSetImmediateData");
    lib.GetProc(&table->renderPassEncoderSetIndexBuffer, "wgpuRenderPassEncoderSetIndexBuffer");
    lib.GetProc(&table->renderPassEncoderSetLabel, "wgpuRenderPassEncoderSetLabel");
    lib.GetProc(&table->renderPassEncoderSetPipeline, "wgpuRenderPassEncoderSetPipeline");
    lib.GetProc(&table->renderPassEncoderSetScissorRect, "wgpuRenderPassEncoderSetScissorRect");
    lib.GetProc(&table->renderPassEncoderSetStencilReference,
                "wgpuRenderPassEncoderSetStencilReference");
    lib.GetProc(&table->renderPassEncoderSetVertexBuffer, "wgpuRenderPassEncoderSetVertexBuffer");
    lib.GetProc(&table->renderPassEncoderSetViewport, "wgpuRenderPassEncoderSetViewport");
    lib.GetProc(&table->renderPassEncoderWriteTimestamp, "wgpuRenderPassEncoderWriteTimestamp");
    lib.GetProc(&table->renderPassEncoderAddRef, "wgpuRenderPassEncoderAddRef");
    lib.GetProc(&table->renderPassEncoderRelease, "wgpuRenderPassEncoderRelease");
    lib.GetProc(&table->renderPipelineGetBindGroupLayout, "wgpuRenderPipelineGetBindGroupLayout");
    lib.GetProc(&table->renderPipelineSetLabel, "wgpuRenderPipelineSetLabel");
    lib.GetProc(&table->renderPipelineAddRef, "wgpuRenderPipelineAddRef");
    lib.GetProc(&table->renderPipelineRelease, "wgpuRenderPipelineRelease");
    lib.GetProc(&table->samplerSetLabel, "wgpuSamplerSetLabel");
    lib.GetProc(&table->samplerAddRef, "wgpuSamplerAddRef");
    lib.GetProc(&table->samplerRelease, "wgpuSamplerRelease");
    lib.GetProc(&table->shaderModuleGetCompilationInfo, "wgpuShaderModuleGetCompilationInfo");
    lib.GetProc(&table->shaderModuleSetLabel, "wgpuShaderModuleSetLabel");
    lib.GetProc(&table->shaderModuleAddRef, "wgpuShaderModuleAddRef");
    lib.GetProc(&table->shaderModuleRelease, "wgpuShaderModuleRelease");
    lib.GetProc(&table->sharedBufferMemoryBeginAccess, "wgpuSharedBufferMemoryBeginAccess");
    lib.GetProc(&table->sharedBufferMemoryCreateBuffer, "wgpuSharedBufferMemoryCreateBuffer");
    lib.GetProc(&table->sharedBufferMemoryEndAccess, "wgpuSharedBufferMemoryEndAccess");
    lib.GetProc(&table->sharedBufferMemoryGetProperties, "wgpuSharedBufferMemoryGetProperties");
    lib.GetProc(&table->sharedBufferMemoryIsDeviceLost, "wgpuSharedBufferMemoryIsDeviceLost");
    lib.GetProc(&table->sharedBufferMemorySetLabel, "wgpuSharedBufferMemorySetLabel");
    lib.GetProc(&table->sharedBufferMemoryAddRef, "wgpuSharedBufferMemoryAddRef");
    lib.GetProc(&table->sharedBufferMemoryRelease, "wgpuSharedBufferMemoryRelease");
    lib.GetProc(&table->sharedFenceExportInfo, "wgpuSharedFenceExportInfo");
    lib.GetProc(&table->sharedFenceAddRef, "wgpuSharedFenceAddRef");
    lib.GetProc(&table->sharedFenceRelease, "wgpuSharedFenceRelease");
    lib.GetProc(&table->sharedTextureMemoryBeginAccess, "wgpuSharedTextureMemoryBeginAccess");
    lib.GetProc(&table->sharedTextureMemoryCreateTexture, "wgpuSharedTextureMemoryCreateTexture");
    lib.GetProc(&table->sharedTextureMemoryEndAccess, "wgpuSharedTextureMemoryEndAccess");
    lib.GetProc(&table->sharedTextureMemoryGetProperties, "wgpuSharedTextureMemoryGetProperties");
    lib.GetProc(&table->sharedTextureMemoryIsDeviceLost, "wgpuSharedTextureMemoryIsDeviceLost");
    lib.GetProc(&table->sharedTextureMemorySetLabel, "wgpuSharedTextureMemorySetLabel");
    lib.GetProc(&table->sharedTextureMemoryAddRef, "wgpuSharedTextureMemoryAddRef");
    lib.GetProc(&table->sharedTextureMemoryRelease, "wgpuSharedTextureMemoryRelease");
    lib.GetProc(&table->surfaceConfigure, "wgpuSurfaceConfigure");
    lib.GetProc(&table->surfaceGetCapabilities, "wgpuSurfaceGetCapabilities");
    lib.GetProc(&table->surfaceGetCurrentTexture, "wgpuSurfaceGetCurrentTexture");
    lib.GetProc(&table->surfacePresent, "wgpuSurfacePresent");
    lib.GetProc(&table->surfaceSetLabel, "wgpuSurfaceSetLabel");
    lib.GetProc(&table->surfaceUnconfigure, "wgpuSurfaceUnconfigure");
    lib.GetProc(&table->surfaceAddRef, "wgpuSurfaceAddRef");
    lib.GetProc(&table->surfaceRelease, "wgpuSurfaceRelease");
    lib.GetProc(&table->textureCreateErrorView, "wgpuTextureCreateErrorView");
    lib.GetProc(&table->textureCreateView, "wgpuTextureCreateView");
    lib.GetProc(&table->textureDestroy, "wgpuTextureDestroy");
    lib.GetProc(&table->textureGetDepthOrArrayLayers, "wgpuTextureGetDepthOrArrayLayers");
    lib.GetProc(&table->textureGetDimension, "wgpuTextureGetDimension");
    lib.GetProc(&table->textureGetFormat, "wgpuTextureGetFormat");
    lib.GetProc(&table->textureGetHeight, "wgpuTextureGetHeight");
    lib.GetProc(&table->textureGetMipLevelCount, "wgpuTextureGetMipLevelCount");
    lib.GetProc(&table->textureGetSampleCount, "wgpuTextureGetSampleCount");
    lib.GetProc(&table->textureGetUsage, "wgpuTextureGetUsage");
    lib.GetProc(&table->textureGetWidth, "wgpuTextureGetWidth");
    lib.GetProc(&table->textureSetLabel, "wgpuTextureSetLabel");
    lib.GetProc(&table->textureAddRef, "wgpuTextureAddRef");
    lib.GetProc(&table->textureRelease, "wgpuTextureRelease");
    lib.GetProc(&table->textureViewSetLabel, "wgpuTextureViewSetLabel");
    lib.GetProc(&table->textureViewAddRef, "wgpuTextureViewAddRef");
    lib.GetProc(&table->textureViewRelease, "wgpuTextureViewRelease");
}

}  // namespace dawn::native::webgpu
