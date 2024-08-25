// Copyright 2024 The Dawn & Tint Authors
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

#include "dawn/native/metal/MultiDrawEncoder.h"
#include "dawn/native/ToBackend.h"
#include "dawn/native/metal/BufferMTL.h"

const char* kShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct DrawCmd {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct DrawIndexedCmd {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    uint32_t baseVertex;
    uint32_t firstInstance;
};

struct Constants {
    uint32_t maxDrawCount;
    uint32_t numIndexBufferElementsAfterOffsetLow;
    uint32_t numIndexBufferElementsAfterOffsetHigh;
    uint8_t flags;
};

// Function constant
constant bool kValidationEnabled [[function_constant(0)]];

// Flags
constant uint8_t kDrawCountBuffer = 4u;
constant uint8_t kIndexedDraw = 2u;
constant uint8_t kIndexType16 = 1u;  // if this bit is set the index type is 16bit, else its 32bit

struct MultiDrawData {
    command_buffer commandBuffer [[id(0)]];
    constant uint32_t* drawCmd [[id(1)]];  // Either DrawCmd or DrawIndexedCmd depending on flags
    // This can either be a buffer of uint16_t or uint32_t depending on flags
    constant uint32_t* indexBuffer [[id(2)]];
    constant uint* countBuffer [[id(3)]];
};

bool validateIndexed(constant Constants* constants, thread DrawIndexedCmd* cmd) {
    // Validation copied over from IndirectDrawValidationEncoder.cpp

    if (constants->numIndexBufferElementsAfterOffsetHigh >= 2) {
        return true;
    }
    if (constants->numIndexBufferElementsAfterOffsetHigh == 0 &&
        constants->numIndexBufferElementsAfterOffsetLow < cmd->firstIndex) {
        return false;
    }
    uint32_t maxIndexCount = constants->numIndexBufferElementsAfterOffsetLow - cmd->firstIndex;
    if (cmd->indexCount > maxIndexCount) {
        return false;
    }
    return true;
}

kernel void convertMultiDrawIndirect(uint2 gid [[thread_position_in_grid]],
                                     constant MultiDrawData* drawData [[buffer(0)]],
                                     constant Constants* constants [[buffer(1)]]) {
    uint32_t drawCount = 0;

    if (constants->flags & kDrawCountBuffer) {
        drawCount = min(constants->maxDrawCount, *drawData->countBuffer);
    } else {
        drawCount = constants->maxDrawCount;
    }

    const uint cmdIndex = gid.x;

    if (cmdIndex >= drawCount) {
        return;
    }

    if (!(constants->flags & kIndexedDraw)) {  // Not indexed
        constant DrawCmd* commands = (constant DrawCmd*)(drawData->drawCmd);

        DrawCmd cmd = commands[cmdIndex];
        render_command command(drawData->commandBuffer, cmdIndex);

        command.draw_primitives(primitive_type::triangle, cmd.firstVertex, cmd.vertexCount,
                                cmd.instanceCount, cmd.firstInstance);
    } else {
        constant DrawIndexedCmd* commands = (constant DrawIndexedCmd*)(drawData->drawCmd);

        DrawIndexedCmd cmd = commands[cmdIndex];
        render_command command(drawData->commandBuffer, cmdIndex);

        if (kValidationEnabled) {
            if (!validateIndexed(constants, &cmd)) {
                return;
            }
        }

        if (constants->flags & kIndexType16) {
            constant uint16_t* offsetteduint16 =
                ((constant uint16_t*)drawData->indexBuffer) + cmd.firstIndex;
            command.draw_indexed_primitives(primitive_type::triangle, cmd.indexCount,
                                            offsetteduint16, cmd.instanceCount, cmd.baseVertex,
                                            cmd.firstInstance);
        } else {
            constant uint32_t* offsetteduint32 =
                ((constant uint32_t*)drawData->indexBuffer) + cmd.firstIndex;
            command.draw_indexed_primitives(primitive_type::triangle, cmd.indexCount,
                                            offsetteduint32, cmd.instanceCount, cmd.baseVertex,
                                            cmd.firstInstance);
        }
    }
})";

// Direct copy of the Metal shader struct
struct Constants {
    uint32_t maxDrawCount;
    uint32_t numIndexBufferElementsAfterOffsetLow;
    uint32_t numIndexBufferElementsAfterOffsetHigh;
    uint8_t flags;
};

static constexpr uint8_t kValidationEnabled = 8u;
static constexpr uint8_t kDrawCountBuffer = 4u;
static constexpr uint8_t kIndexedDraw = 2u;
static constexpr uint8_t kIndexType16 = 1u;  // bit indicating 16bit index type, else 32bit

