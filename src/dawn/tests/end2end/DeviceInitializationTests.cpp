// Copyright 2021 The Dawn & Tint Authors
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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "dawn/dawn_proc.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/SystemUtils.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class DeviceInitializationTest : public DawnTest {
  protected:
    void SetUp() override {
        // We don't call DawnTest::SetUp() because we want to use the native procs
        // and avoid the wire setup that DawnTest does when --use-wire is passed.
        // This test manages its own instances and devices.
        dawnProcSetProcs(&native::GetProcs());

        // We need to populate mAdapterInfo for various Is* functions to work
        // when suppressing tests.
        auto instance = std::make_unique<native::Instance>();
        for (const native::Adapter& nativeAdapter : instance->EnumerateAdapters()) {
            wgpu::Adapter adapter = wgpu::Adapter(nativeAdapter.Get());
            wgpu::AdapterInfo info;
            adapter.GetInfo(&info);

            const auto& props = GetAdapterProperties();
            if (props.deviceID == info.deviceID && props.vendorID == info.vendorID &&
                props.adapterType == info.adapterType && props.backendType == info.backendType &&
                props.name == std::string(info.device.data, info.device.length)) {
                mAdapterInfo = std::make_unique<wgpu::AdapterInfo>();
                adapter.GetInfo(mAdapterInfo.get());
                break;
            }
        }

        // TODO(crbug.com/474097242): Crashes during teardown w/ D3D11 and D3D12.
        DAWN_SUPPRESS_TEST_IF(IsWindows() && (IsD3D11() || IsD3D12()));
    }

    void TearDown() override {
        mAdapterInfo = nullptr;
        // This check is for crbug.com/474097242 and corresponds to the
        // DAWN_SUPPRESS_TEST_IF with the same conditions in SetUp(). It is
        // necessary due to the crash occurring during teardown even if the test
        // is skipped.
        bool skippedDuringSetup = IsWindows() && (IsD3D11() || IsD3D12());
        if (!skippedDuringSetup) {
            dawnProcSetProcs(nullptr);
        }
    }

    // Test that the device can still be used by creating an async pipeline. Note that this test
    // would be better if we did something like a buffer copy instead, but that can only be done
    // once wgpu::CallbackMode::AllowSpontaneous is completely implemented.
    // TODO(crbug.com/42241003): Update this test do a buffer copy instead.
    void ExpectDeviceUsable(wgpu::Device deviceToTest) {
        deviceToTest.PushErrorScope(wgpu::ErrorFilter::Validation);

        wgpu::ComputePipelineDescriptor desc;
        desc.compute.module = utils::CreateShaderModule(deviceToTest, R"(
            @compute @workgroup_size(1) fn main() {}
        )");

        std::atomic<uint8_t> callbacks = 0;
        deviceToTest.CreateComputePipelineAsync(
            &desc, wgpu::CallbackMode::AllowSpontaneous,
            [&callbacks](wgpu::CreatePipelineAsyncStatus status, wgpu::ComputePipeline pipeline,
                         wgpu::StringView) {
                EXPECT_EQ(status, wgpu::CreatePipelineAsyncStatus::Success);
                EXPECT_NE(pipeline, nullptr);
                callbacks++;
            });

        deviceToTest.PopErrorScope(
            wgpu::CallbackMode::AllowSpontaneous,
            [&callbacks](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type, wgpu::StringView) {
                EXPECT_EQ(status, wgpu::PopErrorScopeStatus::Success);
                EXPECT_EQ(type, wgpu::ErrorType::NoError);
                callbacks++;
            });

        while (callbacks != 2) {
            utils::USleep(100);
        }
    }
};

