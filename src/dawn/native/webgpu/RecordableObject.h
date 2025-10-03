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

#ifndef SRC_DAWN_NATIVE_WEBGPU_RECORDABLE_OBJECT_H_
#define SRC_DAWN_NATIVE_WEBGPU_RECORDABLE_OBJECT_H_

#include "dawn/native/Error.h"
#include "dawn/native/webgpu/Serialization.h"

namespace dawn::native::webgpu {

class CaptureContext;
class RecordableObject {
  public:
    explicit RecordableObject(schema::ObjectType objectType);
    schema::ObjectType GetObjectType() const;

    virtual void AddReferenced(CaptureContext& captureContext) const = 0;
    virtual MaybeError CaptureCreationParameters(CaptureContext& context) = 0;
    MaybeError CaptureContent(CaptureContext& context, schema::ObjectId id);

    void MarkAsNeedsCapture();
    void MarkAsHasData();
    bool NeedsCapture(bool newResource);

  protected:
    virtual MaybeError CaptureContentImpl(CaptureContext& context, schema::ObjectId id) = 0;

  private:
    schema::ObjectType mObjectType;

    // Note: There are 3 states for whether a resource needs to be captured:
    //
    // * mNeedsCapture = true means the CPU has put data in the buffer.
    //
    //   This is true if the buffer has data copied it.
    //   eg. WriteBuffer/Copy?ToBuffer, as well as mapAsync(write) and mappedAtCreation.
    //
    //   During capture, if the buffer is used, it is captured and this is cleared.
    //
    // * mHasData = true means the buffer is not all zero
    //
    //   This is set once and almost never changed. clearBuffer can reset it.
    //
    //   It's point is for a 2nd capture. If you capture once, mNeedCapture is
    //   cleared. If you end recording and capture again, you need to know if
    //   the buffer has data in it. See next item
    //
    // * Whether or not the resource has an id
    //
    // When a buffer is used, there are 2 conditions in which it will be captured.
    //   1. mNeedsCapture is true
    //   2. It has no id in the CaptureContext, and mHasData is true.
    bool mNeedsCapture = false;
    bool mHasData = false;
};

// This is used for objects that never change after creation so they only
// ever need to be capture once per recording. eg, ShaderModule, Sampler,
// BindGroupLayout, PipelineLayout, RenderPipeline, ComputePipeline, TextureView, etc...
class ImmutableRecordableObject : public RecordableObject {
  public:
    explicit ImmutableRecordableObject(schema::ObjectType objectType);
};

}  // namespace dawn::native::webgpu

#endif  // SRC_DAWN_NATIVE_WEBGPU_RECORDABLE_OBJECT_H_
