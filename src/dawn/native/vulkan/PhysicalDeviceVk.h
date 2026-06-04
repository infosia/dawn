// Copyright 2019 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_VULKAN_PHYSICALDEVICEVK_H_
#define SRC_DAWN_NATIVE_VULKAN_PHYSICALDEVICEVK_H_

#include <memory>
#include <vector>

#include "dawn/common/Ref.h"
#include "dawn/common/vulkan_platform.h"
#include "dawn/native/PhysicalDevice.h"
#include "dawn/native/vulkan/VulkanInfo.h"

namespace dawn::native {
class AHBFunctions;
}  // namespace dawn::native

namespace dawn::native::vulkan {

class VulkanInstance;

class PhysicalDevice : public PhysicalDeviceBase {
  public:
    PhysicalDevice(VulkanInstance* vulkanInstance, VkPhysicalDevice physicalDevice);
    ~PhysicalDevice() override;

    // PhysicalDeviceBase Implementation
    bool SupportsExternalImages() const override;
    bool SupportsFeatureLevel(wgpu::FeatureLevel featureLevel,
                              InstanceBase* instance) const override;

    const VulkanDeviceInfo& GetDeviceInfo() const;
    VkPhysicalDevice GetVkPhysicalDevice() const;
    VulkanInstance* GetVulkanInstance() const;

    bool IsDepthStencilFormatSupported(VkFormat format) const;
    bool IsTextureCompressionASTCSliced3DSupported(VkFormat format) const;

    bool IsAndroidQualcomm() const;
    bool IsAndroidARM() const;
    bool IsAndroidSamsung() const;
    bool IsAndroidImgTec() const;
    bool IsPixel10() const;
<<<<<<< HEAD   (c1179de12ec3ed8feb91e922f12a90ae33f4a8cf [M149] [native] Fix IndirectDrawValidationEncoder merging no)
||||||| BASE   (78f92fc2d65532f577fd57d77032e21979036fd3 Roll third_party/webgpu-cts/ f08551b0f..b507bd117 (5 commits)
    bool IsSwiftshader() const;

    // Check using VkDriverId, which is available in Vk 1.2 or an extension.
=======
    bool IsSwiftshader() const;
    bool IsWindowsAMD() const;

    // Check using VkDriverId, which is available in Vk 1.2 or an extension.
>>>>>>> CHANGE (99622c4f7ef017a2f527e1e1d4afddcb93d04b35 [tint] Collapse nested subgroupMin/Max ops on AMD)
    bool IsIntelMesa() const;
    bool IsAmdMesa() const;
    bool IsAndroidHuawei() const;
    bool IsSwiftshader() const;

    std::optional<uint32_t> GetDefaultComputeSubgroupSize() const;
    std::vector<SubgroupMatrixConfig> EnumerateSubgroupMatrixConfigs(
        const TogglesState& toggles) const;

    ResultOrError<PhysicalDeviceSurfaceCapabilities> GetSurfaceCapabilities(
        InstanceBase* instance,
        const Surface* surface) const override;
    const AHBFunctions* GetOrLoadAHBFunctions();

  private:
    MaybeError InitializeImpl() override;
    void InitializeSupportedFeaturesImpl() override;
    MaybeError InitializeSupportedLimitsImpl(CombinedLimits* limits) override;

    MaybeError InitializeSupportedLimitsInternal(wgpu::FeatureLevel featureLevel,
                                                 CombinedLimits* limits);

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

    std::optional<uint32_t> FindDefaultComputeSubgroupSize() const;
    bool CheckSemaphoreSupport(DeviceExt deviceExt,
                               VkExternalSemaphoreHandleTypeFlagBits handleType) const;

    void PopulateBackendProperties(UnpackedPtr<AdapterInfo>& info,
                                   const TogglesState& adapterToggles) const override;
    void PopulateBackendFormatCapabilities(
        wgpu::TextureFormat format,
        UnpackedPtr<DawnFormatCapabilities>& capabilities) const override;

    // Sets core feature level as not being supported and stores `error` with
    // reason why core isn't supported.
    void SetCoreNotSupported(std::unique_ptr<ErrorData> error);

    VkPhysicalDevice mVkPhysicalDevice;
    Ref<VulkanInstance> mVulkanInstance;
    VulkanDeviceInfo mDeviceInfo = {};

    std::optional<uint32_t> mDefaultComputeSubgroupSize;
    bool mSupportsCoreFeatureLevel = true;
    mutable std::unique_ptr<ErrorData> mCoreError;

#if DAWN_PLATFORM_IS(ANDROID)
    std::unique_ptr<AHBFunctions> mAHBFunctions;
#endif  // DAWN_PLATFORM_IS(ANDROID)
};

}  // namespace dawn::native::vulkan

#endif  // SRC_DAWN_NATIVE_VULKAN_PHYSICALDEVICEVK_H_
