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

#include "dawn/native/webgpu/PhysicalDeviceWGPU.h"

#include <webgpu/webgpu.h>

#include <limits>
#include <string>
#include <unordered_map>
#include <utility>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/BackendConnection.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/Instance.h"
#include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"

namespace {
std::string GetBackendName(WGPUBackendType backendType) {
    switch (backendType) {
        case WGPUBackendType_D3D11:
            return "D3D11";
        case WGPUBackendType_D3D12:
            return "D3D12";
        case WGPUBackendType_Metal:
            return "Metal";
        case WGPUBackendType_Null:
            return "Null";
        case WGPUBackendType_OpenGL:
            return "OpenGL";
        case WGPUBackendType_OpenGLES:
            return "OpenGLES";
        case WGPUBackendType_Vulkan:
            return "Vulkan";
        case WGPUBackendType_WebGPU:
            return "WebGPU";
        case WGPUBackendType_Undefined:
        default:
            DAWN_UNREACHABLE();
    }
}
}  // namespace

namespace dawn::native::webgpu {

Ref<PhysicalDevice> PhysicalDevice::Create(Backend* backend, WGPUAdapter innerAdapter) {
    auto physicalDevice = AcquireRef(new PhysicalDevice(backend, innerAdapter));
    MaybeError err = physicalDevice->Initialize();
    DAWN_CHECK(err.IsSuccess());
    return physicalDevice;
}

PhysicalDevice::PhysicalDevice(Backend* backend, WGPUAdapter innerAdapter)
    : PhysicalDeviceBase(wgpu::BackendType::WebGPU),
      mBackend(backend),
      mInnerAdapter(innerAdapter) {
    WGPUAdapterInfo info = {};
    WGPUStatus status = GetFunctions().adapterGetInfo(mInnerAdapter, &info);
    DAWN_ASSERT(status == WGPUStatus_Success);
    DAWN_ASSERT(info.backendType != WGPUBackendType_WebGPU);

    mVendorName = ToString(info.vendor);
    mArchitectureName = ToString(info.architecture);
    mVendorId = info.vendorID;
    mDeviceId = info.deviceID;
    mName = "WebGPU backend on " + GetBackendName(info.backendType);
    mAdapterType = FromAPI(info.adapterType);
    mDriverDescription = ToString(info.description);
    mSubgroupMinSize = info.subgroupMinSize;
    mSubgroupMaxSize = info.subgroupMaxSize;

    GetFunctions().adapterInfoFreeMembers(info);
}

PhysicalDevice::~PhysicalDevice() {
    if (mInnerAdapter) {
        GetFunctions().adapterRelease(mInnerAdapter);
        mInnerAdapter = nullptr;
    }
}

const DawnProcTable& PhysicalDevice::GetFunctions() const {
    return mBackend->GetFunctions();
}

bool PhysicalDevice::SupportsExternalImages() const {
    return false;
}

bool PhysicalDevice::SupportsFeatureLevel(wgpu::FeatureLevel featureLevel,
                                          InstanceBase* instance) const {
    return true;
}

ResultOrError<PhysicalDeviceSurfaceCapabilities> PhysicalDevice::GetSurfaceCapabilities(
    InstanceBase* instance,
    const Surface* surface) const {
    WGPUInstance innerInstance = mBackend->GetInnerInstance();
    WGPUSurface innerSurface = GetFunctions().instanceCreateSurface(innerInstance, nullptr);

    WGPUSurfaceCapabilities surfaceCapabilities;
    if (GetFunctions().surfaceGetCapabilities(innerSurface, mInnerAdapter, &surfaceCapabilities) !=
        WGPUStatus_Success) {
        return DAWN_INTERNAL_ERROR("Fail to get inner surface capabilities");
    }

    PhysicalDeviceSurfaceCapabilities capabilities = {
        .usages = static_cast<wgpu::TextureUsage>(surfaceCapabilities.usages),
        .formats =
            std::vector(reinterpret_cast<const wgpu::TextureFormat*>(surfaceCapabilities.formats),
                        reinterpret_cast<const wgpu::TextureFormat*>(
                            surfaceCapabilities.formats + surfaceCapabilities.formatCount)),
        .presentModes = std::vector(
            reinterpret_cast<const wgpu::PresentMode*>(surfaceCapabilities.presentModes),
            reinterpret_cast<const wgpu::PresentMode*>(surfaceCapabilities.presentModes +
                                                       surfaceCapabilities.presentModeCount)),
        .alphaModes = std::vector(
            reinterpret_cast<const wgpu::CompositeAlphaMode*>(surfaceCapabilities.alphaModes),
            reinterpret_cast<const wgpu::CompositeAlphaMode*>(surfaceCapabilities.alphaModes +
                                                              surfaceCapabilities.alphaModeCount)),
    };

    GetFunctions().surfaceCapabilitiesFreeMembers(surfaceCapabilities);

    return capabilities;
}

MaybeError PhysicalDevice::InitializeImpl() {
    return {};
}

void PhysicalDevice::InitializeSupportedFeaturesImpl() {
    for (uint32_t i = 0; i < kEnumCount<Feature>; i++) {
        if (GetFunctions().adapterHasFeature(mInnerAdapter, static_cast<WGPUFeatureName>(i))) {
            EnableFeature(static_cast<Feature>(i));
        }
    }
}

MaybeError PhysicalDevice::InitializeSupportedLimitsImpl(CombinedLimits* limits) {
    WGPUStatus status = GetFunctions().adapterGetLimits(mInnerAdapter, ToAPI(&limits->v1));
    if (status != WGPUStatus_Success) {
        return DAWN_INTERNAL_ERROR("Fail to get inner adapter limits");
    }
    return {};
}

void PhysicalDevice::SetupBackendAdapterToggles(dawn::platform::Platform* platform,
                                                TogglesState* adapterToggles) const {}

void PhysicalDevice::SetupBackendDeviceToggles(dawn::platform::Platform* platform,
                                               TogglesState* deviceToggles) const {}

ResultOrError<Ref<DeviceBase>> PhysicalDevice::CreateDeviceImpl(
    AdapterBase* adapter,
    const UnpackedPtr<DeviceDescriptor>& descriptor,
    const TogglesState& deviceToggles,
    Ref<DeviceBase::DeviceLostEvent>&& lostEvent) {
    return Device::Create(adapter, mInnerAdapter, descriptor, deviceToggles, std::move(lostEvent));
}

void PhysicalDevice::PopulateBackendProperties(UnpackedPtr<AdapterInfo>& info) const {
    if (auto* subgroupProperties = info.Get<AdapterPropertiesSubgroups>()) {
        subgroupProperties->subgroupMinSize = mSubgroupMinSize;
        subgroupProperties->subgroupMaxSize = mSubgroupMaxSize;
    }

    // TODO(crbug.com/413053623): Populate other AdapterInfo Chained extensions when necessary.
}

FeatureValidationResult PhysicalDevice::ValidateFeatureSupportedWithTogglesImpl(
    wgpu::FeatureName feature,
    const TogglesState& toggles) const {
    return {};
}

}  // namespace dawn::native::webgpu
