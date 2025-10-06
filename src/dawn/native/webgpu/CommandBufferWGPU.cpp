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

#include "dawn/native/webgpu/CommandBufferWGPU.h"

#include <vector>

#include "dawn/common/Assert.h"
#include "dawn/common/StringViewUtils.h"
#include "dawn/native/webgpu/BindGroupWGPU.h"
#include "dawn/native/webgpu/BufferWGPU.h"
#include "dawn/native/webgpu/CaptureContext.h"
#include "dawn/native/webgpu/ComputePipelineWGPU.h"
#include "dawn/native/webgpu/DeviceWGPU.h"
#include "dawn/native/webgpu/RenderPipelineWGPU.h"
#include "dawn/native/webgpu/Serialization.h"
#include "dawn/native/webgpu/TextureWGPU.h"
#include "dawn/native/webgpu/ToWGPU.h"

namespace dawn::native::webgpu {

// static
Ref<CommandBuffer> CommandBuffer::Create(CommandEncoder* encoder,
                                         const CommandBufferDescriptor* descriptor) {
    return AcquireRef(new CommandBuffer(encoder, descriptor));
}

CommandBuffer::CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor)
    : CommandBufferBase(encoder, descriptor),
      ImmutableRecordableObject(schema::ObjectType::CommandBuffer) {}

