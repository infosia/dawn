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

#include "dawn/native/metal/BindGroupMTL.h"

#include "dawn/common/MatchVariant.h"
#include "dawn/common/Range.h"
#include "dawn/native/metal/BindGroupLayoutMTL.h"
#include "dawn/native/metal/BufferMTL.h"
#include "dawn/native/metal/DeviceMTL.h"
#include "dawn/native/metal/SamplerMTL.h"
#include "dawn/native/metal/TextureMTL.h"

namespace dawn::native::metal {

// static
ResultOrError<Ref<BindGroup>> BindGroup::Create(Device* device,
                                                const BindGroupDescriptor* descriptor) {
    Ref<BindGroup> bindGroup = ToBackend(descriptor->layout->GetInternalBindGroupLayout())
                                   ->AllocateBindGroup(device, descriptor);
    DAWN_TRY(bindGroup->Initialize(descriptor));
    return bindGroup;
}

BindGroup::BindGroup(Device* device, const BindGroupDescriptor* descriptor)
    : BindGroupBase(this, device, descriptor) {}

BindGroup::~BindGroup() = default;

MaybeError BindGroup::InitializeImpl() {
    auto* device = ToBackend(GetDevice());
    if (!device->IsToggleEnabled(Toggle::MetalUseArgumentBuffers)) {
        return {};
    }

    printf("BIND GROPU SETUP\n");

    // TODO(dsinclair): Make sure the argument encoder and argument buffers get cleaned up ....
    // TODO(dsinclair): The argument buffers should probably work in some kind of pool instead
    //                  of being allocated here

    auto layout = ToBackend(GetLayout());
    {
        auto& descriptors = layout->GetVertexArgumentDescriptors();
        if (!descriptors.empty()) {
            NSArray* ary = [NSArray arrayWithObjects:descriptors.data() count:descriptors.size()];
            mVertexArgumentEncoder = [device->GetMTLDevice() newArgumentEncoderWithArguments:ary];

            NSUInteger vertArgumentBufferLength = mVertexArgumentEncoder.encodedLength;

            mVertexArgumentBuffer =
                [device->GetMTLDevice() newBufferWithLength:vertArgumentBufferLength options:0];
            [mVertexArgumentEncoder setArgumentBuffer:mVertexArgumentBuffer offset:0];
        }
    }
    {
        auto& descriptors = layout->GetFragmentArgumentDescriptors();
        if (!descriptors.empty()) {
            NSArray* ary = [NSArray arrayWithObjects:descriptors.data() count:descriptors.size()];
            mFragmentArgumentEncoder = [device->GetMTLDevice() newArgumentEncoderWithArguments:ary];

            NSUInteger fragArgumentBufferLength = mFragmentArgumentEncoder.encodedLength;

            mFragmentArgumentBuffer =
                [device->GetMTLDevice() newBufferWithLength:fragArgumentBufferLength options:0];
            [mFragmentArgumentEncoder setArgumentBuffer:mFragmentArgumentBuffer offset:0];
        }
    }
    {
        auto& descriptors = layout->GetComputeArgumentDescriptors();
        if (!descriptors.empty()) {
            NSArray* ary = [NSArray arrayWithObjects:descriptors.data() count:descriptors.size()];
            mComputeArgumentEncoder = [device->GetMTLDevice() newArgumentEncoderWithArguments:ary];

            NSUInteger argumentBufferLength = mComputeArgumentEncoder.encodedLength;

            mComputeArgumentBuffer =
                [device->GetMTLDevice() newBufferWithLength:argumentBufferLength options:0];
            [mComputeArgumentEncoder setArgumentBuffer:mComputeArgumentBuffer offset:0];
        }
    }

    // TODO(dsinclair): These values came from the pipeline layout previously ... ?
    uint32_t vertIndex = 0;
    uint32_t fragIndex = 0;
    uint32_t computeIndex = 0;

    for (BindingIndex bindingIndex : Range(layout->GetBindingCount())) {
        const BindingInfo& bindingInfo = layout->GetBindingInfo(bindingIndex);

        bool hasVertStage = bindingInfo.visibility & wgpu::ShaderStage::Vertex;
        bool hasFragStage = bindingInfo.visibility & wgpu::ShaderStage::Fragment;
        bool hasComputeStage = bindingInfo.visibility & wgpu::ShaderStage::Compute;

        auto HandleTextureBinding = [&]() {
            auto textureView = ToBackend(GetBindingAsTextureView(bindingIndex));
            id<MTLTexture> texture = textureView->GetMTLTexture();
            if (hasVertStage) {
                [mVertexArgumentEncoder setTexture:texture atIndex:vertIndex++];
            }
            if (hasFragStage) {
                [mFragmentArgumentEncoder setTexture:texture atIndex:fragIndex++];
            }
            if (hasComputeStage) {
                [mComputeArgumentEncoder setTexture:texture atIndex:computeIndex++];
            }
        };

        MatchVariant(
            bindingInfo.bindingLayout,
            [&](const BufferBindingInfo& layout) {
                const BufferBinding& binding = GetBindingAsBufferBinding(bindingIndex);

                const id<MTLBuffer> buffer = ToBackend(binding.buffer)->GetMTLBuffer();
                NSUInteger offset = binding.offset;

                // TODO(crbug.com/dawn/854): Record bound buffer status to use
                // setBufferOffset to achieve better performance.
                if (layout.hasDynamicOffset) {
                    // Dynamic buffers are packed at the front of BindingIndices.
                    // TODO(dsinclair): Handle dynamic offsets ...
                    // offset += dynamicOffsets[bindingIndex];
                }

                if (hasVertStage) {
                    [mVertexArgumentEncoder setBuffer:buffer offset:offset atIndex:vertIndex++];
                }
                if (hasFragStage) {
                    [mFragmentArgumentEncoder setBuffer:buffer offset:offset atIndex:fragIndex++];
                }
                if (hasComputeStage) {
                    [mComputeArgumentEncoder setBuffer:buffer offset:offset atIndex:computeIndex++];
                }
            },
            [&](const SamplerBindingInfo&) {
                auto sampler = ToBackend(GetBindingAsSampler(bindingIndex));
                id<MTLSamplerState> samplerState = sampler->GetMTLSamplerState();
                if (hasVertStage) {
                    [mVertexArgumentEncoder setSamplerState:samplerState atIndex:vertIndex++];
                }
                if (hasFragStage) {
                    [mFragmentArgumentEncoder setSamplerState:samplerState atIndex:fragIndex++];
                }
                if (hasComputeStage) {
                    [mComputeArgumentEncoder setSamplerState:samplerState atIndex:computeIndex++];
                }
            },
            [&](const StaticSamplerBindingInfo&) {
                // Static samplers are handled in the frontend.
                // TODO(crbug.com/dawn/2482): Implement static samplers in the
                // Metal backend.
                DAWN_UNREACHABLE();
            },
            [&](const TextureBindingInfo&) { HandleTextureBinding(); },
            [&](const StorageTextureBindingInfo&) { HandleTextureBinding(); },
            [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
    }

    return {};
}

void BindGroup::DeleteThis() {
    // This function must first run the destructor and then deallocate memory. Take a reference to
    // the BindGroupLayout+SlabAllocator before running the destructor so this function can access
    // it afterwards and it's not destroyed prematurely.
    Ref<BindGroupLayout> layout = ToBackend(GetLayout());
    BindGroupBase::DeleteThis();
    layout->DeallocateBindGroup(this);
}

}  // namespace dawn::native::metal
