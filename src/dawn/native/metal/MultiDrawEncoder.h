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

#ifndef SRC_DAWN_NATIVE_METAL_MULTIDRAWENCODER_H_
#define SRC_DAWN_NATIVE_METAL_MULTIDRAWENCODER_H_

#include <vector>

#include "dawn/common/NonCopyable.h"
#include "dawn/native/Commands.h"
#include "dawn/native/IndirectDrawMetadata.h"
#include "dawn/native/InternalPipelineStore.h"
#include "dawn/native/metal/DeviceMTL.h"

#import <Metal/Metal.h>

namespace dawn::native::metal {

class MultiDrawConverterPipeline : public ObjectBase {
  public:
    explicit MultiDrawConverterPipeline(DeviceBase* device);
    ~MultiDrawConverterPipeline() override;

    ResultOrError<NSPRef<id<MTLComputePipelineState>>> GetOrCreatePipeline();

    NSPRef<id<MTLArgumentEncoder>> GetArgumentEncoder();

  private:
    NSPRef<id<MTLComputePipelineState>> mPipeline;
    NSPRef<id<MTLArgumentEncoder>> mArgumentEncoder;
};

class MultiDrawIndirectEncoder : public NonCopyable {
  public:
    explicit MultiDrawIndirectEncoder(DeviceBase* device);
    ~MultiDrawIndirectEncoder();

    MaybeError PrepareMultiDraws(
        id<MTLComputeCommandEncoder> encoder,
        const std::vector<IndirectDrawMetadata::IndirectMultiDraw>& multiDraws);

    void ExecuteICB(uint32_t index, id<MTLRenderCommandEncoder> encoder);

  private:
    MultiDrawConverterPipeline* GetMultiDrawPipeline();

    struct PerDrawData {
        NSPRef<id<MTLIndirectCommandBuffer>> mIndirectCommandBuffer;
        uint32_t mMaxDrawCount;
        NSPRef<id<MTLBuffer>> mArgumentBuffer;
    };

    std::vector<PerDrawData> mPerDrawData;

    DeviceBase* mDevice;
};

}  // namespace dawn::native::metal
#endif  // SRC_DAWN_NATIVE_METAL_MULTIDRAWENCODER_H_
