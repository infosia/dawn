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

#include <limits>
#include <vector>

// This must be above VulkanBackend.h otherwise vulkan.h will be included before we can wrap it with
// vulkan_platform.h.
#include "dawn/common/vulkan_platform.h"

#include "dawn/native/VulkanBackend.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

using MemoryInstrumentationVulkanTest = DawnTest;

// Test the detailed vulkan memory usage reported by ComputeEstimatedMemoryUsage()
TEST_P(MemoryInstrumentationVulkanTest, ComputeEstimatedMemoryUsageInDetailsForVulkan) {
    // Create a buffer with size 32.
    EXPECT_TRUE(device.Get() != nullptr);
    constexpr uint64_t kBufferSize = 32;
    constexpr wgpu::BufferDescriptor kBufferDesc = {
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
        .size = kBufferSize,
    };

    std::cout << "Saif --- Test - Creating buffer\n";
    // Creating the buffer should allocate memory with Vulkan ResourceMemoryAllocator.
    wgpu::Buffer uniformBuffer = device.CreateBuffer(&kBufferDesc);
    EXPECT_TRUE(uniformBuffer);

    std::cout << "Saif --- Test - ComputeEstimatedMemoryUsageInfo 1\n";
    native::MemoryUsageInfo memInfo = native::ComputeEstimatedMemoryUsageInfo(device.Get());

    EXPECT_EQ(memInfo.totalUsage, kBufferSize);
    EXPECT_EQ(memInfo.buffersUsage, kBufferSize);
    EXPECT_GT(memInfo.vulkanAllocatedMemory, 0u);
    EXPECT_GT(memInfo.vulkanUsedMemory, 0u);
    EXPECT_GT(memInfo.vulkanAllocatedMemory, memInfo.vulkanUsedMemory);

    std::cout << "Saif --- Test - Destroying buffer\n";
    uniformBuffer.Destroy();
    // Process the destroy so that the pending command serials are updated.

    // Need an empty Submit in order for Future to wait for queue serial.
    device.GetQueue().Submit(0, nullptr);
    // Use Futures WaitAny to wait for the queue to update serial.
    wgpu::Future fSubmittedWorkDoneFuture = device.GetQueue().OnSubmittedWorkDone(
        wgpu::CallbackMode::WaitAnyOnly, [](wgpu::QueueWorkDoneStatus) {});
    wgpu::FutureWaitInfo waitInfo{};
    waitInfo.future = fSubmittedWorkDoneFuture;
    const auto& instance = device.GetAdapter().GetInstance();
    if (instance.WaitAny(1, &waitInfo, /*timeoutNS=*/std::numeric_limits<uint64_t>::max()) !=
        wgpu::WaitStatus::Success) {
        DAWN_ASSERT(false);
    }

    // TODO(chromium:404568017): This submit should not really be needed to free the memory but is
    // needed for now. Investigate this.
    device.GetQueue().Submit(0, nullptr);
    // Tick needed in order to call ResourceMemoryAllocator::Tick.
    device.Tick();

    std::cout << "Saif --- Test - ComputeEstimatedMemoryUsageInfo 2\n";
    memInfo = native::ComputeEstimatedMemoryUsageInfo(device.Get());
    // Vulkan used memory should be 0 now.
    EXPECT_EQ(memInfo.vulkanUsedMemory, 0u);
    EXPECT_GT(memInfo.vulkanAllocatedMemory, 0u);
    EXPECT_GT(memInfo.vulkanAllocatedMemory, memInfo.vulkanUsedMemory);
}

DAWN_INSTANTIATE_TEST(MemoryInstrumentationVulkanTest, VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
