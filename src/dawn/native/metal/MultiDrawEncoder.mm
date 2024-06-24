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

const char* mslShader = R"(
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
    uint32_t numIndexBufferElementsLow;
    uint32_t numIndexBufferElementsHigh;
    uint8_t flags;
};

constant uint32_t kThreadGroupSize = 64u;
constant uint8_t kIndexedDraw = 2u;
constant uint8_t kIndexType32 = 1u; // if this bit is set the index type is 32bit, else its 16bit

struct MultiDrawData {
    command_buffer commandBuffer [[ id(0) ]];
    device uint32_t* drawCmd [[ id(1) ]]; // Either DrawCmd or DrawIndexedCmd depending on flags
    device uint16_t* indexBuffer [[id(2)]];
    device uint* countBuffer [[ id(3) ]];
};


bool validateIndexed(constant Constants* constants, thread DrawIndexedCmd* cmd) {
    // Validation copied over from IndirectDrawValidationEncoder.cpp

    if (constants->numIndexBufferElementsHigh >= 2)
        return true;
    if (constants->numIndexBufferElementsHigh == 0 && constants->numIndexBufferElementsLow < cmd->firstIndex)
        return false;
    uint32_t maxIndexCount = constants->numIndexBufferElementsLow - cmd->firstIndex;
    if(cmd->indexCount > maxIndexCount)
        return false;
    return true;
}

kernel void convertMultiDrawIndirect(uint2 gid [[thread_position_in_grid]],
                                         device MultiDrawData* drawData [[ buffer(0) ]],
                                         constant Constants* constants [[buffer(1)]]) {
    uint32_t drawCount = 0;

    if(drawData->countBuffer != nullptr)
        drawCount = *drawData->countBuffer;
    else
        drawCount = constants->maxDrawCount;

    uint cmdIndex = gid.x;

    if (!(constants->flags & kIndexedDraw)) { // Not indexed
        device DrawCmd* commands = (device DrawCmd*)(drawData->drawCmd);

        while(cmdIndex < drawCount) {
            DrawCmd cmd = commands[cmdIndex];
            render_command command(drawData->commandBuffer, cmdIndex);
            cmdIndex += kThreadGroupSize;

            command.draw_primitives(primitive_type::triangle, cmd.firstVertex, cmd.vertexCount, cmd.instanceCount, cmd.firstInstance);
        }
    }
    else {
        device DrawIndexedCmd* commands = (device DrawIndexedCmd*)(drawData->drawCmd);
        while(cmdIndex < drawCount) {
            DrawIndexedCmd cmd = commands[cmdIndex];
            render_command command(drawData->commandBuffer, cmdIndex);
            cmdIndex += kThreadGroupSize;

            if (!validateIndexed(constants, &cmd))
                continue;

            if (constants->flags & kIndexType32) {
                device uint32_t* offsetteduint32 = ((device uint32_t*)drawData->indexBuffer) + cmd.firstIndex;
                command.draw_indexed_primitives(primitive_type::triangle, cmd.indexCount, offsetteduint32, cmd.instanceCount, cmd.baseVertex, cmd.firstInstance);
            }
            else {
                device uint16_t* offsetteduint16 = drawData->indexBuffer + cmd.firstIndex;
                command.draw_indexed_primitives(primitive_type::triangle, cmd.indexCount, offsetteduint16, cmd.instanceCount, cmd.baseVertex, cmd.firstInstance);
            }
        }
    }
}
)";

// Direct copy of the Metal shader struct
struct Constants {
    uint32_t maxDrawCount;
    uint32_t numIndexBufferElementsLow;
    uint32_t numIndexBufferElementsHigh;
    uint8_t flags;
};

const uint32_t kThreadGroupSize = 64;
const uint8_t kIndexedDraw = 2u;
const uint8_t kIndexType32 = 1u;  // if this bit is set the index type is 32bit, else its 16bit

