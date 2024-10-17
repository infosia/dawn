// Copyright 2024 The Dawn & Tint Authors
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

#include <dawn/webgpu_cpp_print.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <webgpu/webgpu_cpp.h>

#include <iostream>

namespace {

using testing::_;

class InstanceLevelTests : public testing::Test {
  public:
    void SetUp() override { instance = wgpu::CreateInstance(); }

  protected:
    wgpu::Adapter RequestAdapter(const wgpu::RequestAdapterOptions* adapterOptions = nullptr) {
        wgpu::Adapter result = nullptr;
        instance.WaitAny(
            instance.RequestAdapter(adapterOptions, wgpu::CallbackMode::AllowSpontaneous,
                                    [&result](wgpu::RequestAdapterStatus status,
                                              wgpu::Adapter adapter, wgpu::StringView message) {
                                        ASSERT_EQ(status, wgpu::RequestAdapterStatus::Success);
                                        result = std::move(adapter);
                                    }),
            UINT64_MAX);
        return result;
    }

    wgpu::Instance instance;
};

TEST_F(InstanceLevelTests, RequestAdapter) {
    EXPECT_NE(RequestAdapter(), nullptr);
}

class AdapterLevelTests : public InstanceLevelTests {
  public:
    void SetUp() override {
        InstanceLevelTests::SetUp();
        adapter = RequestAdapter();
    }

  protected:
    wgpu::Device RequestDevice(const wgpu::DeviceDescriptor* descriptor = nullptr) {
        wgpu::Device result = nullptr;
        instance.WaitAny(
            adapter.RequestDevice(descriptor, wgpu::CallbackMode::AllowSpontaneous,
                                  [&result](wgpu::RequestDeviceStatus status, wgpu::Device device,
                                            wgpu::StringView message) {
                                      ASSERT_EQ(status, wgpu::RequestDeviceStatus::Success);
                                      result = std::move(device);
                                  }),
            UINT64_MAX);
        return result;
    }

    wgpu::Adapter adapter;
};

TEST_F(AdapterLevelTests, RequestDevice) {
    EXPECT_NE(RequestDevice(), nullptr);
}

TEST_F(AdapterLevelTests, RequestDeviceThenDestroy) {
    wgpu::Device device = nullptr;
    wgpu::DeviceLostReason reason = wgpu::DeviceLostReason::Unknown;

    wgpu::DeviceDescriptor descriptor = {};
    descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [&device, &reason](const wgpu::Device& d, wgpu::DeviceLostReason r, wgpu::StringView) {
            reason = r;
            EXPECT_EQ(device.Get(), d.Get());
        });
    device = RequestDevice(&descriptor);
    device.Destroy();

    EXPECT_EQ(reason, wgpu::DeviceLostReason::Destroyed);
}

TEST_F(AdapterLevelTests, RequestDeviceThenDrop) {
    wgpu::DeviceLostReason reason = wgpu::DeviceLostReason::Unknown;

    wgpu::DeviceDescriptor descriptor = {};
    descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [&reason](const wgpu::Device&, wgpu::DeviceLostReason r, wgpu::StringView) { reason = r; });
    RequestDevice(&descriptor);

    EXPECT_EQ(reason, wgpu::DeviceLostReason::Destroyed);
}

class DeviceLevelTests : public AdapterLevelTests {
  public:
    void SetUp() override {
        AdapterLevelTests::SetUp();

        wgpu::DeviceDescriptor descriptor = {};
        // descriptor.SetUncapturedErrorCallback(
        //     [](const wgpu::Device& d, wgpu::ErrorType t, wgpu::StringView m,
        //        DeviceLevelTests* self) { self->uncapturedErrorCb.Call(d, t, m); },
        //     this);
        descriptor.SetUncapturedErrorCallback(
            [](const wgpu::Device& d, wgpu::ErrorType t, wgpu::StringView m) { std::cout << m; });
        device = RequestDevice(&descriptor);
    }

  protected:
    wgpu::Device device;

    // Mock callback used for uncaptured errors so that test writers can add expectations on this
    // callback which will enforce the expectations at teardown of the test.
    // testing::StrictMock<
    //     testing::MockFunction<void(const wgpu::Device&, wgpu::ErrorType, wgpu::StringView)>>
    //     uncapturedErrorCb;
};

TEST_F(DeviceLevelTests, UncapturedValidationError) {
    // EXPECT_CALL(uncapturedErrorCb, Call(_, wgpu::ErrorType::Validation, _)).Times(1);

    wgpu::BufferDescriptor desc = {};
    desc.size = 1024;
    desc.usage = static_cast<wgpu::BufferUsage>(UINT64_MAX);
    wgpu::Buffer buffer = device.CreateBuffer(&desc);
}

}  // namespace