// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/tests/StringViewMatchers.h"
#include "mocks/BufferMock.h"
#include "mocks/DawnMockTest.h"

namespace dawn::native {
namespace {

using ::testing::_;
using ::testing::ByMove;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SizedStringMatches;

static constexpr char kOomErrorMessage[] = "Out of memory error";

class SkipValidationTests : public DawnMockTest {
    void SetUp() override {
        mDeviceToggles.SetForTesting(Toggle::SkipValidation, true, true);
        DawnMockTest::SetUp();
    }
};

// Tests that validation is re-enabled after an OOM error when skip_validation is active,
// to avoid unpredictable behaviors afterwards. With validation skipped, creating a buffer with
// an invalid descriptor (no usage bits) is accepted. After OOM re-enables validation, the same
// descriptor is rejected with a validation error.
TEST_F(SkipValidationTests, ValidationReenabledAfterOOM) {
    EXPECT_FALSE(mDeviceMock->IsValidationEnabled());

    // A buffer descriptor with no usage bits set is invalid per the WebGPU spec.
    wgpu::BufferDescriptor invalidDesc = {};
    invalidDesc.size = 4;
    invalidDesc.usage = wgpu::BufferUsage::None;

    // With validation skipped, CreateBufferImpl is reached despite the invalid descriptor.
    BufferDescriptor nativeDesc = {};
    nativeDesc.size = invalidDesc.size;
    nativeDesc.usage = static_cast<wgpu::BufferUsage>(invalidDesc.usage);
    Ref<BufferMock> bufferMock = AcquireRef(new NiceMock<BufferMock>(mDeviceMock, &nativeDesc));
    EXPECT_CALL(*mDeviceMock, CreateBufferImpl).WillOnce(Return(ByMove(std::move(bufferMock))));

    wgpu::Buffer buffer = device.CreateBuffer(&invalidDesc);
    EXPECT_NE(buffer, nullptr);

    // Inject OOM — validation is re-enabled to avoid unpredictable behavior afterwards.
    EXPECT_CALL(mDeviceErrorCallback, Call(CHandleIs(device.Get()), wgpu::ErrorType::OutOfMemory,
                                           SizedStringMatches(HasSubstr(kOomErrorMessage))))
        .Times(1);
    device.InjectError(wgpu::ErrorType::OutOfMemory, kOomErrorMessage);

    EXPECT_TRUE(mDeviceMock->IsValidationEnabled());

    // Now the same invalid descriptor triggers a validation error without reaching
    // CreateBufferImpl.
    EXPECT_CALL(*mDeviceMock, CreateBufferImpl).Times(0);
    EXPECT_CALL(mDeviceErrorCallback, Call(CHandleIs(device.Get()), wgpu::ErrorType::Validation, _))
        .Times(1);
    device.CreateBuffer(&invalidDesc);
}

}  // namespace
}  // namespace dawn::native
