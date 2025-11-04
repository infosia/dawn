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

#include <condition_variable>
#include <mutex>
#include <thread>

#include "dawn/common/Log.h"
#include "dawn/common/Ref.h"
#include "dawn/native/Error.h"
#include "dawn/utils/TestUtils.h"
#include "mocks/BufferMock.h"
#include "mocks/DawnMockTest.h"

using testing::_;

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

class BufferMultiThreadTest : public DawnMockTest {
  public:
    BufferMultiThreadTest() : DawnMockTest() {
        mRequiredFeatures.push_back(wgpu::FeatureName::ImplicitDeviceSynchronization);
    }
};

TEST_F(BufferMultiThreadTest, ConcurrentUnmap) {
    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));

    WaitableEvent event;
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl);
    EXPECT_CALL(*bufferMock.Get(), UnmapImpl).WillOnce([&]() { event.Wait(); });

    EXPECT_CALL(mDeviceErrorCallback,
                Call(_, wgpu::ErrorType::Validation,
                     testing::HasSubstr("Concurrent buffer operations are not allowed")))
        .Times(1);

    WGPUBufferMapCallbackInfo callbackInfo = {};
    callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
                               void* result_param, void* userdata_param) {};
    bufferMock->APIMapAsync(wgpu::MapMode::Write, 0, 16, callbackInfo);

    // Two threads will run APIUnmap(). The first thread blocks UnmapImpl() so the second thread
    // will attempt a concurrent unmap operation.
    utils::RunInParallel(2, [&](uint32_t) {
        bufferMock->APIUnmap();
        event.Signal();
    });
}

TEST_F(BufferMultiThreadTest, ConcurrentMapUnmap) {
    BufferDescriptor desc = {};
    desc.size = 16;
    desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::MapWrite;

    Ref<BufferMock> bufferMock = AcquireRef(new BufferMock(mDeviceMock, &desc));

    WaitableEvent event;
    EXPECT_CALL(*bufferMock.Get(), MapAsyncImpl).WillOnce([&]() -> MaybeError {
        event.Signal();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
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
