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
    WriteU32(capture::kVersionNumber);
}

CaptureContext::~CaptureContext() = default;

void CaptureContext::WriteF32(float value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteI32(int32_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteU32(uint32_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteU64(uint64_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteSizeT(size_t value) {
    mOutputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void CaptureContext::WriteString(const std::string& value) {
    WriteU64(value.length());
    mOutputStream.write(value.c_str(), value.length());
}

void CaptureContext::WriteData(const void* data, size_t size) {
    WriteU64(size);
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
    captureContext.WriteString(str);
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
    captureContext.WriteSizeT(buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<TextureBase*>& textures) {
    captureContext.WriteSizeT(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<ExternalTextureBase*>& textures) {
    captureContext.WriteSizeT(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const std::vector<QuerySetBase*>& querySets) {
    captureContext.WriteSizeT(querySets.size());
    for (const auto querySet : querySets) {
        Capture(captureContext, querySet);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<BufferBase*> buffers) {
    captureContext.WriteSizeT(buffers.size());
    for (const auto buffer : buffers) {
        Capture(captureContext, buffer);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<TextureBase*> textures) {
    captureContext.WriteSizeT(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext,
             const absl::flat_hash_set<ExternalTextureBase*> textures) {
    captureContext.WriteSizeT(textures.size());
    for (const auto texture : textures) {
        Capture(captureContext, texture);
    }
}

void Capture(CaptureContext& captureContext, const absl::flat_hash_set<QuerySetBase*> querySets) {
    captureContext.WriteSizeT(querySets.size());
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
    captureContext.WriteSizeT(renderPassUsages.size());
    for (const auto& usage : renderPassUsages) {
        Capture(captureContext, usage);
    }
}

void Capture(CaptureContext& captureContext, const ComputePassResourceUsage& usage) {
    captureContext.WriteSizeT(usage.dispatchUsages.size());
    for (const auto& resourceUsage : usage.dispatchUsages) {
        Capture(captureContext, resourceUsage);
    }
    Capture(captureContext, usage.referencedBuffers);
    Capture(captureContext, usage.referencedTextures);
    Capture(captureContext, usage.referencedExternalTextures);
}

void Capture(CaptureContext& captureContext, const ComputePassUsages& computePassUsages) {
    captureContext.WriteSizeT(computePassUsages.size());
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

void Capture(CaptureContext& captureContext,
             const BeginComputePassCmd* cmd,
             CommandIterator& commands) {
    Command type;
    while (commands.NextCommandId(&type)) {
        switch (type) {
            case Command::EndComputePass: {
                commands.NextCommand<EndComputePassCmd>();
                return;
            }

            case Command::Dispatch: {
                /* DispatchCmd* dispatch = */ commands.NextCommand<DispatchCmd>();
                break;
            }

            case Command::DispatchIndirect: {
                /* DispatchIndirectCmd* dispatch = */ commands.NextCommand<DispatchIndirectCmd>();
                break;
            }

            case Command::SetComputePipeline: {
                /* SetComputePipelineCmd* cmd = */ commands.NextCommand<SetComputePipelineCmd>();
                break;
            }

            case Command::SetBindGroup: {
                /* SetBindGroupCmd* cmd = */ commands.NextCommand<SetBindGroupCmd>();
                break;
            }

            case Command::InsertDebugMarker: {
                /* InsertDebugMarkerCmd* cmd = */ commands.NextCommand<InsertDebugMarkerCmd>();
                break;
            }

            case Command::PopDebugGroup: {
                /* PopDebugGroupCmd* cmd = */ commands.NextCommand<PopDebugGroupCmd>();
                break;
            }

            case Command::PushDebugGroup: {
                /* PushDebugGroupCmd* cmd = */ commands.NextCommand<PushDebugGroupCmd>();
                break;
            }

            case Command::WriteTimestamp: {
                /* WriteTimestampCmd* cmd = */ commands.NextCommand<WriteTimestampCmd>();
                break;
            }

            case Command::SetImmediateData: {
                /* SetImmediateDataCmd* cmd = */ commands.NextCommand<SetImmediateDataCmd>();
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
        switch (type) {
            case Command::BeginComputePass: {
                Capture(captureContext, commands.NextCommand<BeginComputePassCmd>(), commands);
                break;
            }
            case Command::BeginRenderPass: {
                /* BeginRenderPassCmd* cmd = */ commands.NextCommand<BeginRenderPassCmd>();
                break;
            }
            case Command::CopyBufferToBuffer: {
                /* CopyBufferToBufferCmd* copy = */ commands.NextCommand<CopyBufferToBufferCmd>();
                break;
            }
            case Command::CopyBufferToTexture: {
                /* CopyBufferToTextureCmd* copy = */ commands.NextCommand<CopyBufferToTextureCmd>();
                break;
            }
            case Command::CopyTextureToBuffer: {
                /* CopyTextureToBufferCmd* copy = */ commands.NextCommand<CopyTextureToBufferCmd>();
                break;
            }
            case Command::CopyTextureToTexture: {
                /* CopyTextureToTextureCmd* copy = */ commands
                    .NextCommand<CopyTextureToTextureCmd>();
                break;
            }
            case Command::ClearBuffer: {
                /* ClearBufferCmd* cmd = */ commands.NextCommand<ClearBufferCmd>();
                break;
            }
            case Command::ResolveQuerySet: {
                /* ResolveQuerySetCmd* cmd = */ commands.NextCommand<ResolveQuerySetCmd>();
                break;
            }
            case Command::WriteTimestamp: {
                /* WriteTimestampCmd* cmd = */ commands.NextCommand<WriteTimestampCmd>();
                break;
            }
            case Command::InsertDebugMarker: {
                /* InsertDebugMarkerCmd* cmd = */ commands.NextCommand<InsertDebugMarkerCmd>();
                break;
            }
            case Command::PopDebugGroup: {
                /* PopDebugGroupCmd* cmd = */ commands.NextCommand<PopDebugGroupCmd>();
                break;
            }
            case Command::PushDebugGroup: {
                /* PushDebugGroupCmd* cmd = */ commands.NextCommand<PushDebugGroupCmd>();
                break;
            }
            case Command::WriteBuffer: {
                /* WriteBufferCmd* write = */ commands.NextCommand<WriteBufferCmd>();
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
    WriteU32(commandCount);
    for (uint32_t i = 0; i < commandCount; ++i) {
        Capture(*this, commands[i]);
    }
}

}  // namespace dawn::native