namespace {

void EncodeComputePass(const DawnProcTable& wgpu,
                       WGPUCommandEncoder innerEncoder,
                       CommandIterator& commands,
                       BeginComputePassCmd* computePassCmd,
                       const ComputePassResourceUsage& resourceUsages,
                       bool isCapturing) {
    WGPUComputePassDescriptor passDescriptor{
        .nextInChain = nullptr,
        .label = ToOutputStringView(computePassCmd->label),
        .timestampWrites = nullptr,
    };
    WGPUPassTimestampWrites timestampWrites;
    if (computePassCmd->timestampWrites.querySet) {
        timestampWrites = ToWGPU(computePassCmd->timestampWrites);
        passDescriptor.timestampWrites = &timestampWrites;
    }

    WGPUComputePassEncoder passEncoder =
        wgpu.commandEncoderBeginComputePass(innerEncoder, &passDescriptor);
    uint64_t currentDispatch = 0;

    Command type;
    while (commands.NextCommandId(&type)) {
        switch (type) {
            case Command::EndComputePass: {
                commands.NextCommand<EndComputePassCmd>();
                wgpu.computePassEncoderEnd(passEncoder);
                return;
            }

            case Command::Dispatch: {
                auto cmd = commands.NextCommand<DispatchCmd>();
                wgpu.computePassEncoderDispatchWorkgroups(passEncoder, cmd->x, cmd->y, cmd->z);
                if (!isCapturing) {
                    // any buffers or textures that could be written to need to be marked as needs
                    // capture
                    const auto& usage = resourceUsages.dispatchUsages[currentDispatch];
                    for (size_t i = 0; i < usage.buffers.size(); ++i) {
                        const auto& syncInfo = usage.bufferSyncInfos[i];
                        // TODO(413053623): This needs to be WRITE storage. If we're
                        // only reading then we don't need to mark it as needs capture.
                        if (syncInfo.usage & wgpu::BufferUsage::Storage &&
                            syncInfo.shaderStages & wgpu::ShaderStage::Compute) {
                            ToBackend(usage.buffers[i])->MarkAsNeedsCapture();
                        }
                    }
                }
                currentDispatch++;
                break;
            }

            case Command::DispatchIndirect: {
                auto cmd = commands.NextCommand<DispatchIndirectCmd>();
                wgpu.computePassEncoderDispatchWorkgroupsIndirect(
                    passEncoder, ToBackend(cmd->indirectBuffer)->GetInnerHandle(),
                    cmd->indirectOffset);
                currentDispatch++;
                break;
            }

            case Command::SetComputePipeline: {
                auto cmd = commands.NextCommand<SetComputePipelineCmd>();
                wgpu.computePassEncoderSetPipeline(passEncoder,
                                                   ToBackend(cmd->pipeline)->GetInnerHandle());
                break;
            }

            case Command::SetBindGroup: {
                auto cmd = commands.NextCommand<SetBindGroupCmd>();
                uint32_t* dynamicOffsets = nullptr;
                if (cmd->dynamicOffsetCount > 0) {
                    dynamicOffsets = commands.NextData<uint32_t>(cmd->dynamicOffsetCount);
                }
                wgpu.computePassEncoderSetBindGroup(passEncoder, static_cast<uint32_t>(cmd->index),
                                                    ToBackend(cmd->group)->GetInnerHandle(),
                                                    cmd->dynamicOffsetCount, dynamicOffsets);
                break;
            }
            case Command::InsertDebugMarker: {
                auto cmd = commands.NextCommand<InsertDebugMarkerCmd>();
                char* label = commands.NextData<char>(cmd->length + 1);
                wgpu.computePassEncoderInsertDebugMarker(passEncoder, {label, cmd->length});
                break;
            }

            case Command::PopDebugGroup: {
                commands.NextCommand<PopDebugGroupCmd>();
                wgpu.computePassEncoderPopDebugGroup(passEncoder);
                break;
            }

            case Command::PushDebugGroup: {
                auto cmd = commands.NextCommand<PushDebugGroupCmd>();
                char* label = commands.NextData<char>(cmd->length + 1);
                wgpu.computePassEncoderPushDebugGroup(passEncoder, {label, cmd->length});
                break;
            }

            case Command::WriteTimestamp: {
                auto cmd = commands.NextCommand<WriteTimestampCmd>();
                // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented for
                // QuerySetWGPU
                wgpu.computePassEncoderWriteTimestamp(
                    passEncoder, nullptr /*ToBackend(cmd->querySet)->GetInnerHandle()*/,
                    cmd->queryIndex);
                break;
            }

            case Command::SetImmediateData: {
                auto cmd = commands.NextCommand<SetImmediateDataCmd>();
                DAWN_ASSERT(cmd->size > 0);
                uint8_t* value = nullptr;
                value = commands.NextData<uint8_t>(cmd->size);
                wgpu.computePassEncoderSetImmediateData(passEncoder, cmd->offset, value, cmd->size);
                break;
            }

            default: {
                DAWN_UNREACHABLE();
                break;
            }
        }
    }

    // EndComputePass should have been called
    DAWN_UNREACHABLE();
}

void EncodeRenderBundleCommand(const DawnProcTable& wgpu,
                               WGPURenderPassEncoder passEncoder,
                               CommandIterator& commands,
                               Command type) {
    switch (type) {
        case Command::Draw: {
            auto cmd = commands.NextCommand<DrawCmd>();
            wgpu.renderPassEncoderDraw(passEncoder, cmd->vertexCount, cmd->instanceCount,
                                       cmd->firstVertex, cmd->firstInstance);
            break;
        }

        case Command::DrawIndexed: {
            auto cmd = commands.NextCommand<DrawIndexedCmd>();
            wgpu.renderPassEncoderDrawIndexed(passEncoder, cmd->indexCount, cmd->instanceCount,
                                              cmd->firstIndex, cmd->baseVertex, cmd->firstInstance);
            break;
        }

        case Command::DrawIndirect: {
            auto cmd = commands.NextCommand<DrawIndirectCmd>();
            wgpu.renderPassEncoderDrawIndirect(
                passEncoder, ToBackend(cmd->indirectBuffer)->GetInnerHandle(), cmd->indirectOffset);
            break;
        }

        case Command::DrawIndexedIndirect: {
            auto cmd = commands.NextCommand<DrawIndexedIndirectCmd>();
            wgpu.renderPassEncoderDrawIndexedIndirect(
                passEncoder, ToBackend(cmd->indirectBuffer)->GetInnerHandle(), cmd->indirectOffset);
            break;
        }

        case Command::MultiDrawIndirect: {
            DAWN_UNREACHABLE();
            break;
        }

        case Command::MultiDrawIndexedIndirect: {
            DAWN_UNREACHABLE();
            break;
        }

        case Command::InsertDebugMarker: {
            auto cmd = commands.NextCommand<InsertDebugMarkerCmd>();
            char* label = commands.NextData<char>(cmd->length + 1);
            wgpu.renderPassEncoderInsertDebugMarker(passEncoder, {label, cmd->length});
            break;
        }

        case Command::PopDebugGroup: {
            commands.NextCommand<PopDebugGroupCmd>();
            wgpu.renderPassEncoderPopDebugGroup(passEncoder);
            break;
        }

        case Command::PushDebugGroup: {
            auto cmd = commands.NextCommand<PushDebugGroupCmd>();
            char* label = commands.NextData<char>(cmd->length + 1);
            wgpu.renderPassEncoderPushDebugGroup(passEncoder, {label, cmd->length});
            break;
        }

        case Command::SetBindGroup: {
            auto cmd = commands.NextCommand<SetBindGroupCmd>();
            uint32_t* dynamicOffsets = nullptr;
            if (cmd->dynamicOffsetCount > 0) {
                dynamicOffsets = commands.NextData<uint32_t>(cmd->dynamicOffsetCount);
            }
            wgpu.renderPassEncoderSetBindGroup(passEncoder, static_cast<uint32_t>(cmd->index),
                                               ToBackend(cmd->group)->GetInnerHandle(),
                                               cmd->dynamicOffsetCount, dynamicOffsets);
            break;
        }

        case Command::SetIndexBuffer: {
            auto cmd = commands.NextCommand<SetIndexBufferCmd>();
            wgpu.renderPassEncoderSetIndexBuffer(passEncoder,
                                                 ToBackend(cmd->buffer)->GetInnerHandle(),
                                                 ToWGPU(cmd->format), cmd->offset, cmd->size);
            break;
        }

        case Command::SetRenderPipeline: {
            auto cmd = commands.NextCommand<SetRenderPipelineCmd>();
            wgpu.renderPassEncoderSetPipeline(passEncoder,
                                              ToBackend(cmd->pipeline)->GetInnerHandle());
            break;
        }

        case Command::SetVertexBuffer: {
            auto cmd = commands.NextCommand<SetVertexBufferCmd>();
            wgpu.renderPassEncoderSetVertexBuffer(passEncoder, static_cast<uint8_t>(cmd->slot),
                                                  ToBackend(cmd->buffer)->GetInnerHandle(),
                                                  cmd->offset, cmd->size);
            break;
        }

        case Command::SetImmediateData: {
            auto cmd = commands.NextCommand<SetImmediateDataCmd>();
            DAWN_ASSERT(cmd->size > 0);
            uint8_t* value = nullptr;
            value = commands.NextData<uint8_t>(cmd->size);
            wgpu.renderPassEncoderSetImmediateData(passEncoder, cmd->offset, value, cmd->size);
            break;
        }

        default:
            DAWN_UNREACHABLE();
            break;
    }
}

void EncodeRenderPass(const DawnProcTable& wgpu,
                      WGPUCommandEncoder innerEncoder,
                      CommandIterator& commands,
                      BeginRenderPassCmd* renderPassCmd) {
    PerColorAttachment<WGPURenderPassColorAttachment> colorAttachments = {};

    size_t colorAttachmentCount = 0;
    for (auto i : renderPassCmd->attachmentState->GetColorAttachmentsMask()) {
        colorAttachments[i] = ToWGPU(renderPassCmd->colorAttachments[i]);
        colorAttachmentCount = static_cast<size_t>(i) + 1;
    }

    WGPURenderPassDescriptor passDescriptor{
        .nextInChain = nullptr,
        .label = ToOutputStringView(renderPassCmd->label),
        .colorAttachmentCount = colorAttachmentCount,
        .colorAttachments = colorAttachments.data(),
        .depthStencilAttachment = nullptr,
        // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented for
        // QuerySetWGPU
        .occlusionQuerySet = nullptr /*ToBackend(cmd->occlusionQuerySet)->GetInnerHandle()*/,
        .timestampWrites = nullptr,
    };
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    if (renderPassCmd->attachmentState->HasDepthStencilAttachment()) {
        depthStencilAttachment = ToWGPU(renderPassCmd->depthStencilAttachment);
        passDescriptor.depthStencilAttachment = &depthStencilAttachment;
    }
    WGPUPassTimestampWrites timestampWrites;
    if (renderPassCmd->timestampWrites.querySet) {
        timestampWrites = ToWGPU(renderPassCmd->timestampWrites);
        passDescriptor.timestampWrites = &timestampWrites;
    }
    WGPURenderPassEncoder passEncoder =
        wgpu.commandEncoderBeginRenderPass(innerEncoder, &passDescriptor);

    Command type;
    while (commands.NextCommandId(&type)) {
        switch (type) {
            case Command::EndRenderPass: {
                commands.NextCommand<EndRenderPassCmd>();
                wgpu.renderPassEncoderEnd(passEncoder);
                return;
            }

            case Command::SetBlendConstant: {
                auto cmd = commands.NextCommand<SetBlendConstantCmd>();
                WGPUColor color = ToWGPU(cmd->color);
                wgpu.renderPassEncoderSetBlendConstant(passEncoder, &color);
                break;
            }

            case Command::SetStencilReference: {
                auto cmd = commands.NextCommand<SetStencilReferenceCmd>();
                wgpu.renderPassEncoderSetStencilReference(passEncoder, cmd->reference);
                break;
            }

            case Command::SetViewport: {
                auto cmd = commands.NextCommand<SetViewportCmd>();
                wgpu.renderPassEncoderSetViewport(passEncoder, cmd->x, cmd->y, cmd->width,
                                                  cmd->height, cmd->minDepth, cmd->maxDepth);
                break;
            }

            case Command::SetScissorRect: {
                auto cmd = commands.NextCommand<SetScissorRectCmd>();
                wgpu.renderPassEncoderSetScissorRect(passEncoder, cmd->x, cmd->y, cmd->width,
                                                     cmd->height);
                break;
            }

            case Command::ExecuteBundles: {
                auto* cmd = commands.NextCommand<ExecuteBundlesCmd>();
                /*auto bundles = */ commands.NextData<Ref<RenderBundleBase>>(cmd->count);
                std::vector<WGPURenderBundle> wgpuBundles;
                wgpuBundles.reserve(cmd->count);
                for (uint32_t i = 0; i < cmd->count; ++i) {
                    // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented
                    // for RenderBundleWGPU
                    wgpuBundles.emplace_back(
                        nullptr /*ToBackend(cmd->bundles[i])->GetInnerHandle()*/);
                }
                wgpu.renderPassEncoderExecuteBundles(passEncoder, wgpuBundles.size(),
                                                     wgpuBundles.data());
                break;
            }

            case Command::BeginOcclusionQuery: {
                auto cmd = commands.NextCommand<BeginOcclusionQueryCmd>();
                wgpu.renderPassEncoderBeginOcclusionQuery(passEncoder, cmd->queryIndex);
                break;
            }

            case Command::EndOcclusionQuery: {
                commands.NextCommand<EndOcclusionQueryCmd>();
                wgpu.renderPassEncoderEndOcclusionQuery(passEncoder);
                break;
            }

            case Command::WriteTimestamp: {
                auto cmd = commands.NextCommand<WriteTimestampCmd>();
                // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented for
                // QuerySetWGPU
                wgpu.renderPassEncoderWriteTimestamp(
                    passEncoder, nullptr /*ToBackend(cmd->querySet)->GetInnerHandle()*/,
                    cmd->queryIndex);
                break;
            }

            default: {
                EncodeRenderBundleCommand(wgpu, passEncoder, commands, type);
                break;
            }
        }
    }

    // EndRenderPass should have been called
    DAWN_UNREACHABLE();
}

void Capture(CaptureContext& captureContext, const CopyBufferToBufferCmd& cmd) {
    schema::EncoderCommandCopyBufferToBufferCmd data{{
        .data = {{
            .srcBufferId = captureContext.GetId(cmd.source.Get()),
            .srcOffset = cmd.sourceOffset,
            .dstBufferId = captureContext.GetId(cmd.destination.Get()),
            .dstOffset = cmd.destinationOffset,
            .size = cmd.size,
        }},
    }};
    Serialize(captureContext, data);
}

void Capture(CaptureContext& captureContext, const CopyBufferToTextureCmd& cmd) {
    schema::EncoderCommandCopyBufferToTextureCmd data{{
        .data = {{
            .source = ToSchema(captureContext, cmd.source),
            .destination = ToSchema(captureContext, cmd.destination),
            .copySize = ToSchema(cmd.copySize),
        }},
    }};
    Serialize(captureContext, data);
}

void Capture(CaptureContext& captureContext, const CopyTextureToBufferCmd& cmd) {
    schema::EncoderCommandCopyTextureToBufferCmd data{{
        .data = {{
            .source = ToSchema(captureContext, cmd.source),
            .destination = ToSchema(captureContext, cmd.destination),
            .copySize = ToSchema(cmd.copySize),
        }},
    }};
    Serialize(captureContext, data);
}

void Capture(CaptureContext& captureContext, const CopyTextureToTextureCmd& cmd) {
    schema::EncoderCommandCopyTextureToTextureCmd data{{
        .data = {{
            .source = ToSchema(captureContext, cmd.source),
            .destination = ToSchema(captureContext, cmd.destination),
            .copySize = ToSchema(cmd.copySize),
        }},
    }};
    Serialize(captureContext, data);
}

/*
void Capture(CaptureContext& captureContext, const DrawIndexedCmd& cmd) {
    // Write DrawIndexed command parameters
    Capture(captureContext, cmd.indexCount);
    Capture(captureContext, cmd.instanceCount);
    Capture(captureContext, cmd.firstIndex);
    Capture(captureContext, cmd.baseVertex);
    Capture(captureContext, cmd.firstInstance);
}

void Capture(CaptureContext& captureContext, const DrawIndirectCmd& cmd) {
    // Write DrawIndirect command parameters
    CaptureRef(captureContext, cmd.indirectBuffer);
    Capture(captureContext, cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const DrawIndexedIndirectCmd& cmd) {
    // Write DrawIndexedIndirect command parameters
    CaptureRef(captureContext, cmd.indirectBuffer);
    Capture(captureContext, cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const MultiDrawIndirectCmd& cmd) {
    // Write MultiDrawIndirect command parameters
    CaptureRef(captureContext, cmd.indirectBuffer);
    Capture(captureContext, cmd.indirectOffset);
    Capture(captureContext, cmd.maxDrawCount);
    CaptureRef(captureContext, cmd.drawCountBuffer);
    Capture(captureContext, cmd.drawCountOffset);
}

void Capture(CaptureContext& captureContext, const MultiDrawIndexedIndirectCmd& cmd) {
    // Write MultiDrawIndexedIndirect command parameters
    CaptureRef(captureContext, cmd.indirectBuffer);
    Capture(captureContext, cmd.indirectOffset);
    Capture(captureContext, cmd.maxDrawCount);
    CaptureRef(captureContext, cmd.drawCountBuffer);
    Capture(captureContext, cmd.drawCountOffset);
}

void Capture(CaptureContext& captureContext, const SetRenderPipelineCmd& cmd) {
    // Write SetRenderPipeline command parameters
    CaptureRef(captureContext, cmd.pipeline);
}

void Capture(CaptureContext& captureContext, const SetIndexBufferCmd& cmd) {
    // Write SetIndexBuffer command parameters
    CaptureRef(captureContext, cmd.buffer);
    Capture(captureContext, cmd.format);
    Capture(captureContext, cmd.offset);
    Capture(captureContext, cmd.size);
}

void Capture(CaptureContext& captureContext, const SetVertexBufferCmd& cmd) {
    // Write SetVertexBuffer command parameters
    Capture(captureContext, cmd.slot);
    CaptureRef(captureContext, cmd.buffer);
    Capture(captureContext, cmd.offset);
    Capture(captureContext, cmd.size);
}

void Capture(CaptureContext& captureContext, const SetBindGroupCmd& cmd) {
    // Write SetBindGroup command parameters
    Capture(captureContext, cmd.index);
    CaptureRef(captureContext, cmd.group);
    Capture(captureContext, cmd.dynamicOffsetCount);
}

void Capture(CaptureContext& captureContext,
             const InsertDebugMarkerCmd& cmd,
             CommandIterator& commands) {
    const char* label = commands.NextData<char>(cmd.length + 1);
    Capture(captureContext, std::string(label, cmd.length));
}

void Capture(CaptureContext& captureContext,
             const PushDebugGroupCmd& cmd,
             CommandIterator& commands) {
    // Write PushDebugGroup command parameters
    const char* label = commands.NextData<char>(cmd.length + 1);
    Capture(captureContext, std::string(label, cmd.length));
}

void Capture(CaptureContext& captureContext, const PopDebugGroupCmd& cmd) {
    // Write PopDebugGroup command parameters
}

void Capture(CaptureContext& captureContext, const WriteTimestampCmd& cmd) {
    // Write WriteTimestamp command parameters
    CaptureRef(captureContext, cmd.querySet);
    Capture(captureContext, cmd.queryIndex);
}

void Capture(CaptureContext& captureContext,
             const SetImmediateDataCmd& cmd,
             CommandIterator& commands) {
    // Write SetImmediateData command parameters
    DAWN_ASSERT(cmd.size > 0);
    uint8_t* value = nullptr;
    value = commands.NextData<uint8_t>(cmd.size);
    Capture(captureContext, cmd.offset);
    CaptureData(captureContext, value, cmd.size);
}

void Capture(CaptureContext& captureContext, const DispatchCmd& cmd) {
    // Write Dispatch command parameters
    Capture(captureContext, cmd.x);
    Capture(captureContext, cmd.y);
    Capture(captureContext, cmd.z);
}

void Capture(CaptureContext& captureContext, const DispatchIndirectCmd& cmd) {
    // Write DispatchIndirect command parameters
    CaptureRef(captureContext, cmd.indirectBuffer);
    Capture(captureContext, cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const SetComputePipelineCmd& cmd) {
    // Write SetComputePipeline command parameters
    CaptureRef(captureContext, cmd.pipeline);
}

void Capture(CaptureContext& captureContext, const CopyBufferToBufferCmd& cmd) {
    // Write CopyBufferToBuffer command parameters
    CaptureRef(captureContext, cmd.source);
    Capture(captureContext, cmd.sourceOffset);
    CaptureRef(captureContext, cmd.destination);
    Capture(captureContext, cmd.destinationOffset);
    Capture(captureContext, cmd.size);
}

void Capture(CaptureContext& captureContext, const CopyBufferToTextureCmd& cmd) {
    // Write CopyBufferToTexture command parameters
    Capture(captureContext, cmd.source);
    Capture(captureContext, cmd.destination);
    Capture(captureContext, cmd.copySize);
}

void Capture(CaptureContext& captureContext, const CopyTextureToBufferCmd& cmd) {
    // Write CopyTextureToBuffer command parameters
    Capture(captureContext, cmd.source);
    Capture(captureContext, cmd.destination);
    Capture(captureContext, cmd.copySize);
}

void Capture(CaptureContext& captureContext, const CopyTextureToTextureCmd& cmd) {
    // Write CopyTextureToTexture command parameters
    Capture(captureContext, cmd.source);
    Capture(captureContext, cmd.destination);
    Capture(captureContext, cmd.copySize);
}

void Capture(CaptureContext& captureContext, const ClearBufferCmd& cmd) {
    // Write ClearBuffer command parameters
    CaptureRef(captureContext, cmd.buffer);
    Capture(captureContext, cmd.offset);
    Capture(captureContext, cmd.size);
}

void Capture(CaptureContext& captureContext, const ResolveQuerySetCmd& cmd) {
    // Write ResolveQuerySet command parameters
    CaptureRef(captureContext, cmd.querySet);
    Capture(captureContext, cmd.firstQuery);
    Capture(captureContext, cmd.queryCount);
    CaptureRef(captureContext, cmd.destination);
    Capture(captureContext, cmd.destinationOffset);
}

void Capture(CaptureContext& captureContext, const WriteBufferCmd& cmd) {
    // Write WriteBuffer command parameters
    // TBD!
}

void Capture(CaptureContext& captureContext, const TimestampWrites& timestampWrites) {
    CaptureRef(captureContext, timestampWrites.querySet);
    Capture(captureContext, timestampWrites.beginningOfPassWriteIndex);
    Capture(captureContext, timestampWrites.endOfPassWriteIndex);
}

void Capture(CaptureContext& captureContext, const dawn::native::Color& color) {
    Capture(captureContext, color.r);
    Capture(captureContext, color.g);
    Capture(captureContext, color.b);
    Capture(captureContext, color.a);
}

void Capture(CaptureContext& captureContext, const RenderPassColorAttachmentInfo& attachment) {
    CaptureRef(captureContext, attachment.view);
    Capture(captureContext, attachment.depthSlice);
    CaptureRef(captureContext, attachment.resolveTarget);
    Capture(captureContext, attachment.loadOp);
    Capture(captureContext, attachment.storeOp);
    Capture(captureContext, attachment.clearColor);
}

void Capture(CaptureContext& captureContext,
             const RenderPassDepthStencilAttachmentInfo& attachment) {
    CaptureRef(captureContext, attachment.view);
    Capture(captureContext, attachment.depthLoadOp);
    Capture(captureContext, attachment.depthStoreOp);
    Capture(captureContext, attachment.stencilLoadOp);
    Capture(captureContext, attachment.stencilStoreOp);
    Capture(captureContext, attachment.clearDepth);
    Capture(captureContext, attachment.clearStencil);
    Capture(captureContext, attachment.depthReadOnly);
    Capture(captureContext, attachment.stencilReadOnly);
}

void Capture(CaptureContext& captureContext, const RenderPassStorageAttachmentInfo& attachment) {
    CaptureRef(captureContext, attachment.storage);
    Capture(captureContext, attachment.loadOp);
    Capture(captureContext, attachment.storeOp);
    Capture(captureContext, attachment.clearColor);
}

void Capture(CaptureContext& captureContext,
             const std::array<RenderPassStorageAttachmentInfo, kMaxPLSSlots>& attachments) {
    Capture(captureContext, attachments.size());
    for (const auto& attachment : attachments) {
        Capture(captureContext, attachment);
    }
}

void Capture(CaptureContext& captureContext,
             const BeginComputePassCmd& cmd,
             CommandIterator& commands) {
    Capture(captureContext, cmd.label);
    Capture(captureContext, cmd.timestampWrites);

    Command type;
    while (commands.NextCommandId(&type)) {
        Capture(captureContext, type);
        switch (type) {
            case Command::EndComputePass: {
                commands.NextCommand<EndComputePassCmd>();
                return;
            }

            case Command::Dispatch: {
                Capture(captureContext, *commands.NextCommand<DispatchCmd>());
                break;
            }

            case Command::DispatchIndirect: {
                Capture(captureContext, *commands.NextCommand<DispatchIndirectCmd>());
                break;
            }

            case Command::SetComputePipeline: {
                Capture(captureContext, *commands.NextCommand<SetComputePipelineCmd>());
                break;
            }

            case Command::SetBindGroup: {
                Capture(captureContext, *commands.NextCommand<SetBindGroupCmd>());
                break;
            }

            case Command::InsertDebugMarker: {
                Capture(captureContext, *commands.NextCommand<InsertDebugMarkerCmd>(), commands);
                break;
            }

            case Command::PopDebugGroup: {
                Capture(captureContext, *commands.NextCommand<PopDebugGroupCmd>());
                break;
            }

            case Command::PushDebugGroup: {
                Capture(captureContext, *commands.NextCommand<PushDebugGroupCmd>(), commands);
                break;
            }

            case Command::WriteTimestamp: {
                Capture(captureContext, *commands.NextCommand<WriteTimestampCmd>());
                break;
            }

            case Command::SetImmediateData: {
                Capture(captureContext, *commands.NextCommand<SetImmediateDataCmd>(), commands);
                break;
            }

            default: {
                DAWN_UNREACHABLE();
                break;
            }
        }
    }
}

void Capture(CaptureContext& captureContext,
             const BeginRenderPassCmd& cmd,
             CommandIterator& commands) {
    Capture(captureContext, cmd.label);
    Capture(captureContext, cmd.timestampWrites);
    //CaptureRef(captureContext, cmd.occlusionQuerySet);
    //Capture(captureContext, cmd.storageAttachments);
    //Capture(captureContext, cmd.depthStencilAttachment);

    // TBD: write number of attachments or mask
    for (auto i : cmd.attachmentState->GetColorAttachmentsMask()) {
        const auto& attachmentInfo = cmd.colorAttachments[i];
        Capture(captureContext, attachmentInfo);
    }

    Command type;
    while (commands.NextCommandId(&type)) {
        Capture(captureContext, type);
        switch (type) {
            case Command::EndRenderPass: {
                return;
            }

            case Command::Draw: {
                Capture(captureContext, *commands.NextCommand<DrawCmd>());
                break;
            }

            case Command::DrawIndexed: {
                Capture(captureContext, *commands.NextCommand<DrawIndexedCmd>());
                break;
            }

            case Command::DrawIndirect: {
                Capture(captureContext, *commands.NextCommand<DrawIndirectCmd>());
                break;
            }

            case Command::DrawIndexedIndirect: {
                Capture(captureContext, *commands.NextCommand<DrawIndexedIndirectCmd>());
                break;
            }

            case Command::MultiDrawIndirect: {
                Capture(captureContext, *commands.NextCommand<MultiDrawIndirectCmd>());
                break;
            }

            case Command::MultiDrawIndexedIndirect: {
                Capture(captureContext, *commands.NextCommand<MultiDrawIndexedIndirectCmd>());
                break;
            }

            case Command::SetRenderPipeline: {
                Capture(captureContext, *commands.NextCommand<SetRenderPipelineCmd>());
                break;
            }

            case Command::SetIndexBuffer: {
                Capture(captureContext, *commands.NextCommand<SetIndexBufferCmd>());
                break;
            }

            case Command::SetVertexBuffer: {
                Capture(captureContext, *commands.NextCommand<SetVertexBufferCmd>());
                break;
            }

            case Command::SetBindGroup: {
                Capture(captureContext, *commands.NextCommand<SetBindGroupCmd>());
                break;
            }

            case Command::InsertDebugMarker: {
                Capture(captureContext, *commands.NextCommand<InsertDebugMarkerCmd>(), commands);
                break;
            }

            case Command::PopDebugGroup: {
                Capture(captureContext, *commands.NextCommand<PopDebugGroupCmd>());
                break;
            }

            case Command::PushDebugGroup: {
                Capture(captureContext, *commands.NextCommand<PushDebugGroupCmd>(), commands);
                break;
            }

            case Command::WriteTimestamp: {
                Capture(captureContext, *commands.NextCommand<WriteTimestampCmd>());
                break;
            }

            case Command::SetImmediateData: {
                Capture(captureContext, *commands.NextCommand<SetImmediateDataCmd>(), commands);
                break;
            }

            default: {
                DAWN_UNREACHABLE();
                break;
            }
        }
    }
}
    */

}  // anonymous namespace

