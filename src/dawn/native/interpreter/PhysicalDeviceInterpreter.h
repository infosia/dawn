// Copyright 2023 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_NATIVE_INTERPRETER_PHYSICALDEVICEINTERPRETER_H_
#define SRC_DAWN_NATIVE_INTERPRETER_PHYSICALDEVICEINTERPRETER_H_

#include "dawn/native/PhysicalDevice.h"

namespace dawn::native::interpreter {

class PhysicalDevice : public PhysicalDeviceBase {
  public:
    explicit PhysicalDevice(InstanceBase* instance);
    ~PhysicalDevice() override;

    bool SupportsExternalImages() const override;
    bool SupportsFeatureLevel(wgpu::FeatureLevel featureLevel,
                              InstanceBase* instance) const override;

  private:
    MaybeError InitializeImpl() override;
    void InitializeSupportedFeaturesImpl() override;
    MaybeError InitializeSupportedLimitsImpl(CombinedLimits* limits) override;

    FeatureValidationResult ValidateFeatureSupportedWithTogglesImpl(
        wgpu::FeatureName feature,
        const TogglesState& toggles) const override;

    void SetupBackendAdapterToggles(dawn::platform::Platform* platform,
                                    TogglesState* adapterToggles) const override;
    void SetupBackendDeviceToggles(dawn::platform::Platform* platform,
                                   TogglesState* deviceToggles) const override;
    ResultOrError<Ref<DeviceBase>> CreateDeviceImpl(
        AdapterBase* adapter,
        const UnpackedPtr<DeviceDescriptor>& descriptor,
        const TogglesState& deviceToggles,
        Ref<DeviceBase::DeviceLostEvent>&& lostEvent) override;
    void PopulateBackendProperties(UnpackedPtr<AdapterInfo>& properties) const override;

    ResultOrError<PhysicalDeviceSurfaceCapabilities> GetSurfaceCapabilities(
        InstanceBase* instance,
        const Surface* surface) const override;
};

}  // namespace dawn::native::interpreter

#endif  // SRC_DAWN_NATIVE_INTERPRETER_PHYSICALDEVICEINTERPRETER_H_
