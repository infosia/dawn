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

#include "dawn/native/ImmediateConstantsTracker.h"

namespace dawn::native {
RenderImmediateConstantsTrackerBase::RenderImmediateConstantsTrackerBase()
    : UserImmediateConstantsTrackerBase() {}

void RenderImmediateConstantsTrackerBase::SetClampFragDepth(float minClampFragDepth,
                                                            float maxClampFragDepth) {
    size_t clampFragDepthStartOffsetBytes = offsetof(RenderImmediateConstants, clampFragDepth);

    size_t minClampFragDepthOffsetBytes =
        clampFragDepthStartOffsetBytes + offsetof(ClampFragDepthArgs, minClampFragDepth);
    size_t maxClampFragDepthOffsetBytes =
        clampFragDepthStartOffsetBytes + offsetof(ClampFragDepthArgs, maxClampFragDepth);

    // Set dirty bits if any elements needs to be updated. This ensure use a single call
    // to update all internal constants required to be updated.
    // Compare float value is not reliable. Compare bits instead.
    bool updateMinClampFragDepth = memcmp(mContent.Get<float>(minClampFragDepthOffsetBytes),
                                          &minClampFragDepth, sizeof(float)) != 0;
    bool updateMaxClampFragDepth = memcmp(mContent.Get<float>(maxClampFragDepthOffsetBytes),
                                          &maxClampFragDepth, sizeof(float)) != 0;

    if (updateMinClampFragDepth || updateMaxClampFragDepth) {
        mDirty |= GetImmediateConstantBlockBits(clampFragDepthStartOffsetBytes,
                                                sizeof(ClampFragDepthArgs));

        if (updateMinClampFragDepth) {
            mContent->clampFragDepth.minClampFragDepth = minClampFragDepth;
        }

        if (updateMaxClampFragDepth) {
            mContent->clampFragDepth.maxClampFragDepth = maxClampFragDepth;
        }
    }
}

void RenderImmediateConstantsTrackerBase::SetFirstIndexOffset(uint32_t firstVertex,
                                                              uint32_t firstInstance) {
    this->SetFirstVertex(firstVertex);
    this->SetFirstInstance(firstInstance);
}

void RenderImmediateConstantsTrackerBase::SetFirstVertex(uint32_t firstVertex) {
    if (mContent->firstVertex != firstVertex) {
        mDirty |= GetImmediateConstantBlockBits(offsetof(RenderImmediateConstants, firstVertex),
                                                kImmediateConstantElementByteSize);
        mContent->firstVertex = firstVertex;
    }
}

void RenderImmediateConstantsTrackerBase::SetFirstInstance(uint32_t firstInstance) {
    if (mContent->firstInstance != firstInstance) {
        mDirty |= GetImmediateConstantBlockBits(offsetof(RenderImmediateConstants, firstInstance),
                                                kImmediateConstantElementByteSize);
        mContent->firstInstance = firstInstance;
    }
}

ComputeImmediateConstantsTrackerBase::ComputeImmediateConstantsTrackerBase()
    : UserImmediateConstantsTrackerBase() {}
void ComputeImmediateConstantsTrackerBase::SetNumWorkgroups(uint32_t numWorkgroupX,
                                                            uint32_t numWorkgroupY,
                                                            uint32_t numWorkgroupZ) {
    // Set dirty bits if any elements needs to be updated. This ensure use a single call
    // to update all internal constants required to be updated.
    if (mContent->numWorkgroups.numWorkgroupsX != numWorkgroupX ||
        mContent->numWorkgroups.numWorkgroupsY != numWorkgroupY ||
        mContent->numWorkgroups.numWorkgroupsZ != numWorkgroupZ) {
        mDirty |= GetImmediateConstantBlockBits(offsetof(ComputeImmediateConstants, numWorkgroups),
                                                sizeof(NumWorkgroupsDimensions));

        if (mContent->numWorkgroups.numWorkgroupsX != numWorkgroupX) {
            mContent->numWorkgroups.numWorkgroupsX = numWorkgroupX;
        }

        if (mContent->numWorkgroups.numWorkgroupsY != numWorkgroupY) {
            mContent->numWorkgroups.numWorkgroupsY = numWorkgroupY;
        }

        if (mContent->numWorkgroups.numWorkgroupsZ != numWorkgroupZ) {
            mContent->numWorkgroups.numWorkgroupsZ = numWorkgroupZ;
        }
    }
}

}  // namespace dawn::native
