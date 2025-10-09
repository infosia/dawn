// Copyright 2020 The Dawn & Tint Authors
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

#include <vector>

#include "dawn/native/Buffer.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/QueryHelper.h"
#include "dawn/tests/DawnTest.h"
#include "dawn/utils/WGPUHelpers.h"

namespace dawn {
namespace {

static const char sConvertTimestampsToNanoseconds[] = R"(
            struct Timestamp {
                low  : u32,
                high : u32,
            }

            struct TimestampArr {
                t : array<Timestamp>
            }

            struct AvailabilityArr {
                v : array<u32>
            }

            struct TimestampParams {
                first  : u32,
                count  : u32,
                offset : u32,
                quantization_mask : u32,
                multiplier : u32,
                right_shift  : u32,
            }

            @group(0) @binding(0) var<storage, read_write> timestamps : TimestampArr;
            @group(0) @binding(1) var<storage, read> availability : AvailabilityArr;
            @group(0) @binding(2) var<uniform> params : TimestampParams;

            const sizeofTimestamp : u32 = 8u;

            @compute @workgroup_size(8, 1, 1)
            fn main(@builtin(global_invocation_id) GlobalInvocationID : vec3u) {
                if (GlobalInvocationID.x >= params.count) { return; }

                var index = GlobalInvocationID.x + params.offset / sizeofTimestamp;

                // Return 0 for the unavailable value.
                if (availability.v[GlobalInvocationID.x + params.first] == 0u) {
                    timestamps.t[index].low = 0u;
                    timestamps.t[index].high = 0u;
                    return;
                }

                var timestamp = timestamps.t[index];

                // TODO(dawn:1250): Consider using the umulExtended and uaddCarry intrinsics once
                // available.
                var chunks : array<u32, 5>;
                chunks[0] = timestamp.low & 0xFFFFu;
                chunks[1] = timestamp.low >> 16u;
                chunks[2] = timestamp.high & 0xFFFFu;
                chunks[3] = timestamp.high >> 16u;
                chunks[4] = 0u;

                // Multiply all the chunks with the integer period.
                for (var i = 0u; i < 4u; i = i + 1u) {
                    chunks[i] = chunks[i] * params.multiplier;
                }

                // Propagate the carry
                var carry = 0u;
                for (var i = 0u; i < 4u; i = i + 1u) {
                    var chunk_with_carry = chunks[i] + carry;
                    carry = chunk_with_carry >> 16u;
                    chunks[i] = chunk_with_carry & 0xFFFFu;
                }
                chunks[4] = carry;

                // Apply the right shift.
                for (var i = 0u; i < 4u; i = i + 1u) {
                    var low = chunks[i] >> params.right_shift;
                    var high = (chunks[i + 1u] << (16u - params.right_shift)) & 0xFFFFu;
                    chunks[i] = low | high;
                }

                // Apply quantization mask.
                var low = chunks[0] | (chunks[1] << 16u);
                timestamps.t[index].low = low & params.quantization_mask;
                timestamps.t[index].high = chunks[2] | (chunks[3] << 16u);
            }
        )";

wgpu::ComputePipeline GetOrCreateTimestampComputePipeline(wgpu::Device device) {
    wgpu::ShaderModule module = utils::CreateShaderModule(device, sConvertTimestampsToNanoseconds);

    // Create binding group layout
    wgpu::BindGroupLayout bgl = utils::MakeBindGroupLayout(
        device, {
                    {0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage},
                    {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                    {2, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform},
                });

    // Create pipeline layout
    wgpu::PipelineLayout layout = utils::MakePipelineLayout(device, {bgl});

    // Create ComputePipeline.
    wgpu::ComputePipelineDescriptor computePipelineDesc = {};
    // Generate the layout based on shader module.
    computePipelineDesc.layout = layout;
    computePipelineDesc.compute.module = module;
    computePipelineDesc.compute.entryPoint = "main";

    return device.CreateComputePipeline(&computePipelineDesc);
}

void EncodeConvertTimestampsToNanoseconds(wgpu::Device device,
                                          wgpu::CommandEncoder encoder,
                                          wgpu::Buffer timestamps,
                                          wgpu::Buffer availability,
                                          wgpu::Buffer params) {
    wgpu::ComputePipeline pipeline = GetOrCreateTimestampComputePipeline(device);

    // Prepare bind group layout.
    wgpu::BindGroupLayout layout = pipeline.GetBindGroupLayout(0);

    // Create bind group after all binding entries are set.
    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, layout, {{0, timestamps}, {1, availability}, {2, params}});

    // Create compute encoder and issue dispatch.
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.DispatchWorkgroups(
        static_cast<uint32_t>((timestamps.GetSize() / sizeof(uint64_t) + 7) / 8));
    pass.End();
}

class InternalShaderExpectation : public ::dawn::detail::Expectation {
  public:
    ~InternalShaderExpectation() override = default;

    InternalShaderExpectation(const uint64_t* values,
                              const unsigned int count,
                              const uint32_t quantizationMask) {
        mExpected.assign(values, values + count);
        mQuantizationMask = quantizationMask;
    }

