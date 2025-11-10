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

#include <gtest/gtest.h>
#include <webgpu/webgpu_cpp.h>

#include "dawn/common/Log.h"
#include "dawn/common/Ref.h"
#include "dawn/native/Error.h"
#include "dawn/utils/TestUtils.h"
#include "mocks/BufferMock.h"
#include "mocks/DawnMockTest.h"

using testing::_;

namespace dawn::native {
namespace {

class BufferBaseTest : public DawnMockTest {
  public:
    BufferBaseTest() : DawnMockTest() {}
};

TEST_F(BufferBaseTest, MapAsyncImplError) {
    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl).WillOnce([&]() -> MaybeError {
        return DAWN_VALIDATION_ERROR("Platform error");
    });

    EXPECT_CALL(mDeviceErrorCallback,
                Call(_, wgpu::ErrorType::Validation, testing::HasSubstr("Platform error")))
        .Times(1);

    WGPUBufferMapCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                               void* result_param, void* userdata_param) {
        EXPECT_EQ(status, WGPUMapAsyncStatus_Error);
    };
    bufferMock->APIMapAsync(wgpu::MapMode::Write, 0, 16, callbackInfo);
}

}  // namespace
}  // namespace dawn::native
