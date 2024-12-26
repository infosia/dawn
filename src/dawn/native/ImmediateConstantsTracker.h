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

#ifndef SRC_DAWN_NATIVE_IMMEDIATECONSTANTSTRACKER_H_
#define SRC_DAWN_NATIVE_IMMEDIATECONSTANTSTRACKER_H_

#include <algorithm>
#include <array>
#include <bitset>

#include "dawn/common/Constants.h"
#include "dawn/common/ityp_bitset.h"
#include "dawn/common/ityp_span.h"
#include "dawn/native/ImmediateConstantsLayout.h"
#include "dawn/native/IntegerTypes.h"
#include "dawn/native/Pipeline.h"

namespace dawn::native {

template <typename T>
struct ImmediateDataContent {
  public:
    const T* operator->() const { return reinterpret_cast<const T*>(&mData); }
    T* operator->() { return reinterpret_cast<T*>(&mData); }

    const unsigned char* data() const { return mData; }

    template <typename Out>
    const Out* Get(uint32_t offset) const {
        DAWN_ASSERT(sizeof(Out) + offset <= sizeof(T));
        return reinterpret_cast<const Out*>(&mData[offset]);
    }

  private:
    alignas(T) unsigned char mData[sizeof(T)] = {0};
};

template <typename T>
class CommonImmediateConstantsTrackerBase {
  public:
    // Pipeline change resets all dirty bits.
    // TODO(crbug.com/366291600): Support immediate data compatible.
    void OnPipelineChange(PipelineBase* pipeline) {
        mPipelineImmediateConstantMask = pipeline->GetImmediateConstantMask();
        mDirty.reset();
    }

    // Setters
    void SetImmediateData(uint32_t rangeOffset,
                          uint32_t* values,
                          uint32_t dataOffset,
                          uint32_t count) {
        uint32_t rangeIndex = 0;
        for (uint32_t i = 0; i < count; ++i) {
            rangeIndex = rangeOffset + i;
            mDirty |= CommonImmediateConstantsBit::ImmediateDataStart << rangeIndex;
            mContent->commonConstants.immediateData[rangeIndex] = values[dataOffset + i];
        }
    }

    // Getters

    const ImmediateConstantMask& GetDirtyBits() const { return mDirty; }

    const ImmediateDataContent<T>& GetContent() const { return mContent; }

    void SetDirtyBitsForTesting(ImmediateConstantMask dirtyBits) { mDirty = dirtyBits; }

  protected:
    ImmediateDataContent<T> mContent;
    ImmediateConstantMask mDirty = ImmediateConstantMask(0);
    ImmediateConstantMask mPipelineImmediateConstantMask = ImmediateConstantMask(0);
};

class RenderImmediateConstantsTrackerBase
    : public CommonImmediateConstantsTrackerBase<RenderImmediateConstants> {
  public:
    void SetClampFragDepth(float minClampFragDepth, float maxClampFragDepth);
    void SetFirstIndexOffset(uint32_t firstVertex, uint32_t firstInstance);
};

class ComputeImmediateConstantsTrackerBase
    : public CommonImmediateConstantsTrackerBase<ComputeImmediateConstants> {
  public:
    void SetNumWorkgroupsX(uint32_t numWorkgroupX);
    void SetNumWorkgroupsY(uint32_t numWorkgroupY);
    void SetNumWorkgroupsZ(uint32_t numWorkgroupZ);
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_IMMEDIATECONSTANTSTRACKER_H_
