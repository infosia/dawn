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

#include <optional>
#include <vector>

#include "dawn/tests/DawnTest.h"

#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

// Tests somewhat safe results from dynamically infinite loops.
// It's hard to write a conformnace test because the specified result is a dynamic
// error. But we can check for platform-specific results.
class InfiniteLoopTests : public DawnTest {
  public:
    // Runs a test on an input array, yielding a buffer of outputs.
    // Checks the given expectation on the output buffer. Takes ownership
    // of the expectation object.
    void RunTest(const char* file,
                 size_t line,
                 const char* shader,
                 const std::vector<uint32_t>& inputs,
                 size_t numOutputs,
                 detail::Expectation* expectation);
};
#define RUN_TEST(shader, inputs, numOutputs, expectation) \
    this->RunTest(__FILE__, __LINE__, shader, inputs, numOutputs, expectation)

template <typename T>
class ExpectOneNonZero : public detail::Expectation {
  public:
    explicit ExpectOneNonZero(T value) : mValue(value) { DAWN_ASSERT(mValue != T(0)); }

    testing::AssertionResult Check(const void* data, size_t size) override {
        DAWN_ASSERT(size % sizeof(T) == 0 && size > 0);
        const T* actual = static_cast<const T*>(data);

        std::optional<size_t> where_found;
        for (size_t i = 0; i < size / sizeof(T); i++) {
            if (actual[i] == mValue) {
                if (where_found.has_value()) {
                    return testing::AssertionFailure()
                           << "Found value " << mValue << " at data[" << where_found.value()
                           << "] and data[" << i << "]\n";
                }
                where_found = i;
            } else if (actual[i] != 0) {
                return testing::AssertionFailure()
                       << "Found unexpected value data[" << i << "] = " << actual[i]
                       << " instead of " << mValue << "\n";
            }
        }
        return testing::AssertionSuccess();
    }

  private:
    const T mValue;
};

void InfiniteLoopTests::RunTest(const char* file,
                                size_t line,
                                const char* shader,
                                const std::vector<uint32_t>& inputs,
                                size_t numOutputs,
                                detail::Expectation* expectation) {
    // Set up shader and pipeline
    auto module = utils::CreateShaderModule(device, shader);

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.compute.module = module;

    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    // Set up src storage buffer
    wgpu::Buffer src = utils::CreateBufferFromData(
        device, inputs.data(), inputs.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    // Set up dst storage buffer
    std::vector<uint32_t> dst_init_values(numOutputs, 0);

    const auto outSize = numOutputs * sizeof(uint32_t);
    wgpu::Buffer dst = utils::CreateBufferFromData(
        device, dst_init_values.data(), outSize,
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);

    // Set up bind group and issue dispatch
    wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                     {
                                                         {0, src},
                                                         {1, dst},
                                                     });

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.DispatchWorkgroups(1);
        pass.End();

        commands = encoder.Finish();
    }

    AddBufferExpectation(file, line, dst, 0, outSize, expectation);

    queue.Submit(1, &commands);
}

TEST_P(InfiniteLoopTests, WhileThenVaryingOOB) {
    DAWN_SKIP_TEST_IF_BASE(!IsMetal(), "platform-specific", "only test on Metal");

    const char* shader = R"(
@group(0) @binding(0) var<storage, read>       inputs  : array<u32>;
@group(0) @binding(1) var<storage, read_write> outputs : Outputs;

@compute @workgroup_size(1)
fn main() {
  while true { }
  outputs[inputs[0]] = 777;
})";

    auto inputs = std::vector<uint32_t>{100};
    RUN_TEST(shader, inputs, 50, new ExpectOneNonZero<uint32_t>(777));
}

DAWN_INSTANTIATE_TEST(InfiniteLoopTests, MetalBackend());

}  // anonymous namespace
}  // namespace dawn