    // Expect the actual results are approximately equal to the expected values.
    testing::AssertionResult Check(const void* data, size_t size) override {
        DAWN_ASSERT(size == sizeof(uint64_t) * mExpected.size());
        // The computations in the shader use a multiplier that's a 16bit integer plus a shift
        // that maximize the multiplier. This means that for the range of periods we care about
        // (1 to 2^16-1 ns per tick), the high order bit of the multiplier will always be set.
        // Intuitively this means that we have 15 bits of precision in the computation so we
        // expect that for the error tolerance.
        constexpr static double kErrorToleranceRatio = 1.0 / (1 << 15);  // about 3e-5.

        constexpr static double kUpperLimitMultiplier = 1.0 + kErrorToleranceRatio;
        constexpr static double kLowerLimitMultiplier = 1.0 - kErrorToleranceRatio;

        const uint64_t* actual = static_cast<const uint64_t*>(data);
        for (size_t i = 0; i < mExpected.size(); ++i) {
            if (mExpected[i] == 0) {
                if (actual[i] != 0) {
                    return testing::AssertionFailure()
                           << "Expected data[" << i << "] to be 0, actual " << actual[i] << "\n";
                }
                continue;
            }

            uint64_t expected = mExpected[i];
            uint64_t upperLimit = static_cast<double>(mExpected[i]) * kUpperLimitMultiplier;
            uint64_t lowerLimit = static_cast<double>(mExpected[i]) * kLowerLimitMultiplier;

            // Quantization may make an actual value close to the lower limit go below it.
            // Take this into account by also quantizing the lower limit.
            uint32_t invertedQuantizationMask = ~mQuantizationMask;
            uint64_t quantizationMask64 = ~uint64_t(invertedQuantizationMask);
            lowerLimit &= quantizationMask64;

            if (actual[i] < lowerLimit || actual[i] > upperLimit) {
                return testing::AssertionFailure()
                       << "Expected data[" << i << "] to be " << expected << ", actual "
                       << actual[i] << ". Error rate is larger than " << kErrorToleranceRatio
                       << ". Upper limit is " << upperLimit << ". Lower limit is " << lowerLimit
                       << "\n";
            }

            if ((actual[i] & ~quantizationMask64) != 0) {
                return testing::AssertionFailure() << "Actual data 0x" << std::hex << actual[i]
                                                   << " does not match quantization mask 0x"
                                                   << std::hex << mQuantizationMask << "\n";
            }
        }

        return testing::AssertionSuccess();
    }

  private:
    std::vector<uint64_t> mExpected;
    uint32_t mQuantizationMask;
};

constexpr static uint64_t kSentinelValue = ~uint64_t(0u);

class QueryInternalShaderTests : public DawnTest {
  protected:
    void SetUp() override {
        DawnTest::SetUp();

        DAWN_TEST_UNSUPPORTED_IF(UsesWire());
        DAWN_TEST_UNSUPPORTED_IF(HasToggleEnabled("disable_timestamp_query_conversion"));

        // If implicit device synchronization is turned on, EncodeConvertTimestampsToNanoseconds
        // will expect the device to be locked. But we are calling it directly without going through
        // wgpu API, hence the device won't be locked on this route. This would lead to assertion
        // failure. So disable the test if this feature is turned on.
        DAWN_TEST_UNSUPPORTED_IF(IsImplicitDeviceSyncEnabled());
    }

    // Original timestamp values in query set for testing
    const std::vector<uint64_t> querySetValues = {
        kSentinelValue,  // garbage data which is not written at beginning
        10079569507,     // t0
        10394415012,     // t1
        kSentinelValue,  // garbage data which is not written between timestamps
        11713454943,     // t2
        38912556941,     // t3 (big value)
        10080295766,     // t4 (reset)
        12159966783,     // t5 (after reset)
        12651224612,     // t6
        39872473956,     // t7
    };

    const uint32_t kQueryCount = querySetValues.size();

    // Timestamps available state
    const std::vector<uint32_t> availabilities = {0, 1, 1, 0, 1, 1, 1, 1, 1, 1};

    const std::vector<uint64_t> GetExpectedResults(const std::vector<uint64_t>& origin,
                                                   uint32_t start,
                                                   uint32_t firstQuery,
                                                   uint32_t queryCount,
                                                   float period) {
        std::vector<uint64_t> expected(origin.begin(), origin.end());
        for (size_t i = 0; i < queryCount; i++) {
            if (availabilities[firstQuery + i] == 0) {
                // Not a available timestamp, write 0
                expected[start + i] = 0u;
            } else {
                // Maybe the timestamp * period is larger than the maximum of uint64, so cast the
                // delta value to double (higher precision than float)
                expected[start + i] =
                    static_cast<uint64_t>(static_cast<double>(origin[start + i]) * period);
            }
        }
        return expected;
    }

