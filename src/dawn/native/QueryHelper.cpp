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

#include "dawn/native/QueryHelper.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "dawn/common/Constants.h"
#include "dawn/common/Strings.h"
#include "dawn/native/BindGroup.h"
#include "dawn/native/BindGroupLayout.h"
#include "dawn/native/Buffer.h"
#include "dawn/native/CommandEncoder.h"
#include "dawn/native/ComputePassEncoder.h"
#include "dawn/native/ComputePipeline.h"
#include "dawn/native/Device.h"
#include "dawn/native/InternalPipelineStore.h"
#include "dawn/native/PassResourceUsage.h"
#include "dawn/native/QuerySet.h"
#include "dawn/native/Queue.h"
#include "dawn/native/Toggles.h"
#include "dawn/native/utils/WGPUHelpers.h"

namespace dawn::native {

namespace {

// Assert the offsets in dawn::native::TimestampParams are same with the ones in the shader
static_assert(offsetof(dawn::native::TimestampParams, first) == 0);
static_assert(offsetof(dawn::native::TimestampParams, count) == 4);
static_assert(offsetof(dawn::native::TimestampParams, offset) == 8);
static_assert(offsetof(dawn::native::TimestampParams, quantizationMask) == 12);
static_assert(offsetof(dawn::native::TimestampParams, multiplier) == 16);
static_assert(offsetof(dawn::native::TimestampParams, rightShift) == 20);

static const char sConvertTimestampsToNanoseconds[] = DAWN_MULTILINE(
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
        var lowValue = (chunks[0] & 0xFFFFu) | (chunks[1] << 16u);
        var highValue = (chunks[2] & 0xFFFFu) | (chunks[3] << 16u);
        timestamps.t[index].low = lowValue & params.quantization_mask;
        timestamps.t[index].high = highValue;
    }
);

ResultOrError<ComputePipelineBase*> GetOrCreateTimestampComputePipeline(DeviceBase* device) {
    InternalPipelineStore* store = device->GetInternalPipelineStore();

    if (store->timestampComputePipeline == nullptr) {
        // Create compute shader module if not cached before.
        if (store->timestampCS == nullptr) {
            DAWN_TRY_ASSIGN(store->timestampCS,
                            utils::CreateShaderModule(device, sConvertTimestampsToNanoseconds));
        }

        // Create binding group layout
        Ref<BindGroupLayoutBase> bgl;
        DAWN_TRY_ASSIGN(
            bgl, utils::MakeBindGroupLayout(
                     device,
                     {
                         {0, wgpu::ShaderStage::Compute, kInternalStorageBufferBinding},
                         {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                         {2, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform},
                     },
                     /* allowInternalBinding */ true));

        // Create pipeline layout
        Ref<PipelineLayoutBase> layout;
        DAWN_TRY_ASSIGN(layout, utils::MakeBasicPipelineLayout(device, bgl));

        // Create ComputePipeline.
        ComputePipelineDescriptor computePipelineDesc = {};
        // Generate the layout based on shader module.
        computePipelineDesc.layout = layout.Get();
        computePipelineDesc.compute.module = store->timestampCS.Get();
        computePipelineDesc.compute.entryPoint = "main";

        DAWN_TRY_ASSIGN(store->timestampComputePipeline,
                        device->CreateComputePipeline(&computePipelineDesc));
    }

    return store->timestampComputePipeline.Get();
}

ResultOrError<Ref<BufferBase>> CreateBufferWithData(DeviceBase* device,
                                                    const void* data,
                                                    uint64_t size,
                                                    wgpu::BufferUsage usage,
                                                    const char* label) {
    DAWN_ASSERT(size == 0 || data != nullptr);

    BufferDescriptor descriptor = {};
    descriptor.size = size;
    descriptor.usage = usage;
    descriptor.label = label;
    descriptor.mappedAtCreation = true;

    Ref<BufferBase> buffer;
    DAWN_TRY_ASSIGN(buffer, device->CreateBuffer(&descriptor));

    uint8_t* mappedData = static_cast<uint8_t*>(buffer->GetMappedPointer());
    std::memcpy(mappedData, data, size);
    DAWN_TRY(buffer->Unmap());

    return buffer;
}

}  // anonymous namespace

TimestampParams::TimestampParams(uint32_t first,
                                 uint32_t count,
                                 uint32_t offset,
                                 uint32_t quantizationMask,
                                 float period)
    : first(first),
      count(count),
      offset(offset),
      quantizationMask(quantizationMask),
      multiplier(0),
      rightShift(0) {
    DAWN_ASSERT(period >= 1.0f);
    // Optimization for period is 1.0.
    if (period == 1.0f) {
        multiplier = 1u;
        return;
    }

    // The logic to convert the timestamp query to nanoseconds:
    // val = timestamp * period
    //
    // However, the period is a float, we need to convert it to a multiplier and a right shift
    // to perform the multiplication and shift in WGSL.
    //
    // period = multiplier / (1 << rightShift)
    // multiplier = period * (1 << rightShift)
    //
    // We choose the multiplier to be as large as possible without overflowing 32-bit
    // integer.
    uint32_t upperLog2 = static_cast<uint32_t>(std::ceil(std::log2(period)));
    // Clamp the shift to 16 because we're doing computations in 16bit chunks. The
    // multiplication by the period will overflow the chunks, but timestamps are mostly
    // informational so that's ok.
    rightShift = 16u - std::min(upperLog2, 16u);
    multiplier = uint32_t(period * (1 << rightShift));
}

ResultOrError<Ref<CommandBufferBase>> EncodeTimestampsToNanosecondsConversion(
    DeviceBase* device,
    QuerySetBase* querySet,
    uint32_t firstQuery,
    uint32_t queryCount,
    BufferBase* destination,
    uint64_t destinationOffset) {
    const uint32_t quantization_mask = (device->IsToggleEnabled(Toggle::TimestampQuantization))
                                           ? kTimestampQuantizationMask
                                           : 0xFFFFFFFF;

    // Timestamp params uniform buffer
    TimestampParams params(firstQuery, queryCount, static_cast<uint32_t>(destinationOffset),
                           quantization_mask, device->GetTimestampPeriodInNS());

    Ref<CommandEncoder> encoder = CommandEncoder::Create(device, "InternalTimestampConversion");
    CommandEncoder::InternalUsageScope scope = encoder->MakeInternalUsageScope();

    Ref<BufferBase> paramsBuffer;
    DAWN_TRY_ASSIGN(paramsBuffer,
                    CreateBufferWithData(device, &params, sizeof(params),
                                         wgpu::BufferUsage::Uniform, "TimestampConversionParams"));

    // Availability buffer
    std::vector<uint32_t> availabilities(querySet->GetQueryCount());
    for (uint32_t i = 0; i < querySet->GetQueryCount(); i++) {
        availabilities[i] = querySet->IsQueryAvailable(i) ? 1u : 0u;
    }
    Ref<BufferBase> availabilityBuffer;
    DAWN_TRY_ASSIGN(availabilityBuffer,
                    CreateBufferWithData(
                        device, availabilities.data(), availabilities.size() * sizeof(uint32_t),
                        wgpu::BufferUsage::Storage, "TimestampConversionAvailability"));

    // In the internal shader to convert timestamps to nanoseconds, we can ensure no uninitialized
    // data will be read and the full buffer range will be filled with valid data.
    if (!destination->IsInitialized() &&
        destination->IsFullBufferRange(destinationOffset, sizeof(uint64_t) * queryCount)) {
        destination->SetInitialized(true);
    }

    DAWN_TRY(EncodeConvertTimestampsToNanoseconds(encoder.Get(), destination,
                                                  availabilityBuffer.Get(), paramsBuffer.Get()));

    return encoder->Finish();
}

MaybeError EncodeConvertTimestampsToNanoseconds(CommandEncoder* encoder,
                                                BufferBase* timestamps,
                                                BufferBase* availability,
                                                BufferBase* params) {
    DeviceBase* device = encoder->GetDevice();
    DAWN_ASSERT(device->IsLockedByCurrentThreadIfNeeded());

    ComputePipelineBase* pipeline;
    DAWN_TRY_ASSIGN(pipeline, GetOrCreateTimestampComputePipeline(device));

    // Prepare bind group layout.
    Ref<BindGroupLayoutBase> layout;
    DAWN_TRY_ASSIGN(layout, pipeline->GetBindGroupLayout(0));

    // Create bind group after all binding entries are set.
    Ref<BindGroupBase> bindGroup;
    DAWN_TRY_ASSIGN(
        bindGroup,
        utils::MakeBindGroup(device, layout, {{0, timestamps}, {1, availability}, {2, params}},
                             UsageValidationMode::Internal));

    // Create compute encoder and issue dispatch.
    Ref<ComputePassEncoder> pass = encoder->BeginComputePass();
    pass->APISetPipeline(pipeline);
    pass->APISetBindGroup(0, bindGroup.Get());
    pass->APIDispatchWorkgroups(
        static_cast<uint32_t>((timestamps->GetSize() / sizeof(uint64_t) + 7) / 8));
    pass->APIEnd();

    return {};
}

}  // namespace dawn::native