MaybeError CommandBuffer::AddReferenced(CaptureContext& captureContext) const {
    const auto& resourceUsages = GetResourceUsages();
    for (auto buffer : resourceUsages.topLevelBuffers) {
        DAWN_TRY(captureContext.AddResource(buffer));
    }
    for (auto texture : resourceUsages.topLevelTextures) {
        DAWN_TRY(captureContext.AddResource(texture));
    }
    for (const auto& pass : resourceUsages.renderPasses) {
        for (auto buffer : pass.buffers) {
            DAWN_TRY(captureContext.AddResource(buffer));
        }
    }
    for (const auto& pass : resourceUsages.computePasses) {
        for (auto buffer : pass.referencedBuffers) {
            DAWN_TRY(captureContext.AddResource(buffer));
        }
    }
    return {};
}

MaybeError CommandBuffer::CaptureCreationParameters(CaptureContext& captureContext) {
    CommandIterator& commands = *GetCommandIteratorForTesting();

    Command type;
    while (commands.NextCommandId(&type)) {
        switch (type) {
            case Command::CopyBufferToBuffer: {
                Capture(captureContext, *commands.NextCommand<CopyBufferToBufferCmd>());
                break;
            }
            case Command::CopyBufferToTexture: {
                Capture(captureContext, *commands.NextCommand<CopyBufferToTextureCmd>());
                break;
            }
            case Command::CopyTextureToBuffer: {
                Capture(captureContext, *commands.NextCommand<CopyTextureToBufferCmd>());
                break;
            }
            case Command::CopyTextureToTexture: {
                Capture(captureContext, *commands.NextCommand<CopyTextureToTextureCmd>());
                break;
            }
            default:
                DAWN_UNREACHABLE();
        }
    }
    Serialize(captureContext, schema::EncoderCommand::End);
    return {};
}

