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

#include <string>
#include <utility>

#include "absl/strings/str_format.h"
#include "dawn/common/Log.h"
#include "dawn/native/vulkan/DeviceVk.h"
#include "vulkan/vulkan_core.h"

namespace dawn::native::vulkan {
namespace {

// Separate barriers with vertex stages in destination stages from all other barriers.
// This avoids creating unnecessary fragment->vertex dependencies when merging barriers.
// Eg. merging a compute->vertex barrier and a fragment->fragment barrier would create
// a compute|fragment->vertex|fragment barrier.
constexpr VkPipelineStageFlags vertexStages = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                                              VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                              VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

}  // namespace
std::string AccessFlagToString(VkAccessFlags flags) {
    if (flags == VK_ACCESS_NONE) {
        return "NONE";
    }
    static constexpr std::pair<VkAccessFlagBits, const char*> kFlags[] = {
        {VK_ACCESS_INDIRECT_COMMAND_READ_BIT, "INDIRECT_COMMAND_READ"},
        {VK_ACCESS_INDEX_READ_BIT, "INDEX_READ"},
        {VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, "VERTEX_ATTRIBUTE_READ"},
        {VK_ACCESS_UNIFORM_READ_BIT, "UNIFORM_READ"},
        {VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, "INPUT_ATTACHMENT_READ"},
        {VK_ACCESS_SHADER_READ_BIT, "SHADER_READ"},
        {VK_ACCESS_SHADER_WRITE_BIT, "SHADER_WRITE"},
        {VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, "COLOR_ATTACHMENT_READ"},
        {VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, "COLOR_ATTACHMENT_WRITE"},
        {VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, "DEPTH_STENCIL_ATTACHMENT_READ"},
        {VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, "DEPTH_STENCIL_ATTACHMENT_WRITE"},
        {VK_ACCESS_TRANSFER_READ_BIT, "TRANSFER_READ"},
        {VK_ACCESS_TRANSFER_WRITE_BIT, "TRANSFER_WRITE"},
        {VK_ACCESS_HOST_READ_BIT, "HOST_READ"},
        {VK_ACCESS_HOST_WRITE_BIT, "HOST_WRITE"},
        {VK_ACCESS_MEMORY_READ_BIT, "MEMORY_READ"},
        {VK_ACCESS_MEMORY_WRITE_BIT, "MEMORY_WRITE"}};

    std::string s;
    for (auto& flag : kFlags) {
        if (flag.first & flags) {
            if (!s.empty()) {
                s += "|";
            }
            s += flag.second;
        }
    }
    return s;
}

std::string PipelineStageToString(VkPipelineStageFlags flags) {
    if (flags == VK_PIPELINE_STAGE_NONE) {
        return "NONE";
    }
    static constexpr std::pair<VkPipelineStageFlagBits, const char*> kFlags[] = {
        {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, "TOP_OF_PIPE_BIT"},
        {VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, "DRAW_INDIRECT_BIT"},
        {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, "VERTEX_INPUT_BIT"},
        {VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, "VERTEX_SHADER_BIT"},
        {VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, "TESSELLATION_CONTROL_SHADER_BIT"},
        {VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
         "TESSELLATION_EVALUATION_SHADER_BIT"},
        {VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, "GEOMETRY_SHADER_BIT"},
        {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, "FRAGMENT_SHADER_BIT"},
        {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, "EARLY_FRAGMENT_TESTS_BIT"},
        {VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, "LATE_FRAGMENT_TESTS_BIT"},
        {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, "COLOR_ATTACHMENT_OUTPUT_BIT"},
        {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, "COMPUTE_SHADER_BIT"},
        {VK_PIPELINE_STAGE_TRANSFER_BIT, "TRANSFER_BIT"},
        {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, "BOTTOM_OF_PIPE_BIT"},
        {VK_PIPELINE_STAGE_HOST_BIT, "HOST_BIT"},
        {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, "ALL_GRAPHICS_BIT"},
        {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, "ALL_COMMANDS_BIT"}};

    std::string s;
    for (auto& flag : kFlags) {
        if (flag.first & flags) {
            if (!s.empty()) {
                s += "|";
            }
            s += flag.second;
        }
    }
    return s;
}

void CommandRecordingContext::AddBufferBarrier(VkAccessFlags srcAccessMask,
                                               VkAccessFlags dstAccessMask,
                                               VkPipelineStageFlags srcStages,
                                               VkPipelineStageFlags dstStages) {
    BufferBarrier* barrier = nullptr;
    if (dstStages & vertexStages) {
        barrier = &mVertexBufferBarrier;
    } else {
        barrier = &mNonVertexBufferBarrier;
    }

    barrier->bufferSrcAccessMask |= srcAccessMask;
    barrier->bufferDstAccessMask |= dstAccessMask;
    barrier->bufferSrcStages |= srcStages;
    barrier->bufferDstStages |= dstStages;
}

void CommandRecordingContext::EmitBufferBarriers(Device* device) {
    auto EmitBarrierIfNeeded = [&](const BufferBarrier& barrier) {
        if (barrier.bufferSrcStages == 0 || barrier.bufferDstStages == 0) {
            return;
        }

        VkMemoryBarrier vkBarrier;
        vkBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        vkBarrier.pNext = nullptr;
        vkBarrier.srcAccessMask = barrier.bufferSrcAccessMask;
        vkBarrier.dstAccessMask = barrier.bufferDstAccessMask;

        // dawn::ErrorLog() << "EmitBarrier "
        //                  << absl::StrFormat(
        //                         "srcStages=%s srcAccessMask=%s dstStages=%s dstAccessMask=%s",
        //                         PipelineStageToString(barrier.bufferSrcStages),
        //                         AccessFlagToString(barrier.bufferSrcAccessMask),
        //                         PipelineStageToString(barrier.bufferDstStages),
        //                         AccessFlagToString(barrier.bufferDstAccessMask));

        device->fn.CmdPipelineBarrier(commandBuffer, barrier.bufferSrcStages,
                                      barrier.bufferDstStages, 0, 1, &vkBarrier, 0, nullptr, 0,
                                      nullptr);
    };
    // Emit vertex and non-vertex barriers separately to keep src and dst stage masks separate.
    EmitBarrierIfNeeded(mVertexBufferBarrier);
    EmitBarrierIfNeeded(mNonVertexBufferBarrier);

    mVertexBufferBarrier = {};
    mNonVertexBufferBarrier = {};
}

}  // namespace dawn::native::vulkan
