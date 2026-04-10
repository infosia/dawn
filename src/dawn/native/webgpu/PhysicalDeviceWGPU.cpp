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

#include <string>
#include <utility>

#include "dawn/common/StringViewUtils.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/Features_autogen.h"
#include "dawn/native/Instance.h"
#include "dawn/native/Surface.h"
#include "dawn/native/Toggles.h"
#include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"

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

    const bool supportsSubgroupSizeControl = GetFunctions().adapterHasFeature(
        mInnerAdapter, WGPUFeatureName_ChromiumExperimentalSubgroupSizeControl);

    WGPUAdapterPropertiesExplicitComputeSubgroupSizeConfigs explicitComputeSubgroupSizeConfigs = {};
    explicitComputeSubgroupSizeConfigs.chain.sType =
        WGPUSType_AdapterPropertiesExplicitComputeSubgroupSizeConfigs;
    if (supportsSubgroupSizeControl) {
        info.nextInChain = &explicitComputeSubgroupSizeConfigs.chain;
    }

    WGPUStatus status = GetFunctions().adapterGetInfo(mInnerAdapter, &info);
    DAWN_ASSERT(status == WGPUStatus_Success);
    DAWN_ASSERT(info.backendType != WGPUBackendType_WebGPU);

    mVendorName = ToString(info.vendor);
    mArchitectureName = ToString(info.architecture);
    mVendorId = info.vendorID;
    mDeviceId = info.deviceID;
    mName = absl::StrFormat("WebGPU backend on %s %s", FromAPI(info.backendType),
                            ToString(info.device));
    mAdapterType = FromAPI(info.adapterType);
    mDriverDescription = ToString(info.description);
    mSubgroupMinSize = info.subgroupMinSize;
    mSubgroupMaxSize = info.subgroupMaxSize;
    mInnerBackendType = info.backendType;

    if (supportsSubgroupSizeControl) {
        mMinExplicitComputeSubgroupSize =
            explicitComputeSubgroupSizeConfigs.minExplicitComputeSubgroupSize;
        mMaxExplicitComputeSubgroupSize =
            explicitComputeSubgroupSizeConfigs.maxExplicitComputeSubgroupSize;
        mMaxComputeWorkgroupSubgroups =
            explicitComputeSubgroupSizeConfigs.maxComputeWorkgroupSubgroups;
    }

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

Backend* PhysicalDevice::GetBackend() const {
    return mBackend;
}

bool PhysicalDevice::SupportsExternalImages() const {
    // TODO(crbug.com/494307326): remove the function.
    return false;
}

bool PhysicalDevice::SupportsFeatureLevel(wgpu::FeatureLevel featureLevel,
                                          InstanceBase* instance) const {
    return true;
}

