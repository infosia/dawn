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

#include <utility>

#include "dawn/common/Log.h"
#include "dawn/common/MutexProtected.h"
#include "dawn/common/windows_with_undefs.h"
#include "dawn/tests/MockCallback.h"
#include "dawn/tests/end2end/BufferHostMappedFileHandleTests.h"

namespace dawn {
namespace {

class NamedSharedMemoryBackend : public BufferHostMappedFileHandleTestBackend {
  public:
    static BufferHostMappedFileHandleTestBackend* GetInstance() {
        static NamedSharedMemoryBackend backend;
        return &backend;
    }

    const char* Name() const override { return "NamedSharedMemoryMapping"; }

    std::pair<wgpu::Buffer, void*> CreateHostMappedBuffer(
        wgpu::Device device,
        wgpu::BufferUsage usage,
        size_t size,
        std::function<void(void*)> Populate) override {
        LARGE_INTEGER largeSize = {};
        largeSize.QuadPart = size;
        // Create a named shared memory object by using INVALID_HANDLE_VALUE as input file handle.
        // See https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory.
        HANDLE sharedMemoryHandle =
            CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, largeSize.HighPart,
                              largeSize.LowPart, nullptr);

        EXPECT_NE(sharedMemoryHandle, nullptr);

        // Get mapped pointer to the file mapping object for test
        void* ptr = MapViewOfFile(sharedMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        EXPECT_NE(ptr, nullptr);
        Populate(ptr);

        auto DeallocMemory = [=]() {
            // Cleanup mapping and handle.
            EXPECT_TRUE(UnmapViewOfFile(ptr));
            CloseHandle(sharedMemoryHandle);
        };

        wgpu::BufferHostMappedFileHandle hostMappedDesc;
        // We can use `sharedMemoryHandle` directly as `HANDLE` is typedef-ed to `void*`.
        hostMappedDesc.handle = sharedMemoryHandle;
        mDisposeCallback.Use([&](auto callback) {
            hostMappedDesc.disposeCallback = callback->Callback();
            hostMappedDesc.userdata = callback->MakeUserdata(sharedMemoryHandle);
        });

        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.usage = usage;
        bufferDesc.size = size;
        bufferDesc.nextInChain = &hostMappedDesc;

        wgpu::Buffer buffer = device.CreateBuffer(&bufferDesc);
        if (dawn::native::CheckIsErrorForTesting(buffer.Get())) {
            DeallocMemory();
        } else {
            mDisposeCallback.Use([&](auto callback) {
                EXPECT_CALL(*callback, Call(sharedMemoryHandle))
                    .WillOnce(testing::InvokeWithoutArgs(DeallocMemory));
            });
        }

        return std::make_pair(std::move(buffer), ptr);
    }

  private:
    MutexProtected<testing::MockCallback<WGPUCallback>> mDisposeCallback;
};

DAWN_INSTANTIATE_PREFIXED_TEST_P(Win,
                                 BufferHostMappedFileHandleTests,
                                 {D3D12Backend()},
                                 {NamedSharedMemoryBackend::GetInstance()});

}  // anonymous namespace
}  // namespace dawn
