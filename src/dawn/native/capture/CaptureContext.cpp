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

#include <concepts>
#include <vector>

#include "src/dawn/native/CommandBuffer.h"
#include "src/dawn/native/Commands.h"
#include "src/dawn/native/capture/Serialization.h"

namespace dawn::native {

namespace {

template <typename T>
concept IsInt32 = std::is_same_v<T, int32_t>;

template <typename T>
concept IsUint32 = std::is_same_v<T, uint32_t>;

template <typename T>
concept IsUint64 = std::is_same_v<T, uint64_t>;

template <typename T>
concept IsSizeT = std::is_same_v<T, size_t>;

template <typename T>
concept IsFloat = std::is_same_v<T, float>;

template <typename T>
concept IsDouble = std::is_same_v<T, double>;

template <typename T>
concept IsBool = std::is_same_v<T, bool>;

void Capture(CaptureContext& captureContext, IsDouble auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsFloat auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsInt32 auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsUint32 auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsUint64 auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsSizeT auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

void Capture(CaptureContext& captureContext, IsBool auto value) {
    captureContext.WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
}

// Captures a stream of bytes preceeded by a length.
void CaptureData(CaptureContext& captureContext, const void* data, size_t size) {
    Capture(captureContext, size);
    captureContext.WriteBytes(data, size);
}

void Capture(CaptureContext& captureContext, wgpu::LoadOp value) {
    Capture(captureContext, static_cast<uint32_t>(value));
}

void Capture(CaptureContext& captureContext, wgpu::StoreOp value) {
    Capture(captureContext, static_cast<uint32_t>(value));
}

void Capture(CaptureContext& captureContext, Aspect aspect) {
    Capture(captureContext, static_cast<uint32_t>(aspect));
}

void Capture(CaptureContext& captureContext, Command command) {
    Capture(captureContext, static_cast<uint32_t>(command));
}

void Capture(CaptureContext& captureContext, BindGroupIndex index) {
    Capture(captureContext, static_cast<uint32_t>(index));
}

void Capture(CaptureContext& captureContext, VertexBufferSlot slot) {
    Capture(captureContext, static_cast<uint32_t>(slot));
}

void Capture(CaptureContext& captureContext, wgpu::IndexFormat format) {
    Capture(captureContext, static_cast<uint32_t>(format));
}

void Capture(CaptureContext& captureContext, const std::string& value) {
    Capture(captureContext, value.length());
    CaptureData(captureContext, value.c_str(), value.length());
}

template <typename T>
void CaptureRef(CaptureContext& captureContext, const Ref<T>& ref) {
    Capture(captureContext, static_cast<uint64_t>(captureContext.GetId(ref.Get())));
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
    Capture(captureContext, buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<TextureBase*>& textures) {
    Capture(captureContext, textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<ExternalTextureBase*>& textures) {
    Capture(captureContext, textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<QuerySetBase*>& querySets) {
    Capture(captureContext, querySets.size());
    for (const auto querySet : querySets) {
        Capture(captureContext, querySet);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<BufferBase*> buffers) {
    Capture(captureContext, buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<TextureBase*> textures) {
    Capture(captureContext, textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext,
             const absl::flat_hash_set<ExternalTextureBase*> textures) {
    Capture(captureContext, textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<QuerySetBase*> querySets) {
    Capture(captureContext, querySets.size());
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
    Capture(captureContext, renderPassUsages.size());
    for (const auto& usage : renderPassUsages) {
        Capture(captureContext, usage);
    }
}

void Capture(CaptureContext& captureContext, const ComputePassResourceUsage& usage) {
    Capture(captureContext, usage.dispatchUsages.size());
    for (const auto& resourceUsage : usage.dispatchUsages) {
        Capture(captureContext, resourceUsage);
    }
    Capture(captureContext, usage.referencedBuffers);
    Capture(captureContext, usage.referencedTextures);
    Capture(captureContext, usage.referencedExternalTextures);
}

void Capture(CaptureContext& captureContext, const ComputePassUsages& computePassUsages) {
    Capture(captureContext, computePassUsages.size());
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
    Capture(captureContext, extent.width);
    Capture(captureContext, extent.height);
    Capture(captureContext, extent.depthOrArrayLayers);
}

void Capture(CaptureContext& captureContext, const Origin3D& origin) {
    Capture(captureContext, origin.x);
    Capture(captureContext, origin.y);
    Capture(captureContext, origin.z);
}

void Capture(CaptureContext& captureContext, const BufferCopy& copy) {
    CaptureRef(captureContext, copy.buffer);
    Capture(captureContext, copy.offset);
    Capture(captureContext, copy.bytesPerRow);
    Capture(captureContext, copy.rowsPerImage);
}

void Capture(CaptureContext& captureContext, const TextureCopy& copy) {
    CaptureRef(captureContext, copy.texture);
    Capture(captureContext, copy.mipLevel);
    Capture(captureContext, copy.origin);
    Capture(captureContext, copy.aspect);
}

void Capture(CaptureContext& captureContext, const DrawCmd& cmd) {
    // Write Draw command parameters
    Capture(captureContext, cmd.vertexCount);
    Capture(captureContext, cmd.instanceCount);
    Capture(captureContext, cmd.firstVertex);
    Capture(captureContext, cmd.firstInstance);
}

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
    CaptureRef(captureContext, cmd.occlusionQuerySet);
    Capture(captureContext, cmd.storageAttachments);
    Capture(captureContext, cmd.depthStencilAttachment);

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

void Capture(CaptureContext& captureContext, CommandIterator& commands) {
    Command type;
    while (commands.NextCommandId(&type)) {
        Capture(captureContext, type);
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

void AddReferenced(CaptureContext& captureContext, const BufferBase* buffer) {}

template <>
void CaptureContent<BufferBase>(CaptureContext& captureContext, const BufferBase* buffer) {
    // Capture buffer contents.
    // buffer->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const TextureBase* texture) {
}

template <>
void CaptureContent<TextureBase>(CaptureContext& captureContext, const TextureBase* texture) {
    // Capture texture contents.
    // texture->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const ExternalTextureBase* texture) {
}

template <>
void CaptureContent<ExternalTextureBase>(CaptureContext& captureContext,
                                         const ExternalTextureBase* texture) {
    // Capture texture contents.
    // texture->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const QuerySetBase* querySet) {
}

template <>
void CaptureContent<QuerySetBase>(CaptureContext& captureContext, const QuerySetBase* querySet) {
    // Capture query set contents.
    // querySet->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const ComputePipelineBase* pipeline) {
    // TDB: Add pipeline descriptor resources.
}

template <>
void CaptureContent<ComputePipelineBase>(CaptureContext& captureContext, const ComputePipelineBase* pipeline) {
    // Capture pipeline
    // pipeline->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const BindGroupBase* pipeline) {
    // TDB: Add bindgroup entry resources.
}

template <>
void CaptureContent<BindGroupBase>(CaptureContext& captureContext, const BindGroupBase* bindGroup) {
    // Capture bindgroup
    // bindGroup->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const TextureViewBase* view) {
    captureContext.AddResource(view->GetTexture());
}

template <>
void CaptureContent<TextureViewBase>(CaptureContext& captureContext, const TextureViewBase* view) {
    // Capture texture view
    // view->Capture(captureContext);
}

void AddReferenced(CaptureContext& captureContext, const RenderPipelineBase* pipeline) {
    // TDB: Add pipeline descriptor resources.
}

template <>
void CaptureContent<RenderPipelineBase>(CaptureContext& captureContext, const RenderPipelineBase* pipeline) {
    // Capture render pipeline
    // pipeline->Capture(captureContext);
}

CaptureContext::CaptureContext(std::ostream& outputStream) : mOutputStream(outputStream) {
    WriteBytes(reinterpret_cast<const char*>(capture::kMagicNumber), sizeof(capture::kMagicNumber));
    Capture(*this, static_cast<uint32_t>(capture::kVersionNumber));
}

CaptureContext::~CaptureContext() = default;

void CaptureContext::WriteBytes(const void* data, size_t size) {
    mOutputStream.write(reinterpret_cast<const char*>(data), size);
}

void CaptureContext::CaptureCommandBuffer(uint32_t commandCount,
                                          CommandBufferBase* const* commands) {
    Capture(*this, commandCount);
    for (uint32_t i = 0; i < commandCount; ++i) {
        Capture(*this, commands[i]);
    }
    // Write out all the resources needed for these command buffers.
    for (const auto cap : mToBeCaptured) {
        cap->Capture(*this);
    }
    mToBeCaptured.clear();
}

}  // namespace dawn::native
