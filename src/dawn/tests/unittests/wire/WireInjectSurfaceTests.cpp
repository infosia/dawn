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

#include "dawn/tests/unittests/wire/WireTest.h"

#include "dawn/wire/WireClient.h"
#include "dawn/wire/WireServer.h"

namespace dawn::wire {
namespace {

using testing::Mock;
using testing::Return;

class WireInjectSurfaceTests : public WireTest {
  public:
    void SetUp() override {
        WireTest::SetUp();

        mCapabilities.usages = mSupportedUsages;
        mCapabilities.formatCount = mSupportedFormats.size();
        mCapabilities.formats = mSupportedFormats.data();
        mCapabilities.presentModeCount = mSupportedPresentModes.size();
        mCapabilities.presentModes = mSupportedPresentModes.data();
        mCapabilities.alphaModeCount = mSupportedAlphaModes.size();
        mCapabilities.alphaModes = mSupportedAlphaModes.data();

        mConfiguration.format = mCapabilities.formats[0];
        mConfiguration.alphaMode = mCapabilities.alphaModes[0];
        mConfiguration.presentMode = mCapabilities.presentModes[0];
        mConfiguration.width = 100;
        mConfiguration.height = 100;
        mConfiguration.usage = WGPUTextureUsage_RenderAttachment;
        mConfiguration.device = device;
        mConfiguration.viewFormatCount = 0;
    }

    WGPUSurfaceCapabilities mCapabilities;
    WGPUTextureUsage mSupportedUsages =
        WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
    std::array<WGPUTextureFormat, 2> mSupportedFormats = {WGPUTextureFormat_RGBA8Unorm,
                                                          WGPUTextureFormat_RGBA8UnormSrgb};
    std::array<WGPUPresentMode, 2> mSupportedPresentModes = {WGPUPresentMode_Fifo,
                                                             WGPUPresentMode_Mailbox};
    std::array<WGPUCompositeAlphaMode, 2> mSupportedAlphaModes = {WGPUCompositeAlphaMode_Opaque,
                                                                  WGPUCompositeAlphaMode_Inherit};

