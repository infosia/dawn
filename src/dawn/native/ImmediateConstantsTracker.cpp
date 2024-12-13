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

#include "dawn/native/ImmediateConstantsTracker.h"

namespace dawn::native {
RenderImmediateConstantsTrackerBase::RenderImmediateConstantsTrackerBase(DeviceBase* device)
    : CommonImmediateConstantsTrackerBase(device) {}

void RenderImmediateConstantsTrackerBase::SetClampFragDepth(float minClampFragDepth,
                                                            float maxClampFragDepth) {
    const ImmediateConstant& clampFragDepthConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::ClampFragDepth);
    mDirty |= clampFragDepthConstant.constantBlockBits;
    mContent->minClampFragDepth = minClampFragDepth;
    mContent->maxClampFragDepth = maxClampFragDepth;
}

void RenderImmediateConstantsTrackerBase::SetFirstIndexOffset(uint32_t firstVertex,
                                                              uint32_t firstInstance) {
    this->SetFirstVertex(firstVertex);
    this->SetFirstInstance(firstInstance);
}

void RenderImmediateConstantsTrackerBase::SetFirstVertex(uint32_t firstVertex) {
    const ImmediateConstant& firstVertexConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::FirstVertex);
    mDirty |= firstVertexConstant.constantBlockBits;
    mContent->firstVertex = firstVertex;
}

void RenderImmediateConstantsTrackerBase::SetFirstInstance(uint32_t firstInstance) {
    const ImmediateConstant& firstInstanceConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::FirstInstance);
    mDirty |= firstInstanceConstant.constantBlockBits;
    mContent->firstInstance = firstInstance;
}

ComputeImmediateConstantsTrackerBase::ComputeImmediateConstantsTrackerBase(DeviceBase* device)
    : CommonImmediateConstantsTrackerBase(device) {}
void ComputeImmediateConstantsTrackerBase::SetNumWorkgroups(uint32_t numWorkgroupX,
                                                            uint32_t numWorkgroupY,
                                                            uint32_t numWorkgroupZ) {
    const ImmediateConstant& numWorkgroupsConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::NumWorkgroups);
    mDirty |= numWorkgroupsConstant.constantBlockBits;
    mContent->numWorkgroupsX = numWorkgroupX;
    mContent->numWorkgroupsY = numWorkgroupY;
    mContent->numWorkgroupsZ = numWorkgroupZ;
}

void ComputeImmediateConstantsTrackerBase::SetNumWorkgroupsX(uint32_t numWorkgroupX) {
    const ImmediateConstant& numWorkgroupsConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::NumWorkgroups);
    mDirty |= numWorkgroupsConstant.GetRawStartBit();
    mContent->numWorkgroupsX = numWorkgroupX;
}

void ComputeImmediateConstantsTrackerBase::SetNumWorkgroupsY(uint32_t numWorkgroupY) {
    const ImmediateConstant& numWorkgroupsConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::NumWorkgroups);
    mDirty |= numWorkgroupsConstant.GetRawStartBit() << 1u;
    mContent->numWorkgroupsY = numWorkgroupY;
}

void ComputeImmediateConstantsTrackerBase::SetNumWorkgroupsZ(uint32_t numWorkgroupZ) {
    const ImmediateConstant& numWorkgroupsConstant =
        mDevice->GetImmediateConstant(ImmediateConstantType::NumWorkgroups);
    mDirty |= numWorkgroupsConstant.GetRawStartBit() << 2u;
    mContent->numWorkgroupsZ = numWorkgroupZ;
}

}  // namespace dawn::native
