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

#include "dawn/native/BackendConnection.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/ErrorData.h"
#include "dawn/native/Instance.h"
#include "dawn/native/webgpu/BackendWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"

namespace dawn::native::webgpu {

// Implementation of pre-Device objects: the webgpu physical device, webgpu backend connection and
// Connect()

Ref<PhysicalDevice> PhysicalDevice::Create(Backend* backend,
                                           const UnpackedPtr<RequestAdapterOptions>& options) {
    auto physicalDevice = AcquireRef(new PhysicalDevice(backend, options));
    MaybeError err = physicalDevice->Initialize();
    DAWN_CHECK(err.IsSuccess());
    return physicalDevice;
}

PhysicalDevice::PhysicalDevice(Backend* backend, const UnpackedPtr<RequestAdapterOptions>& options)
    : PhysicalDeviceBase(wgpu::BackendType::WebGPU), mBackend(backend) {
    mVendorId = 0;
    mDeviceId = 0;
    mName = "WebGPU backend";
    mAdapterType = wgpu::AdapterType::CPU;

    mRequestAdapterOptions = *(*options);
    // For the implementation backend, don't select WebGPU otherwise it creates an infinite loop.
    mRequestAdapterOptions.backendType = wgpu::BackendType::Undefined;
    // TODO(crbug.com/413053623): copy chained extensions.
    mRequestAdapterOptions.nextInChain = nullptr;
}

PhysicalDevice::~PhysicalDevice() = default;

const DawnProcTable& PhysicalDevice::GetFunctions() const {
    return mBackend->GetFunctions();
}

// const RequestAdapterOptions* PhysicalDevice::GetRequestAdapterOptions() const {
//     return &mRequestAdapterOptions;
// }

// AdapterBase* PhysicalDevice::GetAdapterImpl() const {
//     return mAdapterImpl.Get();
// }

bool PhysicalDevice::SupportsExternalImages() const {
    // TODO(crbug.com/413053623): query the implementation adapter.
    return false;
}

bool PhysicalDevice::SupportsFeatureLevel(wgpu::FeatureLevel, InstanceBase*) const {
    // TODO(crbug.com/413053623): query the implementation adapter.
    return true;
}

ResultOrError<PhysicalDeviceSurfaceCapabilities> PhysicalDevice::GetSurfaceCapabilities(
    InstanceBase* instance,
    const Surface* surface) const {
    PhysicalDeviceSurfaceCapabilities capabilities;
    capabilities.usages = wgpu::TextureUsage::RenderAttachment;
    capabilities.formats = {wgpu::TextureFormat::BGRA8Unorm};
    capabilities.presentModes = {wgpu::PresentMode::Fifo};
    capabilities.alphaModes = {wgpu::CompositeAlphaMode::Opaque};
    return capabilities;
}

MaybeError PhysicalDevice::InitializeImpl() {
    return {};
}

void PhysicalDevice::InitializeSupportedFeaturesImpl() {
    // Enable all features by default for the convenience of tests.
    for (uint32_t i = 0; i < kEnumCount<Feature>; i++) {
        EnableFeature(static_cast<Feature>(i));
    }
}

MaybeError PhysicalDevice::InitializeSupportedLimitsImpl(CombinedLimits* limits) {
    GetDefaultLimitsForSupportedFeatureLevel(limits);
    // Set the subgroups limit, as DeviceWGPU should support subgroups feature.
    limits->experimentalImmediateDataLimits.maxImmediateDataRangeByteSize =
        kMaxExternalImmediateConstantsPerPipeline * kImmediateConstantElementByteSize;
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
    if (!mAdapterImpl.Get()) {
        auto adapters = adapter->GetInstance()->EnumerateAdapters(&mRequestAdapterOptions);
        // The adapters vector is sorted by SortAdapters in Adapter.cpp
        // in the order of Core -> Compat -> Testing while preferring OS-specific backends.
        mAdapterImpl = std::move(adapters[0]);
        DAWN_ASSERT(mAdapterImpl->GetPhysicalDevice()->GetBackendType() !=
                    wgpu::BackendType::WebGPU);
    }

    return Device::Create(adapter, mAdapterImpl.Get(), descriptor, deviceToggles,
                          std::move(lostEvent));
}

void PhysicalDevice::PopulateBackendProperties(UnpackedPtr<AdapterInfo>& info) const {
    if (auto* subgroupProperties = info.Get<AdapterPropertiesSubgroups>()) {
        subgroupProperties->subgroupMinSize = 4;
        subgroupProperties->subgroupMaxSize = 128;
    }
    if (auto* memoryHeapProperties = info.Get<AdapterPropertiesMemoryHeaps>()) {
        auto* heapInfo = new MemoryHeapInfo[1];
        memoryHeapProperties->heapCount = 1;
        memoryHeapProperties->heapInfo = heapInfo;

        heapInfo[0].size = 1024 * 1024 * 1024;
        heapInfo[0].properties = wgpu::HeapProperty::DeviceLocal | wgpu::HeapProperty::HostVisible |
                                 wgpu::HeapProperty::HostCached;
    }
    if (auto* d3dProperties = info.Get<AdapterPropertiesD3D>()) {
        d3dProperties->shaderModel = 0;
    }
}

FeatureValidationResult PhysicalDevice::ValidateFeatureSupportedWithTogglesImpl(
    wgpu::FeatureName feature,
    const TogglesState& toggles) const {
    // TODO(crbug.com/413053623): query the implementation adapter.
    return {};
}

}  // namespace dawn::native::webgpu