    WGPUSurfaceConfiguration mConfiguration;
};

// Test that reserving and injecting a surface makes calls on the client object forward to the
// server object correctly.
TEST_F(WireInjectSurfaceTests, CallAfterReserveInject) {
    auto reserved = GetWireClient()->ReserveSurface(instance, &mCapabilities);

    WGPUSurface apiSurface = api.GetNewSurface();
    EXPECT_CALL(api, SurfaceAddRef(apiSurface));
    ASSERT_TRUE(
        GetWireServer()->InjectSurface(apiSurface, reserved.handle, reserved.instanceHandle));

    wgpuSurfacePresent(reserved.surface);
    EXPECT_CALL(api, SurfacePresent(apiSurface));
    FlushClient();
}

// Test that reserve correctly returns different IDs each time.
TEST_F(WireInjectSurfaceTests, ReserveDifferentIDs) {
    auto reserved1 = GetWireClient()->ReserveSurface(instance, &mCapabilities);
    auto reserved2 = GetWireClient()->ReserveSurface(instance, &mCapabilities);

    ASSERT_NE(reserved1.handle.id, reserved2.handle.id);
    ASSERT_NE(reserved1.surface, reserved2.surface);
}

// Test that injecting the same id without a destroy first fails.
TEST_F(WireInjectSurfaceTests, InjectExistingID) {
    auto reserved = GetWireClient()->ReserveSurface(instance, &mCapabilities);

    WGPUSurface apiSurface = api.GetNewSurface();
    EXPECT_CALL(api, SurfaceAddRef(apiSurface));
    ASSERT_TRUE(
        GetWireServer()->InjectSurface(apiSurface, reserved.handle, reserved.instanceHandle));

    // ID already in use, call fails.
    ASSERT_FALSE(
        GetWireServer()->InjectSurface(apiSurface, reserved.handle, reserved.instanceHandle));
}

// Test that the server only borrows the surface and does a single addref-release
TEST_F(WireInjectSurfaceTests, InjectedSurfaceLifetime) {
    auto reserved = GetWireClient()->ReserveSurface(instance, &mCapabilities);

    // Injecting the surface adds a reference
    WGPUSurface apiSurface = api.GetNewSurface();
    EXPECT_CALL(api, SurfaceAddRef(apiSurface));
    ASSERT_TRUE(
        GetWireServer()->InjectSurface(apiSurface, reserved.handle, reserved.instanceHandle));

    // Releasing the surface removes a single reference.
    wgpuSurfaceRelease(reserved.surface);
    EXPECT_CALL(api, SurfaceRelease(apiSurface));
    FlushClient();

    // Deleting the server doesn't release a second reference.
    DeleteServer();
    Mock::VerifyAndClearExpectations(&api);
}

// Test that a surface reservation can be reclaimed. This is necessary to
// avoid leaking ObjectIDs for reservations that are never injected.
TEST_F(WireInjectSurfaceTests, ReclaimSurfaceReservation) {
    // Test that doing a reservation and full release is an error.
    {
        auto reserved = GetWireClient()->ReserveSurface(instance, &mCapabilities);
        wgpuSurfaceRelease(reserved.surface);
        FlushClient(false);
    }

    // Test that doing a reservation and then reclaiming it recycles the ID.
    {
        auto reserved1 = GetWireClient()->ReserveSurface(instance, &mCapabilities);
        GetWireClient()->ReclaimSurfaceReservation(reserved1);

        auto reserved2 = GetWireClient()->ReserveSurface(instance, &mCapabilities);

        // The ID is the same, but the generation is still different.
        ASSERT_EQ(reserved1.handle.id, reserved2.handle.id);
        ASSERT_NE(reserved1.handle.generation, reserved2.handle.generation);

        // No errors should occur.
        FlushClient();
    }
}

// Check capabalities returned.
// Check texture reflection.
// Check configure / unconfigure sends correct device id to server.
// Check getCurrentTexture, never configured.
// Check getCurrentTexture, unconfigured.
// Check getCurrentTexture, device destroyed.

// Test that the texture's reflection is correct for injected surface in the wire.
TEST_F(WireInjectSurfaceTests, TextureReflection) {
    // auto reserved = GetWireClient()->ReserveSurface(instance, &mCapabilities);

    // WGPUSurface apiSurface = api.GetNewSurface();
    // EXPECT_CALL(api, SurfaceAddRef(apiSurface));
    // ASSERT_TRUE(
    //     GetWireServer()->InjectSurface(apiSurface, reserved.handle, reserved.instanceHandle));

    // EXPECT_CALL(api, SurfaceConfigure(_));
    // wgpuSurfaceConfigure(reserved.surface, &mConfiguration);

    // WGPUSurfaceTexture tex = wgpuSurfaceGetCurrentTexture(reserved.surface);
    // WGPUTexture apiTex = api.GetNewTexture();
    // EXPECT_CALL(api, Surface.GetCurrentTexture(apiSurface)).WillOnce(Return(apiTex));
    // FlushClient();

    // EXPECT_EQ(swapChainDesc.width, wgpuTextureGetWidth(tex));
    // EXPECT_EQ(swapChainDesc.height, wgpuTextureGetHeight(tex));
    // EXPECT_EQ(swapChainDesc.usage, wgpuTextureGetUsage(tex));
    // EXPECT_EQ(swapChainDesc.format, wgpuTextureGetFormat(tex));
    // EXPECT_EQ(1u, wgpuTextureGetDepthOrArrayLayers(tex));
    // EXPECT_EQ(1u, wgpuTextureGetMipLevelCount(tex));
    // EXPECT_EQ(1u, wgpuTextureGetSampleCount(tex));
    // EXPECT_EQ(WGPUTextureDimension_2D, wgpuTextureGetDimension(tex));
}

}  // anonymous namespace
}  // namespace dawn::wire