    void RunTest(uint32_t firstQuery,
                 uint32_t queryCount,
                 uint32_t destinationOffset,
                 uint32_t quantizationMask,
                 float period) {
        DAWN_ASSERT(destinationOffset % kQueryResolveAlignment == 0);

        uint64_t size = queryCount * sizeof(uint64_t) + destinationOffset;

        // The resolve buffer storing original timestamps and the converted values
        wgpu::BufferDescriptor timestampsDesc;
        timestampsDesc.size = size;
        timestampsDesc.usage =
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
        wgpu::Buffer timestampsBuffer = device.CreateBuffer(&timestampsDesc);

        // Set sentinel values to check the slots before the destination offset should not be
        // converted
        std::vector<uint64_t> timestampValues(size / sizeof(uint64_t), 1u);
        uint32_t start = destinationOffset / sizeof(uint64_t);
        for (uint32_t i = 0; i < queryCount; i++) {
            timestampValues[start + i] = querySetValues[firstQuery + i];
        }
        // Write sentinel values and orignal timestamps to timestamps buffer
        queue.WriteBuffer(timestampsBuffer, 0, timestampValues.data(), size);

        // The buffer indicating which values are available timestamps
        wgpu::Buffer availabilityBuffer =
            utils::CreateBufferFromData(device, availabilities.data(),
                                        kQueryCount * sizeof(uint32_t), wgpu::BufferUsage::Storage);

        // The params uniform buffer
        native::TimestampParams params(firstQuery, queryCount, destinationOffset, quantizationMask,
                                       period);
        wgpu::Buffer paramsBuffer = utils::CreateBufferFromData(device, &params, sizeof(params),
                                                                wgpu::BufferUsage::Uniform);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        EncodeConvertTimestampsToNanoseconds(device, encoder, timestampsBuffer, availabilityBuffer,
                                             paramsBuffer);
        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        const std::vector<uint64_t> expected =
            GetExpectedResults(timestampValues, start, firstQuery, queryCount, period);

        EXPECT_BUFFER(timestampsBuffer, 0, size,
                      new InternalShaderExpectation(expected.data(), size / sizeof(uint64_t),
                                                    quantizationMask))
            << "Conversion test for period:" << period << " firstQuery:" << firstQuery
            << " queryCount:" << queryCount << " destinationOffset:" << destinationOffset
            << " quantizationMask: 0x" << std::hex << quantizationMask;
    }
};

// Test the accuracy of timestamp compute shader which uses unsigned 32-bit integers to simulate
// unsigned 64-bit integers (timestamps) multiplied by float (period).
// The arguments pass to timestamp internal pipeline:
// - The timestamps buffer contains the original timestamps resolved from query set (created
//   manually here), and will be used to store the results processed by the compute shader.
//   Expect 0 for unavailable timestamps and nanoseconds for available timestamps in an expected
//   error tolerance ratio.
// - The availability buffer passes the data of which slot in timestamps buffer is an initialized
//    timestamp.
// - The params buffer passes the timestamp count, the offset in timestamps buffer and the
//   timestamp period (here use GPU frequency (HZ) on Intel D3D12 to calculate the period in
//   ns for testing).
TEST_P(QueryInternalShaderTests, TimestampComputeShaderMultiplication) {
    DAWN_SUPPRESS_TEST_IF(IsWARP());

    constexpr std::array<float, 5> kPeriodsToTest = {
        1,
        7,
        // A gpu frequency on Intel D3D12 (ticks/second)
        83.333,
        1042,
        65535,
    };

    const uint32_t kNoOpQuantizationBitmask = 0xFFFFFFFF;

    for (float period : kPeriodsToTest) {
        // Convert timestamps in timestamps buffer with offset 0
        // Test for ResolveQuerySet(querySet, 0, kQueryCount, timestampsBuffer, 0)
        RunTest(0, kQueryCount, 0, kNoOpQuantizationBitmask, period);

        // Convert timestamps in timestamps buffer with offset 256
        // Test for ResolveQuerySet(querySet, 1, kQueryCount - 1, timestampsBuffer, 256)
        RunTest(1, kQueryCount - 1, kQueryResolveAlignment, kNoOpQuantizationBitmask, period);

        // Convert partial timestamps in timestamps buffer with offset 256
        // Test for ResolveQuerySet(querySet, 1, 4, timestampsBuffer, 256)
        RunTest(1, 4, kQueryResolveAlignment, kNoOpQuantizationBitmask, period);
    }
}

TEST_P(QueryInternalShaderTests, TimestampComputeShaderQuantization) {
    DAWN_SUPPRESS_TEST_IF(IsWARP());
    DAWN_TEST_UNSUPPORTED_IF(!HasToggleEnabled("timestamp_quantization"));

    constexpr std::array<uint32_t, 3> kQuantizationMasksToTest = {
        0xFFFFFFFF,
        0xFFFFFF00,
        0xFFFF0000,
    };

    for (uint32_t quantizationMask : kQuantizationMasksToTest) {
        // Convert timestamps in timestamps buffer with offset 0
        // Test for ResolveQuerySet(querySet, 0, kQueryCount, timestampsBuffer, 0)
        RunTest(0, kQueryCount, 0, quantizationMask, 1);
    }
}

DAWN_INSTANTIATE_TEST(QueryInternalShaderTests, D3D12Backend(), MetalBackend(), VulkanBackend());

}  // anonymous namespace
}  // namespace dawn