namespace dawn::native::metal {

ResultOrError<std::vector<PerMultiDrawData>> PrepareMultiDraws(
    DeviceBase* device,
    id<MTLComputeCommandEncoder> encoder,
    const std::vector<IndirectDrawMetadata::IndirectMultiDraw>& multiDraws) {
    // Create the class that stores the pipeline and argument encoder if it doesn't exist
    InternalPipelineStore* store = device->GetInternalPipelineStore();
    if (store->multidrawICBConverterPipeline == nullptr) {
        store->multidrawICBConverterPipeline = AcquireRef(new MultiDrawConverterPipeline(device));
    }

    MultiDrawConverterPipeline* pipelineStore = GetMultiDrawPipeline(device);
    NSPRef<id<MTLComputePipelineState>> pipeline;
    DAWN_TRY_ASSIGN(pipeline, pipelineStore->GetOrCreatePipeline());

    std::vector<PerMultiDrawData> outPerMultiDrawData;

    id<MTLDevice> mtlDevice = ToBackend(device)->GetMTLDevice();
    for (auto& draw : multiDraws) {
        PerMultiDrawData& drawData = outPerMultiDrawData.emplace_back();
        drawData.mMaxDrawCount = draw.cmd->maxDrawCount;

        id<MTLBuffer> indirectBuffer = ToBackend(draw.cmd->indirectBuffer.Get())->GetMTLBuffer();
        id<MTLBuffer> drawCountBuffer = nullptr;
        id<MTLBuffer> indexBuffer = nullptr;
        if (draw.cmd->drawCountBuffer != nullptr) {
            drawCountBuffer = ToBackend(draw.cmd->drawCountBuffer.Get())->GetMTLBuffer();
        }
        if (draw.indexBuffer != nullptr) {
            indexBuffer = ToBackend(draw.indexBuffer)->GetMTLBuffer();
        }

        // Create indirect command buffer
        MTLIndirectCommandBufferDescriptor* descriptor = [MTLIndirectCommandBufferDescriptor new];
        descriptor.commandTypes = draw.type == IndirectDrawMetadata::DrawType::Indexed
                                      ? MTLIndirectCommandTypeDrawIndexed
                                      : MTLIndirectCommandTypeDraw;
        descriptor.inheritBuffers = true;
        descriptor.inheritPipelineState = true;

        drawData.mIndirectCommandBuffer =
            [mtlDevice newIndirectCommandBufferWithDescriptor:descriptor
                                              maxCommandCount:draw.cmd->maxDrawCount
                                                      options:MTLResourceStorageModePrivate];
        DAWN_ASSERT(drawData.mIndirectCommandBuffer != nil);

        id<MTLArgumentEncoder> argEnc = pipelineStore->GetArgumentEncoder().Get();
        id<MTLBuffer> argBuffer = [mtlDevice newBufferWithLength:argEnc.encodedLength
                                                         options:MTLResourceStorageModePrivate];
        DAWN_ASSERT(argBuffer != nil);

        [argEnc setArgumentBuffer:argBuffer offset:0];
        [argEnc setIndirectCommandBuffer:drawData.mIndirectCommandBuffer.Get() atIndex:0];
        [argEnc setBuffer:indirectBuffer offset:draw.cmd->indirectOffset atIndex:1];
        [argEnc setBuffer:indexBuffer offset:draw.indexBufferOffsetInBytes atIndex:2];
        if (draw.cmd->drawCountBuffer != nullptr) {
            [argEnc setBuffer:drawCountBuffer offset:draw.cmd->drawCountOffset atIndex:3];
        }

        [encoder setComputePipelineState:pipeline.Get()];
        [encoder setBuffer:argBuffer offset:0 atIndex:0];
        {
            Constants constants;
            constants.maxDrawCount = draw.cmd->maxDrawCount;
            constants.flags = 0;
            constants.numIndexBufferElementsAfterOffsetLow = 0;
            constants.numIndexBufferElementsAfterOffsetHigh = 0;

            if (draw.cmd->drawCountBuffer != nullptr) {
                constants.flags |= kDrawCountBuffer;
            }
            if (device->IsValidationEnabled()) {
                constants.flags |= kValidationEnabled;
            }

            if (draw.type == IndirectDrawMetadata::DrawType::Indexed) {
                uint64_t indexBufferSizeAfterOffset =
                    draw.indexBuffer->GetSize() - draw.indexBufferOffsetInBytes;
                constants.flags |= kIndexedDraw;

                uint64_t numIndexElements = 0;
                switch (draw.indexFormat) {
                    case wgpu::IndexFormat::Uint16: {
                        numIndexElements = indexBufferSizeAfterOffset / sizeof(uint16_t);
                        constants.flags |= kIndexType16;  // Default is 32bit
                        break;
                    }
                    case wgpu::IndexFormat::Uint32: {
                        numIndexElements = indexBufferSizeAfterOffset / sizeof(uint32_t);
                        break;
                    }
                    default:
                        DAWN_UNREACHABLE();
                }
                constants.numIndexBufferElementsAfterOffsetLow =
                    static_cast<uint32_t>(numIndexElements & 0xFFFFFFFF);
                constants.numIndexBufferElementsAfterOffsetHigh =
                    static_cast<uint32_t>((numIndexElements >> 32) & 0xFFFFFFFF);
            }
            [encoder setBytes:&constants length:sizeof(Constants) atIndex:1];
        }

        // Function useResource makes the resource resident for the compute pass.
        // This needs to be called for everything that is accessed through the argument buffer,
        // otherwise it's undefined behavior, and hard to debug.
        [encoder useResource:drawData.mIndirectCommandBuffer.Get() usage:MTLResourceUsageRead];
        [encoder useResource:indexBuffer usage:MTLResourceUsageRead];
        [encoder useResource:indirectBuffer usage:MTLResourceUsageRead];
        if (draw.cmd->drawCountBuffer != nullptr) {
            [encoder useResource:drawCountBuffer usage:MTLResourceUsageRead];
        }

        id<MTLComputePipelineState> pipelineState = pipeline.Get();
        uint32_t threadgroupCount = draw.cmd->maxDrawCount / pipelineState.threadExecutionWidth;
        threadgroupCount +=
            (draw.cmd->maxDrawCount % pipelineState.threadExecutionWidth) == 0 ? 0 : 1;

        [encoder dispatchThreadgroups:MTLSizeMake(threadgroupCount, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(pipelineState.threadExecutionWidth, 1, 1)];
    }

    return outPerMultiDrawData;
}

MultiDrawConverterPipeline* GetMultiDrawPipeline(DeviceBase* device) {
    InternalPipelineStore* store = device->GetInternalPipelineStore();
    return reinterpret_cast<MultiDrawConverterPipeline*>(
        store->multidrawICBConverterPipeline.Get());
}

void ExecuteMultiDraw(PerMultiDrawData& data, id<MTLRenderCommandEncoder> encoder) {
    [encoder executeCommandsInBuffer:data.mIndirectCommandBuffer.Get()
                           withRange:NSMakeRange(0, data.mMaxDrawCount)];
}

MultiDrawConverterPipeline::MultiDrawConverterPipeline(DeviceBase* device) : ObjectBase(device) {}

MultiDrawConverterPipeline::~MultiDrawConverterPipeline() = default;

NSPRef<id<MTLArgumentEncoder>> MultiDrawConverterPipeline::GetArgumentEncoder() {
    DAWN_ASSERT(mArgumentEncoder != nullptr);
    return mArgumentEncoder;
}

ResultOrError<NSPRef<id<MTLComputePipelineState>>>
MultiDrawConverterPipeline::GetOrCreatePipeline() {
    if (mPipeline != nil) {
        return mPipeline;
    }

    id<MTLDevice> mtlDevice = ToBackend(GetDevice())->GetMTLDevice();
    NSError* error = nil;
    NSString* source = [NSString stringWithUTF8String:kShaderSource];
    id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:source options:nullptr error:&error];

    // Function constant for validation
    MTLFunctionConstantValues* funcConstants = [MTLFunctionConstantValues new];
    bool isValidationEnabled = ToBackend(GetDevice())->IsValidationEnabled();
    [funcConstants setConstantValue:&isValidationEnabled type:MTLDataTypeBool atIndex:0];

    if (error != nullptr) {
        NSLog(@"Error creating multi draw MTLLibrary: %@", error.localizedDescription);
        return DAWN_INTERNAL_ERROR("Error creating multi draw MTLLibrary:" +
                                   std::string([error.localizedDescription UTF8String]));
    }
    DAWN_ASSERT(lib != nil);

    id<MTLFunction> convertFunc = [lib newFunctionWithName:@"convertMultiDrawIndirect"
                                            constantValues:funcConstants
                                                     error:&error];
    if (error != nullptr) {
        return DAWN_INTERNAL_ERROR("Error creating multi draw converter compute function:" +
                                   std::string([error.localizedDescription UTF8String]));
    }
    DAWN_ASSERT(convertFunc != nil);

    mArgumentEncoder = [convertFunc newArgumentEncoderWithBufferIndex:0];
    mPipeline = [mtlDevice newComputePipelineStateWithFunction:convertFunc error:&error];

    if (error != nullptr) {
        return DAWN_INTERNAL_ERROR("Error creating multi draw converter compute pipeline:" +
                                   std::string([error.localizedDescription UTF8String]));
    }
    DAWN_ASSERT(mPipeline != nil);

    return mPipeline;
}

}  // namespace dawn::native::metal
