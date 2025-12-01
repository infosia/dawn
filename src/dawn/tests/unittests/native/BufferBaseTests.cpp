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

#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

#include "dawn/common/Log.h"
#include "dawn/common/Ref.h"
#include "dawn/utils/TestUtils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mocks/BufferMock.h"
#include "mocks/DawnMockTest.h"

using testing::_;
using testing::ByMove;
using testing::Eq;
using testing::Return;

namespace dawn::native {
namespace {

class WaitableEvent {
  public:
    void Signal() {
        std::scoped_lock lock(mMutex);
        mSignaled = true;
        mCv.notify_all();
    }
    void Wait() {
        std::unique_lock<std::mutex> lock(mMutex);
        while (!mSignaled) {
            mCv.wait(lock);
        }
    }

  private:
    std::mutex mMutex;
    std::condition_variable mCv;
    bool mSignaled = false;
};

using MockMapAsyncCallback =
    testing::StrictMock<testing::MockCppCallback<void (*)(wgpu::MapAsyncStatus, wgpu::StringView)>>;

class BufferBaseTest : public DawnMockTest {};

// Test that MapAsyncImpl() throwing an error causes map async callback to return an error status.
TEST_F(BufferBaseTest, MapAsyncImplError) {
    constexpr std::string_view kErrorText = "Platform error";

    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl).WillOnce([&]() -> MaybeError {
        return DAWN_FORMAT_INTERNAL_ERROR(kErrorText);
    });

    // Internal error will cause device loss as well.
    EXPECT_CALL(mDeviceLostCallback,
                Call(_, wgpu::DeviceLostReason::Unknown, testing::HasSubstr(kErrorText)))
        .Times(1);

    MockMapAsyncCallback cb;
    EXPECT_CALL(cb, Call(Eq(wgpu::MapAsyncStatus::Error), Eq(kErrorText)));
    {
        EXPECT_CALL(*mDeviceMock, CreateBufferImpl).WillOnce(Return(ByMove(std::move(bufferMock))));
        wgpu::Buffer buffer = device.CreateBuffer(ToCppAPI(&desc));
        buffer.MapAsync(wgpu::MapMode::Write, 0, desc.size, wgpu::CallbackMode::AllowSpontaneous,
                        cb.Callback());
    }
}
class BufferBaseThreadedTest : public DawnMockTest {
  public:
    BufferBaseThreadedTest() : DawnMockTest() {
        mRequiredFeatures.push_back(wgpu::FeatureName::ImplicitDeviceSynchronization);
    }
};

TEST_F(BufferBaseThreadedTest, ConcurrentUnmap) {
    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));

    WaitableEvent event;
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl);
    EXPECT_CALL(*bufferMock.Get(), UnmapImpl).WillOnce([&]() { event.Wait(); });

    EXPECT_CALL(mDeviceErrorCallback,
                Call(_, wgpu::ErrorType::Validation,
                     testing::HasSubstr("Concurrent buffer operations are not allowed")))
        .Times(1);

    EXPECT_CALL(*mDeviceMock, CreateBufferImpl).WillOnce(Return(ByMove(std::move(bufferMock))));
    wgpu::Buffer buffer = device.CreateBuffer(ToCppAPI(&desc));

    MockMapAsyncCallback cb;
    EXPECT_CALL(cb, Call(Eq(wgpu::MapAsyncStatus::Success), _));
    buffer.MapAsync(wgpu::MapMode::Write, 0, desc.size, wgpu::CallbackMode::AllowProcessEvents,
                    cb.Callback());
    ProcessEvents();

    // Two threads will run APIUnmap(). The first thread blocks UnmapImpl() so the second thread
    // will attempt a concurrent unmap operation.
    utils::RunInParallel(2, [&](uint32_t) {
        buffer.Unmap();
        event.Signal();
    });
}

// TEST_F(BufferBaseThreadedTest, ConcurrentUnmapDestroy) {
//     BufferDescriptor desc = {};
//     desc.size = 16;
//     desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

//     Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));

//     WaitableEvent unmapEvent;
//     WaitableEvent destroyEvent;
//     EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl);
//     EXPECT_CALL(*bufferMock.Get(), UnmapImpl).WillOnce([&]() {
//         destroyEvent.Signal();
//         unmapEvent.Wait();
//     });

//     EXPECT_CALL(mDeviceErrorCallback,
//                 Call(_, wgpu::ErrorType::Validation,
//                      testing::HasSubstr("Concurrent buffer operations are not allowed")))
//         .Times(1);

//     EXPECT_CALL(*mDeviceMock, CreateBufferImpl).WillOnce(Return(ByMove(std::move(bufferMock))));
//     wgpu::Buffer buffer = device.CreateBuffer(ToCppAPI(&desc));

//     MockMapAsyncCallback cb;
//     EXPECT_CALL(cb, Call(Eq(wgpu::MapAsyncStatus::Success), _));
//     buffer.MapAsync(wgpu::MapMode::Write, 0, desc.size, wgpu::CallbackMode::AllowSpontaneous,
//                     cb.Callback());

//     // Two threads will run APIUnmap(). The first thread blocks UnmapImpl() so the second thread
//     // will attempt a concurrent unmap operation.
//     utils::RunInParallel(2, [&](uint32_t index) {
//         if (index == 0) {
//             buffer.Unmap();
//         } else {
//             destroyEvent.Wait();
//             buffer.Destroy();
//             unmapEvent.Signal();
//         }
//     });
// }

TEST_F(BufferBaseThreadedTest, ConcurrentMapUnmap) {
    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));

    WaitableEvent event;
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl).WillOnce([&]() -> MaybeError {
        event.Signal();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return {};
    });

    EXPECT_CALL(mDeviceErrorCallback,
                Call(_, wgpu::ErrorType::Validation,
                     testing::HasSubstr("Concurrent buffer operations are not allowed")))
        .Times(1);

    // Two threads will run APIUnmap(). The first thread blocks UnmapImpl() so the second thread
    // will attempt a concurrent unmap operation.
    utils::RunInParallel(2, [&](uint32_t index) {
        if (index == 0) {
            // First thread will start MapAsync() operation.
            WGPUBufferMapCallbackInfo callbackInfo = {};
            callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
            callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                                       void* result_param, void* userdata_param) {};
            bufferMock->APIMapAsync(wgpu::MapMode::Write, 0, 16, callbackInfo);
        } else {
            // Second thread will start Unmap() operation concurrently with MapAsync() and
            // fail with validation error.
            event.Wait();
            bufferMock->APIUnmap();
        }
    });
}

}  // namespace
}  // namespace dawn::native
