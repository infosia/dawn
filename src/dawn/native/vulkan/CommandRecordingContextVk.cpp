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

#include "src/dawn/native/vulkan/CommandRecordingContextVk.h"

#include "dawn/native/vulkan/DeviceVk.h"

namespace dawn::native::vulkan {

void CommandRecordingContext::AddBufferBarrier(VkAccessFlags srcAccessMask,
                                               VkAccessFlags dstAccessMask,
                                               VkPipelineStageFlags srcStages,
                                               VkPipelineStageFlags dstStages,
                                               wgpu::ShaderStage shaderStage) {
    mBufferSrcAccessMask |= srcAccessMask;
    mBufferDstAccessMask |= dstAccessMask;
    mBufferSrcStages |= srcStages;
    mBufferDstStages |= dstStages;
    mBufferShaderStages |= shaderStage;
}

void CommandRecordingContext::EmitBufferBarriers(Device* device) {
    if (mBufferSrcStages != 0 && mBufferDstStages != 0) {
        VkMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcAccessMask = mBufferSrcAccessMask;
        barrier.dstAccessMask = mBufferDstAccessMask;

        device->fn.CmdPipelineBarrier(commandBuffer, mBufferSrcStages, mBufferDstStages, 0, 1,
                                      &barrier, 0, nullptr, 0, nullptr);
    }

    mBufferSrcAccessMask = 0;
    mBufferDstAccessMask = 0;
    mBufferSrcStages = 0;
    mBufferDstStages = 0;
    mBufferShaderStages = wgpu::ShaderStage::None;
}

}  // namespace dawn::native::vulkan
