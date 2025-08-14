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

#include "src/dawn/native/capture/CaptureContext.h"

#include <vector>

#include "src/dawn/native/CommandBuffer.h"
#include "src/dawn/native/capture/Serialization.h"

namespace dawn::native {

CaptureContext::CaptureContext(std::ostream& outputStream) : mOutputStream(outputStream) {
    mOutputStream.write(reinterpret_cast<const char*>(capture::kMagicNumber),
                        sizeof(capture::kMagicNumber));
    Write(static_cast<uint32_t>(capture::kVersionNumber));
}

CaptureContext::~CaptureContext() = default;

void CaptureContext::Write(IsFloat auto value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::Write(IsInt32 auto value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::Write(IsUint32 auto value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::Write(IsUint64 auto value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::Write(IsSizeT auto value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::Write(Aspect aspect) {
    Write(static_cast<uint32_t>(aspect));
}

void CaptureContext::Write(Command command) {
    Write(static_cast<uint32_t>(command));
}

void CaptureContext::Write(BindGroupIndex index) {
    Write(static_cast<uint32_t>(index));
}

void CaptureContext::Write(VertexBufferSlot slot) {
    Write(static_cast<uint32_t>(slot));
}

void CaptureContext::Write(wgpu::IndexFormat format) {
    Write(static_cast<uint32_t>(format));
}

void CaptureContext::Write(const std::string& value) {
    Write(value.length());
    mOutputStream.write(value.c_str(), value.length());
}

void CaptureContext::WriteData(const void* data, size_t size) {
    Write(size);
    mOutputStream.write(reinterpret_cast<const char*>(data), size);
}

void CaptureContent(CaptureContext& captureContext, const BufferBase* buffer) {
    // Capture buffer contents.
    // buffer->Capture(captureContext);
}

void CaptureContent(CaptureContext& captureContext, const TextureBase* texture) {
    // Capture texture contents.
    // texture->Capture(captureContext);
}

void CaptureContent(CaptureContext& captureContext, const ExternalTextureBase* texture) {
    // Capture texture contents.
    // texture->Capture(captureContext);
}

void CaptureContent(CaptureContext& captureContext, const QuerySetBase* querySet) {
    // Capture query set contents.
    // querySet->Capture(captureContext);
}

namespace {

void Capture(CaptureContext& captureContext, const std::string& str) {
    captureContext.Write(str);
}

void Capture(CaptureContext& captureContext, const BufferBase* buffer) {
    captureContext.AddResource(buffer);
}

void Capture(CaptureContext& captureContext, const TextureBase* texture) {
    captureContext.AddResource(texture);
}

void Capture(CaptureContext& captureContext, const ExternalTextureBase* texture) {
    captureContext.AddResource(texture);
}

void Capture(CaptureContext& captureContext, const QuerySetBase* querySet) {
    captureContext.AddResource(querySet);
}

void Capture(CaptureContext& captureContext, const std::vector<BufferBase*>& buffers) {
    captureContext.Write(buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<TextureBase*>& textures) {
    captureContext.Write(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<ExternalTextureBase*>& textures) {
    captureContext.Write(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<QuerySetBase*>& querySets) {
    captureContext.Write(querySets.size());
    for (const auto querySet : querySets) {
        Capture(captureContext, querySet);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<BufferBase*> buffers) {
    captureContext.Write(buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<TextureBase*> textures) {
    captureContext.Write(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext,
             const absl::flat_hash_set<ExternalTextureBase*> textures) {
    captureContext.Write(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<QuerySetBase*> querySets) {
    captureContext.Write(querySets.size());
    for (const auto* querySet : querySets) {
        Capture(captureContext, querySet);
    }
}

void Capture(CaptureContext& captureContext, const SyncScopeResourceUsage& usage) {
    Capture(captureContext, usage.buffers);
    Capture(captureContext, usage.textures);
    Capture(captureContext, usage.externalTextures);
}

void Capture(CaptureContext& captureContext, const RenderPassResourceUsage& usage) {
    Capture(captureContext, static_cast<const SyncScopeResourceUsage&>(usage));
    Capture(captureContext, usage.querySets);
    // Capture(captureContext, usage.queryAvailabilities));
}

void Capture(CaptureContext& captureContext, const RenderPassUsages& renderPassUsages) {
    captureContext.Write(renderPassUsages.size());
    for (const auto& usage : renderPassUsages) {
        Capture(captureContext, usage);
    }
}

void Capture(CaptureContext& captureContext, const ComputePassResourceUsage& usage) {
    captureContext.Write(usage.dispatchUsages.size());
    for (const auto& resourceUsage : usage.dispatchUsages) {
        Capture(captureContext, resourceUsage);
    }
    Capture(captureContext, usage.referencedBuffers);
    Capture(captureContext, usage.referencedTextures);
    Capture(captureContext, usage.referencedExternalTextures);
}

void Capture(CaptureContext& captureContext, const ComputePassUsages& computePassUsages) {
    captureContext.Write(computePassUsages.size());
    for (const auto& usage : computePassUsages) {
        Capture(captureContext, usage);
    }
}

void Capture(CaptureContext& captureContext, const CommandBufferResourceUsage& resourceUsages) {
    Capture(captureContext, resourceUsages.renderPasses);
    Capture(captureContext, resourceUsages.computePasses);
    Capture(captureContext, resourceUsages.topLevelBuffers);
    Capture(captureContext, resourceUsages.topLevelTextures);
    Capture(captureContext, resourceUsages.usedQuerySets);
}

void Capture(CaptureContext& captureContext,
             const std::vector<IndirectDrawMetadata>& indirectDrawMetaData) {}

void Capture(CaptureContext& captureContext, const Extent3D& extent) {
    captureContext.Write(extent.width);
    captureContext.Write(extent.height);
    captureContext.Write(extent.depthOrArrayLayers);
}

void Capture(CaptureContext& captureContext, const Origin3D& origin) {
    captureContext.Write(origin.x);
    captureContext.Write(origin.y);
    captureContext.Write(origin.z);
}

void Capture(CaptureContext& captureContext, const BufferCopy& copy) {
    captureContext.WriteRef(copy.buffer);
    captureContext.Write(copy.offset);
    captureContext.Write(copy.bytesPerRow);
    captureContext.Write(copy.rowsPerImage);
}

void Capture(CaptureContext& captureContext, const TextureCopy& copy) {
    captureContext.WriteRef(copy.texture);
    captureContext.Write(copy.mipLevel);
    Capture(captureContext, copy.origin);
    captureContext.Write(copy.aspect);
}

void Capture(CaptureContext& captureContext, const DrawCmd& cmd) {
    // Write Draw command parameters
    captureContext.Write(cmd.vertexCount);
    captureContext.Write(cmd.instanceCount);
    captureContext.Write(cmd.firstVertex);
    captureContext.Write(cmd.firstInstance);
}

void Capture(CaptureContext& captureContext, const DrawIndexedCmd& cmd) {
    // Write DrawIndexed command parameters
    captureContext.Write(cmd.indexCount);
    captureContext.Write(cmd.instanceCount);
    captureContext.Write(cmd.firstIndex);
    captureContext.Write(cmd.baseVertex);
    captureContext.Write(cmd.firstInstance);
}

void Capture(CaptureContext& captureContext, const DrawIndirectCmd& cmd) {
    // Write DrawIndirect command parameters
    captureContext.WriteRef(cmd.indirectBuffer);
    captureContext.Write(cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const DrawIndexedIndirectCmd& cmd) {
    // Write DrawIndexedIndirect command parameters
    captureContext.WriteRef(cmd.indirectBuffer);
    captureContext.Write(cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const MultiDrawIndirectCmd& cmd) {
    // Write MultiDrawIndirect command parameters
    captureContext.WriteRef(cmd.indirectBuffer);
    captureContext.Write(cmd.indirectOffset);
    captureContext.Write(cmd.maxDrawCount);
    captureContext.WriteRef(cmd.drawCountBuffer);
    captureContext.Write(cmd.drawCountOffset);
}

void Capture(CaptureContext& captureContext, const MultiDrawIndexedIndirectCmd& cmd) {
    // Write MultiDrawIndexedIndirect command parameters
    captureContext.WriteRef(cmd.indirectBuffer);
    captureContext.Write(cmd.indirectOffset);
    captureContext.Write(cmd.maxDrawCount);
    captureContext.WriteRef(cmd.drawCountBuffer);
    captureContext.Write(cmd.drawCountOffset);
}

void Capture(CaptureContext& captureContext, const SetRenderPipelineCmd& cmd) {
    // Write SetRenderPipeline command parameters
    captureContext.WriteRef(cmd.pipeline);
}

void Capture(CaptureContext& captureContext, const SetIndexBufferCmd& cmd) {
    // Write SetIndexBuffer command parameters
    captureContext.WriteRef(cmd.buffer);
    captureContext.Write(cmd.format);
    captureContext.Write(cmd.offset);
    captureContext.Write(cmd.size);
}

void Capture(CaptureContext& captureContext, const SetVertexBufferCmd& cmd) {
    // Write SetVertexBuffer command parameters
    captureContext.Write(cmd.slot);
    captureContext.WriteRef(cmd.buffer);
    captureContext.Write(cmd.offset);
    captureContext.Write(cmd.size);
}

void Capture(CaptureContext& captureContext, const SetBindGroupCmd& cmd) {
    // Write SetBindGroup command parameters
    captureContext.Write(cmd.index);
    captureContext.WriteRef(cmd.group);
    captureContext.Write(cmd.dynamicOffsetCount);
}

void Capture(CaptureContext& captureContext, const InsertDebugMarkerCmd& cmd, CommandIterator& commands) {
    const char* label = commands.NextData<char>(cmd.length + 1);
    Capture(captureContext, std::string(label, cmd.length));
}

void Capture(CaptureContext& captureContext, const PushDebugGroupCmd& cmd, CommandIterator& commands) {
    // Write PushDebugGroup command parameters
    const char* label = commands.NextData<char>(cmd.length + 1);
    Capture(captureContext, std::string(label, cmd.length));
}

void Capture(CaptureContext& captureContext, const PopDebugGroupCmd& cmd) {
    // Write PopDebugGroup command parameters
}

void Capture(CaptureContext& captureContext, const WriteTimestampCmd& cmd) {
    // Write WriteTimestamp command parameters
    captureContext.WriteRef(cmd.querySet);
    captureContext.Write(cmd.queryIndex);
}

void Capture(CaptureContext& captureContext, const SetImmediateDataCmd& cmd, CommandIterator& commands) {
    // Write SetImmediateData command parameters
    DAWN_ASSERT(cmd.size > 0);
    uint8_t* value = nullptr;
    value = commands.NextData<uint8_t>(cmd.size);
    captureContext.Write(cmd.offset);
    captureContext.WriteData(value, cmd.size);
}

void Capture(CaptureContext& captureContext, const DispatchCmd& cmd) {
    // Write Dispatch command parameters
    captureContext.Write(cmd.x);
    captureContext.Write(cmd.y);
    captureContext.Write(cmd.z);
}

void Capture(CaptureContext& captureContext, const DispatchIndirectCmd& cmd) {
    // Write DispatchIndirect command parameters
    captureContext.WriteRef(cmd.indirectBuffer);
    captureContext.Write(cmd.indirectOffset);
}

void Capture(CaptureContext& captureContext, const SetComputePipelineCmd& cmd) {
    // Write SetComputePipeline command parameters
    captureContext.WriteRef(cmd.pipeline);
}

void Capture(CaptureContext& captureContext, const CopyBufferToBufferCmd& cmd) {
    // Write CopyBufferToBuffer command parameters
    captureContext.WriteRef(cmd.source);
    captureContext.Write(cmd.sourceOffset);
    captureContext.WriteRef(cmd.destination);
    captureContext.Write(cmd.destinationOffset);
    captureContext.Write(cmd.size);
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
    captureContext.WriteRef(cmd.buffer);
    captureContext.Write(cmd.offset);
    captureContext.Write(cmd.size);
}

void Capture(CaptureContext& captureContext, const ResolveQuerySetCmd& cmd) {
    // Write ResolveQuerySet command parameters
    captureContext.WriteRef(cmd.querySet);
    captureContext.Write(cmd.firstQuery);
    captureContext.Write(cmd.queryCount);
    captureContext.WriteRef(cmd.destination);
    captureContext.Write(cmd.destinationOffset);
}

void Capture(CaptureContext& captureContext, const WriteBufferCmd& cmd) {
    // Write WriteBuffer command parameters
    // TBD!
}

void Capture(CaptureContext& captureContext,
             const BeginComputePassCmd& cmd,
             CommandIterator& commands) {
    Command type;
    while (commands.NextCommandId(&type)) {
        captureContext.Write(type);
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
    Command type;
    while (commands.NextCommandId(&type)) {
        captureContext.Write(type);
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

void Capture(CaptureContext& captureContext, CommandIterator& commands) {
    Command type;
    while (commands.NextCommandId(&type)) {
        captureContext.Write(type);
        switch (type) {
            case Command::BeginComputePass: {
                Capture(captureContext, *commands.NextCommand<BeginComputePassCmd>(), commands);
                break;
            }
            case Command::BeginRenderPass: {
                Capture(captureContext, *commands.NextCommand<BeginRenderPassCmd>(), commands);
                break;
            }
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
            case Command::ClearBuffer: {
                Capture(captureContext, *commands.NextCommand<ClearBufferCmd>());
                break;
            }
            case Command::ResolveQuerySet: {
                Capture(captureContext, *commands.NextCommand<ResolveQuerySetCmd>());
                break;
            }
            case Command::WriteTimestamp: {
                Capture(captureContext, *commands.NextCommand<WriteTimestampCmd>());
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
            case Command::WriteBuffer: {
                Capture(captureContext, *commands.NextCommand<WriteBufferCmd>());
                break;
            }
            default:
                DAWN_UNREACHABLE();
        }
    }
}

void Capture(CaptureContext& captureContext, CommandBufferBase* commandBuffer) {
    Capture(captureContext, commandBuffer->GetEncoderLabel());
    Capture(captureContext, commandBuffer->GetResourceUsages());
    Capture(captureContext, commandBuffer->GetIndirectDrawMetadata());
    Capture(captureContext, *commandBuffer->GetCommandIteratorForTesting());
}

}  // namespace

void CaptureContext::CaptureCommandBuffer(uint32_t commandCount,
                                          CommandBufferBase* const* commands) {
    Write(commandCount);
    for (uint32_t i = 0; i < commandCount; ++i) {
        Capture(*this, commands[i]);
    }
}

}  // namespace dawn::native
