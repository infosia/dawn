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

#include <string>
#include <vector>

#include "dawn/common/NonMovable.h"
#include "dawn/tests/unittests/validation/ValidationTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

class PixelLocalStorageDisabledTest : public ValidationTest {};
class ImmediateDataDisableTest : public ValidationTest {};

TEST_F(ImmediateDataDisableTest, ImmediateDataRangeByteSizeNotAllowed) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;

    // Failed with small byte size Without toggle supported.
    desc.immediateDataRangeByteSize = 1;

    ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
}

class ImmediateDataTest : public ValidationTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        // Test only the non-coherent version, ad assume that the same validaiton code paths are
        // taken for the coherent path
        return {wgpu::FeatureName::ChromiumExperimentalImmediateData};
    }

    std::vector<const char*> GetEnabledToggles() override { return {"use_immediate_data"}; }
};

// Check that it is possible to use the WGSL extension when the device extension is enabled.
TEST_F(ImmediateDataTest, ImmediateDataRangeByteSizeAllowed) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;

    // Failed with exceed limit byte range size.
    desc.immediateDataRangeByteSize = kMaxImmediateDataRangeByteSize;

    device.CreatePipelineLayout(&desc);
}

// Check that it is possible to use the WGSL extension when the device extension is enabled.
TEST_F(ImmediateDataTest, ImmediateDataRangeByteSizeExceedLimits) {
    wgpu::PipelineLayoutDescriptor desc;
    desc.bindGroupLayoutCount = 0;
    // Failed with exceed limit byte range size.
    desc.immediateDataRangeByteSize = kMaxImmediateDataRangeByteSize + 1;

    ASSERT_DEVICE_ERROR(device.CreatePipelineLayout(&desc));
}

}  // anonymous namespace
}  // namespace dawn