// Test that device operations are still valid if the reference to the instance
// is dropped.
TEST_P(DeviceInitializationTest, DeviceOutlivesInstance) {
    // Get info of all available adapters and then free the instance.
    // We want to create a device on a fresh instance and adapter each time.
    std::vector<wgpu::AdapterInfo> availableAdapterInfo;
    {
        auto instance = std::make_unique<native::Instance>();
        // TODO(347047627): Use a webgpu.h version of enumerateAdapters
        for (const native::Adapter& nativeAdapter : instance->EnumerateAdapters()) {
            wgpu::Adapter adapter = wgpu::Adapter(nativeAdapter.Get());
            wgpu::AdapterInfo info;
            adapter.GetInfo(&info);

            if (info.backendType == wgpu::BackendType::Null) {
                continue;
            }

            availableAdapterInfo.push_back(std::move(info));
        }
    }

    for (const wgpu::AdapterInfo& desiredInfo : availableAdapterInfo) {
        wgpu::Device device;

        auto instance = std::make_unique<native::Instance>();
        // TODO(347047627): Use a webgpu.h version of enumerateAdapters
        for (native::Adapter& nativeAdapter : instance->EnumerateAdapters()) {
            wgpu::Adapter adapter = wgpu::Adapter(nativeAdapter.Get());
            wgpu::AdapterInfo info;
            adapter.GetInfo(&info);

            if (info.deviceID == desiredInfo.deviceID && info.vendorID == desiredInfo.vendorID &&
                info.adapterType == desiredInfo.adapterType &&
                info.backendType == desiredInfo.backendType) {
                // Create the device, destroy the instance, and break out of the loop.
                device = wgpu::Device::Acquire(nativeAdapter.CreateDevice());
                instance.reset();
                break;
            }
        }

        if (device) {
            ExpectDeviceUsable(std::move(device));
        }
    }
}

// Test that it is still possible to create a device from an adapter after the reference to the
// instance is dropped.
TEST_P(DeviceInitializationTest, AdapterOutlivesInstance) {
    // TODO(crbug.com/472472701): Flakily kills test process when run with
    // software backends and MSVC-compiled binaries.
#if DAWN_COMPILER_IS(MSVC)
    DAWN_SUPPRESS_TEST_IF(IsSwiftshader() || IsANGLESwiftShader() || IsWARP());
#endif  // DAWN_COMPILER_IS(MSVC)

    // Get info of all available adapters and then free the instance.
    // We want to create a device on a fresh instance and adapter each time.
    std::vector<wgpu::AdapterInfo> availableAdapterInfo;
    {
        auto instance = std::make_unique<native::Instance>();
        // TODO(347047627): Use a webgpu.h version of enumerateAdapters
        for (const native::Adapter& nativeAdapter : instance->EnumerateAdapters()) {
            wgpu::Adapter adapter = wgpu::Adapter(nativeAdapter.Get());
            wgpu::AdapterInfo info;
            adapter.GetInfo(&info);

            if (info.backendType == wgpu::BackendType::Null) {
                continue;
            }

            availableAdapterInfo.push_back(std::move(info));
        }
    }

    for (const wgpu::AdapterInfo& desiredInfo : availableAdapterInfo) {
        wgpu::Adapter adapter;

        auto instance = std::make_unique<native::Instance>();
        // TODO(347047627): Use a webgpu.h version of enumerateAdapters
        for (native::Adapter& nativeAdapter : instance->EnumerateAdapters()) {
            wgpu::AdapterInfo info;
            wgpu::Adapter(nativeAdapter.Get()).GetInfo(&info);

            if (info.deviceID == desiredInfo.deviceID && info.vendorID == desiredInfo.vendorID &&
                info.adapterType == desiredInfo.adapterType &&
                info.backendType == desiredInfo.backendType) {
                // Save the adapter, and reset the instance.
                adapter = wgpu::Adapter(nativeAdapter.Get());
                instance.reset();
                break;
            }
        }

        if (adapter) {
            ExpectDeviceUsable(adapter.CreateDevice());
        }
    }
}

DAWN_INSTANTIATE_TEST(DeviceInitializationTest,
                      D3D11Backend(),
                      D3D12Backend(),
                      MetalBackend(),
                      NullBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend(),
                      WebGPUBackend());

}  // anonymous namespace
}  // namespace dawn