ResultOrError<PhysicalDeviceSurfaceCapabilities> PhysicalDevice::GetSurfaceCapabilities(
    InstanceBase* instance,
    const Surface* surface) const {
    // Create a inner surface to query the surface capabilities.
    WGPUSurfaceDescriptor descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
    descriptor.label = ToOutputStringView(surface->GetLabel());

    // Reconstruct the chained descriptor based on the surface type.
    WGPUSurfaceSourceAndroidNativeWindow androidDesc =
        WGPU_SURFACE_SOURCE_ANDROID_NATIVE_WINDOW_INIT;
    WGPUSurfaceSourceMetalLayer metalDesc = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
    WGPUSurfaceSourceWindowsHWND hwndDesc = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
    WGPUSurfaceSourceWaylandSurface waylandDesc = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
    WGPUSurfaceSourceXlibWindow xlibDesc = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
    WGPUSurfaceDescriptorFromWindowsCoreWindow coreWindowDesc =
        WGPU_SURFACE_DESCRIPTOR_FROM_WINDOWS_CORE_WINDOW_INIT;
    WGPUSurfaceDescriptorFromWindowsUWPSwapChainPanel uwpDesc =
        WGPU_SURFACE_DESCRIPTOR_FROM_WINDOWS_UWP_SWAP_CHAIN_PANEL_INIT;
    WGPUSurfaceDescriptorFromWindowsWinUISwapChainPanel winuiDesc =
        WGPU_SURFACE_DESCRIPTOR_FROM_WINDOWS_WINUI_SWAP_CHAIN_PANEL_INIT;

    switch (surface->GetType()) {
        case Surface::Type::AndroidWindow:
            androidDesc.chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;
            androidDesc.window = surface->GetAndroidNativeWindow();
            descriptor.nextInChain = &androidDesc.chain;
            break;
        case Surface::Type::MetalLayer:
            metalDesc.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
            metalDesc.layer = surface->GetMetalLayer();
            descriptor.nextInChain = &metalDesc.chain;
            break;
        case Surface::Type::WindowsHWND:
            hwndDesc.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
            hwndDesc.hinstance = surface->GetHInstance();
            hwndDesc.hwnd = surface->GetHWND();
            descriptor.nextInChain = &hwndDesc.chain;
            break;
        case Surface::Type::WindowsCoreWindow:
            coreWindowDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsCoreWindow;
            coreWindowDesc.coreWindow = surface->GetCoreWindow();
            descriptor.nextInChain = &coreWindowDesc.chain;
            break;
        case Surface::Type::WindowsUWPSwapChainPanel:
            uwpDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsUWPSwapChainPanel;
            uwpDesc.swapChainPanel = surface->GetUWPSwapChainPanel();
            descriptor.nextInChain = &uwpDesc.chain;
            break;
        case Surface::Type::WindowsWinUISwapChainPanel:
            winuiDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsWinUISwapChainPanel;
            winuiDesc.swapChainPanel = surface->GetWinUISwapChainPanel();
            descriptor.nextInChain = &winuiDesc.chain;
            break;
        case Surface::Type::WaylandSurface:
            waylandDesc.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
            waylandDesc.display = surface->GetWaylandDisplay();
            waylandDesc.surface = surface->GetWaylandSurface();
            descriptor.nextInChain = &waylandDesc.chain;
            break;
        case Surface::Type::XlibWindow:
            xlibDesc.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
            xlibDesc.display = surface->GetXDisplay();
            xlibDesc.window = surface->GetXWindow();
            descriptor.nextInChain = &xlibDesc.chain;
            break;
        default:
            DAWN_UNREACHABLE();
            return DAWN_INTERNAL_ERROR("Unknown surface type");
    }

    WGPUSurface innerSurface =
        mBackend->GetFunctions().instanceCreateSurface(mBackend->GetInnerInstance(), &descriptor);
    DAWN_ASSERT(innerSurface);

    WGPUSurfaceCapabilities innerCapabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    WGPUStatus status = mBackend->GetFunctions().surfaceGetCapabilities(innerSurface, mInnerAdapter,
                                                                        &innerCapabilities);

    if (status != WGPUStatus_Success) {
        mBackend->GetFunctions().surfaceRelease(innerSurface);
        return DAWN_INTERNAL_ERROR("Failed to get inner adapter capabilities");
    }

    PhysicalDeviceSurfaceCapabilities capabilities;
    capabilities.usages = static_cast<wgpu::TextureUsage>(innerCapabilities.usages);
    for (size_t i = 0; i < innerCapabilities.formatCount; ++i) {
        capabilities.formats.push_back(FromAPI(innerCapabilities.formats[i]));
    }
    for (size_t i = 0; i < innerCapabilities.presentModeCount; ++i) {
        capabilities.presentModes.push_back(FromAPI(innerCapabilities.presentModes[i]));
    }
    for (size_t i = 0; i < innerCapabilities.alphaModeCount; ++i) {
        capabilities.alphaModes.push_back(FromAPI(innerCapabilities.alphaModes[i]));
    }

    mBackend->GetFunctions().surfaceCapabilitiesFreeMembers(innerCapabilities);
    mBackend->GetFunctions().surfaceRelease(innerSurface);

    return capabilities;
}

MaybeError PhysicalDevice::InitializeImpl() {
    return {};
}

void PhysicalDevice::InitializeSupportedFeaturesImpl() {
    DAWN_ASSERT(kEnumCount<Feature> == static_cast<uint32_t>(kFeatureNameAndInfoList.size()));
    for (uint32_t f = 0; f < kEnumCount<Feature>; f++) {
        Feature feature = static_cast<Feature>(f);
        WGPUFeatureName apiFeature = ToAPI(ToAPI(feature));
        if (GetFunctions().adapterHasFeature(mInnerAdapter, apiFeature)) {
            EnableFeature(feature);
        }
    }
    EnableFeature(Feature::AdapterPropertiesWGPU);
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
                                               TogglesState* deviceToggles) const {
    // We should always use this toggle in order to capture the label.
    deviceToggles->ForceSet(Toggle::UseUserDefinedLabelsInBackend, true);
}

ResultOrError<Ref<DeviceBase>> PhysicalDevice::CreateDeviceImpl(
    AdapterBase* adapter,
    const UnpackedPtr<DeviceDescriptor>& descriptor,
    const TogglesState& deviceToggles,
    Ref<DeviceBase::DeviceLostEvent>&& lostEvent) {
    return Device::Create(adapter, mInnerAdapter, descriptor, deviceToggles, std::move(lostEvent));
}

void PhysicalDevice::PopulateBackendProperties(UnpackedPtr<AdapterInfo>& info,
                                               const TogglesState&) const {
    // TODO(crbug.com/413053623): Populate other AdapterInfo Chained extensions when necessary.
    if (auto* wgpuProperties = info.Get<AdapterPropertiesWGPU>()) {
        wgpuProperties->backendType = FromAPI(mInnerBackendType);
    }
    if (auto* explicitSubgroupSizeConfigs =
            info.Get<AdapterPropertiesExplicitComputeSubgroupSizeConfigs>()) {
        explicitSubgroupSizeConfigs->minExplicitComputeSubgroupSize =
            mMinExplicitComputeSubgroupSize;
        explicitSubgroupSizeConfigs->maxExplicitComputeSubgroupSize =
            mMaxExplicitComputeSubgroupSize;
        explicitSubgroupSizeConfigs->maxComputeWorkgroupSubgroups = mMaxComputeWorkgroupSubgroups;
    }
}

FeatureValidationResult PhysicalDevice::ValidateFeatureSupportedWithTogglesImpl(
    wgpu::FeatureName feature,
    const TogglesState& toggles) const {
    return {};
}

}  // namespace dawn::native::webgpu