MaybeError CommandBuffer::CaptureContentImpl(CaptureContext& captureContext, schema::ObjectId id) {
    return {};
}

WGPUCommandBuffer CommandBuffer::Encode(bool isCapturing) {
    auto& wgpu = ToBackend(GetDevice())->wgpu;

    // TODO(crbug.com/413053623): Use stored command encoder descriptor
    WGPUCommandEncoder innerEncoder =
        wgpu.deviceCreateCommandEncoder(ToBackend(GetDevice())->GetInnerHandle(), nullptr);

    size_t nextComputePassNumber = 0;

    Command type;
    while (mCommands.NextCommandId(&type)) {
        switch (type) {
            case Command::BeginComputePass: {
                BeginComputePassCmd* cmd = mCommands.NextCommand<BeginComputePassCmd>();
                EncodeComputePass(wgpu, innerEncoder, mCommands, cmd,
                                  GetResourceUsages().computePasses[nextComputePassNumber],
                                  isCapturing);
                ++nextComputePassNumber;
                break;
            }
            case Command::BeginRenderPass: {
                auto cmd = mCommands.NextCommand<BeginRenderPassCmd>();
                EncodeRenderPass(wgpu, innerEncoder, mCommands, cmd);
                break;
            }
            case Command::CopyBufferToBuffer: {
                auto copy = mCommands.NextCommand<CopyBufferToBufferCmd>();
                wgpu.commandEncoderCopyBufferToBuffer(
                    innerEncoder, ToBackend(copy->source)->GetInnerHandle(), copy->sourceOffset,
                    ToBackend(copy->destination)->GetInnerHandle(), copy->destinationOffset,
                    copy->size);
                if (!isCapturing) {
                    ToBackend(copy->destination)->MarkAsNeedsCapture();
                }
                break;
            }
            case Command::CopyBufferToTexture: {
                auto cmd = mCommands.NextCommand<CopyBufferToTextureCmd>();
                WGPUTexelCopyBufferInfo source = ToWGPU(cmd->source);
                WGPUTexelCopyTextureInfo destination = ToWGPU(cmd->destination);
                WGPUExtent3D size = ToWGPU(cmd->copySize);
                wgpu.commandEncoderCopyBufferToTexture(innerEncoder, &source, &destination, &size);
                if (!isCapturing) {
                    ToBackend(cmd->destination.texture)->MarkAsNeedsCapture();
                }
                break;
            }
            case Command::CopyTextureToBuffer: {
                auto cmd = mCommands.NextCommand<CopyTextureToBufferCmd>();
                WGPUTexelCopyTextureInfo source = ToWGPU(cmd->source);
                WGPUTexelCopyBufferInfo destination = ToWGPU(cmd->destination);
                WGPUExtent3D size = ToWGPU(cmd->copySize);
                wgpu.commandEncoderCopyTextureToBuffer(innerEncoder, &source, &destination, &size);
                if (!isCapturing) {
                    ToBackend(cmd->destination.buffer)->MarkAsNeedsCapture();
                }
                break;
            }
            case Command::CopyTextureToTexture: {
                auto cmd = mCommands.NextCommand<CopyTextureToTextureCmd>();
                WGPUTexelCopyTextureInfo source = ToWGPU(cmd->source);
                WGPUTexelCopyTextureInfo destination = ToWGPU(cmd->destination);
                WGPUExtent3D size = ToWGPU(cmd->copySize);
                wgpu.commandEncoderCopyTextureToTexture(innerEncoder, &source, &destination, &size);
                if (!isCapturing) {
                    ToBackend(cmd->destination.texture)->MarkAsNeedsCapture();
                }
                break;
            }
            case Command::ClearBuffer: {
                auto cmd = mCommands.NextCommand<ClearBufferCmd>();
                wgpu.commandEncoderClearBuffer(
                    innerEncoder, ToBackend(cmd->buffer)->GetInnerHandle(), cmd->offset, cmd->size);
                break;
            }
            case Command::ResolveQuerySet: {
                auto cmd = mCommands.NextCommand<ResolveQuerySetCmd>();
                // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented for
                // QuerySetWGPU
                wgpu.commandEncoderResolveQuerySet(
                    innerEncoder, nullptr /*ToBackend(cmd->querySet)->GetInnerHandle()*/,
                    cmd->firstQuery, cmd->queryCount, ToBackend(cmd->destination)->GetInnerHandle(),
                    cmd->destinationOffset);
                break;
            }
            case Command::WriteTimestamp: {
                auto cmd = mCommands.NextCommand<WriteTimestampCmd>();
                // TODO(crbug.com/440123094): remove nullptr when GetInnerHandle is implemented for
                // QuerySetWGPU
                wgpu.commandEncoderWriteTimestamp(
                    innerEncoder, nullptr /*ToBackend(cmd->querySet)->GetInnerHandle()*/,
                    cmd->queryIndex);
                break;
            }
            case Command::InsertDebugMarker: {
                auto cmd = mCommands.NextCommand<InsertDebugMarkerCmd>();
                char* label = mCommands.NextData<char>(cmd->length + 1);
                wgpu.commandEncoderInsertDebugMarker(innerEncoder, {label, cmd->length});
                break;
            }
            case Command::PopDebugGroup: {
                mCommands.NextCommand<PopDebugGroupCmd>();
                wgpu.commandEncoderPopDebugGroup(innerEncoder);
                break;
            }
            case Command::PushDebugGroup: {
                auto cmd = mCommands.NextCommand<PushDebugGroupCmd>();
                char* label = mCommands.NextData<char>(cmd->length + 1);
                wgpu.commandEncoderPushDebugGroup(innerEncoder, {label, cmd->length});
                break;
            }
            case Command::WriteBuffer: {
                auto cmd = mCommands.NextCommand<WriteBufferCmd>();
                auto data = mCommands.NextData<uint8_t>(cmd->size);
                wgpu.commandEncoderWriteBuffer(innerEncoder,
                                               ToBackend(cmd->buffer)->GetInnerHandle(),
                                               cmd->offset, data, cmd->size);
                break;
            }
            default:
                DAWN_UNREACHABLE();
        }
    }

    // TODO(crbug.com/413053623): Store WGPUCommandBufferDescriptor and assign here.
    WGPUCommandBuffer result = wgpu.commandEncoderFinish(innerEncoder, nullptr);
    wgpu.commandEncoderRelease(innerEncoder);
    return result;
}

}  // namespace dawn::native::webgpu