namespace dawn::native::metal {

MultiDrawIndirectEncoder::MultiDrawIndirectEncoder(DeviceBase* device) : mDevice(device) {
    InternalPipelineStore* store = mDevice->GetInternalPipelineStore();

    if (store->multidrawICBConverterPipeline == nullptr) {
        store->multidrawICBConverterPipeline = AcquireRef(new MultiDrawConverterPipeline(mDevice));
    }
}

MultiDrawIndirectEncoder::~MultiDrawIndirectEncoder() {}

MaybeError MultiDrawIndirectEncoder::PrepareMultiDraws(id<MTLComputeCommandEncoder> encoder,
                                                       IndirectDrawMetadata* metadata) {
    MultiDrawConverterPipeline* pipelineStore = GetMultiDrawPipeline();
    id<MTLComputePipelineState> pipeline;
    DAWN_TRY_ASSIGN(pipeline, pipelineStore->GetOrCreatePipeline());

    // Cleanup the draw commands from the previous render pass
    mMultiDraws.clear();

    id<MTLDevice> device = ToBackend(mDevice)->GetMTLDevice();
    for (auto& draw : metadata->GetIndirectMultiDraws()) {
        MultiDrawData& drawData = mMultiDraws.emplace_back();
        drawData.DrawCmd = draw.cmd;

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
        descriptor.commandTypes = MTLIndirectCommandTypeDraw | MTLIndirectCommandTypeDrawIndexed;
        descriptor.inheritBuffers = true;
        descriptor.inheritPipelineState = true;

        drawData.IndirectCommandBuffer =
            [device newIndirectCommandBufferWithDescriptor:descriptor
                                           maxCommandCount:draw.cmd->maxDrawCount
                                                   options:MTLResourceStorageModePrivate];

        id<MTLArgumentEncoder> argEnc = pipelineStore->GetArgumentEncoder();
        drawData.ArgumentBuffer = [device newBufferWithLength:argEnc.encodedLength
                                                      options:MTLResourceStorageModeShared];

        [argEnc setArgumentBuffer:drawData.ArgumentBuffer offset:0];
        [argEnc setIndirectCommandBuffer:drawData.IndirectCommandBuffer atIndex:0];
        [argEnc setBuffer:indirectBuffer offset:draw.cmd->indirectOffset atIndex:1];
        [argEnc setBuffer:indexBuffer offset:draw.indexBufferOffsetInBytes atIndex:2];
        [argEnc setBuffer:drawCountBuffer offset:draw.cmd->drawCountOffset atIndex:3];

        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:drawData.ArgumentBuffer offset:0 atIndex:0];
        {
            Constants constants;
            constants.maxDrawCount = draw.cmd->maxDrawCount;
            constants.flags = 0;

            if (draw.type == IndirectDrawMetadata::DrawType::Indexed) {
                uint64_t indexBufferSize =
                    draw.indexBufferSize - draw.indexBufferOffsetInBytes;

                constants.flags |= kIndexedDraw;

                switch (draw.indexFormat) {
                    case wgpu::IndexFormat::Uint16: {
                        uint64_t numIndexElements = draw.indexBufferSize / 2;
                        constants.numIndexBufferElementsLow =
                            static_cast<uint32_t>(numIndexElements & 0xFFFFFFFF);
                        constants.numIndexBufferElementsHigh =
                            static_cast<uint32_t>((numIndexElements >> 32) & 0xFFFFFFFF);
                        // No need to flip the index format bit since 0 represent uint16
                        break;
                    }
                    case wgpu::IndexFormat::Uint32: {
                        uint64_t numIndexElements = draw.indexBufferSize / 4;
                        constants.numIndexBufferElementsLow =
                            static_cast<uint32_t>(numIndexElements & 0xFFFFFFFF);
                        constants.numIndexBufferElementsHigh =
                            static_cast<uint32_t>((numIndexElements >> 32) & 0xFFFFFFFF);
                        constants.flags |= kIndexType32;
                        break;
                    }
                    default:
                        DAWN_UNREACHABLE();
                }
            }
            [encoder setBytes:&constants length:sizeof(Constants) atIndex:1];
        }
        [encoder useResource:drawData.IndirectCommandBuffer usage:MTLResourceUsageWrite];

        [encoder dispatchThreads:MTLSizeMake(kThreadGroupSize, 1, 1)
            threadsPerThreadgroup:MTLSizeMake(pipeline.threadExecutionWidth, 1, 1)];
    }

    return {};
}

MultiDrawConverterPipeline* MultiDrawIndirectEncoder::GetMultiDrawPipeline() {
    InternalPipelineStore* store = mDevice->GetInternalPipelineStore();
    return reinterpret_cast<MultiDrawConverterPipeline*>(
        store->multidrawICBConverterPipeline.Get());
}

void MultiDrawIndirectEncoder::ExecuteICB(uint32_t index, id<MTLRenderCommandEncoder> encoder) {
    MultiDrawData& data = mMultiDraws[index];

    [encoder executeCommandsInBuffer:data.IndirectCommandBuffer
                           withRange:NSMakeRange(0, data.DrawCmd->maxDrawCount)];
}

MultiDrawConverterPipeline::MultiDrawConverterPipeline(DeviceBase* device) : ObjectBase(device) {}

MultiDrawConverterPipeline::~MultiDrawConverterPipeline() = default;

id<MTLArgumentEncoder> MultiDrawConverterPipeline::GetArgumentEncoder() {
    DAWN_ASSERT(mArgumentEncoder != nullptr);
    return mArgumentEncoder;
}

ResultOrError<id<MTLComputePipelineState>> MultiDrawConverterPipeline::GetOrCreatePipeline() {
    if (mPipeline != nil) {
        return mPipeline;
    }

    id<MTLDevice> mtlDevice = ToBackend(GetDevice())->GetMTLDevice();

    NSError* error = nil;

    NSString* source = [NSString stringWithUTF8String:mslShader];

    id<MTLLibrary> lib = [mtlDevice newLibraryWithSource:source options:nullptr error:&error];

    if (error != nullptr) {
        NSLog([error localizedDescription]);
        return DAWN_INTERNAL_ERROR("Error creating multi draw MTLLibrary:" +
                                   std::string([error.localizedDescription UTF8String]));
    }

    DAWN_ASSERT(lib != nil);

    id<MTLFunction> convertFunc = [lib newFunctionWithName:@"convertMultiDrawIndirect"];

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
