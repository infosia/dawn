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

#include "dawn/native/metal/BindGroupLayoutMTL.h"

#include <vector>

#include "dawn/common/MatchVariant.h"
#include "dawn/native/Device.h"
#include "dawn/native/metal/BindGroupMTL.h"
#include "dawn/native/metal/DeviceMTL.h"
#include "dawn/native/metal/UtilsMetal.h"

namespace dawn::native::metal {

namespace {

tint::BindingTypeBuffer ToTint(const BufferBindingInfo& binding) {
    switch (binding.type) {
        case wgpu::BufferBindingType::Uniform:
            return tint::BindingTypeBuffer::kUniform;
        case wgpu::BufferBindingType::Storage:
        case wgpu::BufferBindingType::ReadOnlyStorage:
        case kInternalStorageBufferBinding:
        case kInternalReadOnlyStorageBufferBinding:
            return tint::BindingTypeBuffer::kStorage;
        case wgpu::BufferBindingType::BindingNotUsed:
        case wgpu::BufferBindingType::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::BindingTypeSampler ToTint(const SamplerBindingInfo& binding) {
    switch (binding.type) {
        case wgpu::SamplerBindingType::Comparison:
            return tint::BindingTypeSampler::kComparison;
        case wgpu::SamplerBindingType::Filtering:
        case wgpu::SamplerBindingType::NonFiltering:
        case kUnknownFilteringSamplerBindingType:
            return tint::BindingTypeSampler::kNonComparison;
        case wgpu::SamplerBindingType::BindingNotUsed:
        case wgpu::SamplerBindingType::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::TextureViewDimension ToTint(wgpu::TextureViewDimension viewDimension) {
    switch (viewDimension) {
        case wgpu::TextureViewDimension::e1D:
            return tint::TextureViewDimension::k1d;
        case wgpu::TextureViewDimension::e2D:
            return tint::TextureViewDimension::k2d;
        case wgpu::TextureViewDimension::e2DArray:
            return tint::TextureViewDimension::k2dArray;
        case wgpu::TextureViewDimension::Cube:
            return tint::TextureViewDimension::kCube;
        case wgpu::TextureViewDimension::CubeArray:
            return tint::TextureViewDimension::kCubeArray;
        case wgpu::TextureViewDimension::e3D:
            return tint::TextureViewDimension::k3d;
        case wgpu::TextureViewDimension::Undefined:
        case kInternalInputAttachmentDim:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::BindingTypeTexture::SampleType ToTint(wgpu::TextureSampleType sampleType) {
    switch (sampleType) {
        case wgpu::TextureSampleType::Float:
        case wgpu::TextureSampleType::UnfilterableFloat:
        case kUnknownFilterableFloatSampleType:
        case kInternalResolveAttachmentSampleType:
            return tint::BindingTypeTexture::SampleType::kFloat;
        case wgpu::TextureSampleType::Depth:
            return tint::BindingTypeTexture::SampleType::kDepth;
        case wgpu::TextureSampleType::Sint:
            return tint::BindingTypeTexture::SampleType::kSInt;
        case wgpu::TextureSampleType::Uint:
            return tint::BindingTypeTexture::SampleType::kUInt;
        case wgpu::TextureSampleType::BindingNotUsed:
        case wgpu::TextureSampleType::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::BindingTypeTexture ToTint(const TextureBindingInfo& binding) {
    return {
        .multisampled = binding.multisampled,
        .viewDimension = ToTint(binding.viewDimension),
        .sampleType = ToTint(binding.sampleType),
    };
}

tint::BindingTypeStorageTexture::Format ToTint(wgpu::TextureFormat format) {
    switch (format) {
        using enum tint::BindingTypeStorageTexture::Format;
        case wgpu::TextureFormat::BGRA8Unorm:
            return kBgra8Unorm;
        case wgpu::TextureFormat::R16Float:
            return kR16Float;
        case wgpu::TextureFormat::R16Sint:
            return kR16Sint;
        case wgpu::TextureFormat::R16Snorm:
            return kR16Snorm;
        case wgpu::TextureFormat::R16Uint:
            return kR16Uint;
        case wgpu::TextureFormat::R16Unorm:
            return kR16Unorm;
        case wgpu::TextureFormat::R32Float:
            return kR32Float;
        case wgpu::TextureFormat::R32Sint:
            return kR32Sint;
        case wgpu::TextureFormat::R32Uint:
            return kR32Uint;
        case wgpu::TextureFormat::R8Sint:
            return kR8Sint;
        case wgpu::TextureFormat::R8Snorm:
            return kR8Snorm;
        case wgpu::TextureFormat::R8Uint:
            return kR8Uint;
        case wgpu::TextureFormat::R8Unorm:
            return kR8Unorm;
        case wgpu::TextureFormat::RG11B10Ufloat:
            return kRg11B10Ufloat;
        case wgpu::TextureFormat::RG16Float:
            return kRg16Float;
        case wgpu::TextureFormat::RG16Sint:
            return kRg16Sint;
        case wgpu::TextureFormat::RG16Snorm:
            return kRg16Snorm;
        case wgpu::TextureFormat::RG16Uint:
            return kRg16Uint;
        case wgpu::TextureFormat::RG16Unorm:
            return kRg16Unorm;
        case wgpu::TextureFormat::RG32Float:
            return kRg32Float;
        case wgpu::TextureFormat::RG32Sint:
            return kRg32Sint;
        case wgpu::TextureFormat::RG32Uint:
            return kRg32Uint;
        case wgpu::TextureFormat::RG8Sint:
            return kRg8Sint;
        case wgpu::TextureFormat::RG8Snorm:
            return kRg8Snorm;
        case wgpu::TextureFormat::RG8Uint:
            return kRg8Uint;
        case wgpu::TextureFormat::RG8Unorm:
            return kRg8Unorm;
        case wgpu::TextureFormat::RGB10A2Uint:
            return kRgb10A2Uint;
        case wgpu::TextureFormat::RGB10A2Unorm:
            return kRgb10A2Unorm;
        case wgpu::TextureFormat::RGBA16Float:
            return kRgba16Float;
        case wgpu::TextureFormat::RGBA16Sint:
            return kRgba16Sint;
        case wgpu::TextureFormat::RGBA16Snorm:
            return kRgba16Snorm;
        case wgpu::TextureFormat::RGBA16Uint:
            return kRgba16Uint;
        case wgpu::TextureFormat::RGBA16Unorm:
            return kRgba16Unorm;
        case wgpu::TextureFormat::RGBA32Float:
            return kRgba32Float;
        case wgpu::TextureFormat::RGBA32Sint:
            return kRgba32Sint;
        case wgpu::TextureFormat::RGBA32Uint:
            return kRgba32Uint;
        case wgpu::TextureFormat::RGBA8Sint:
            return kRgba8Sint;
        case wgpu::TextureFormat::RGBA8Snorm:
            return kRgba8Snorm;
        case wgpu::TextureFormat::RGBA8Uint:
            return kRgba8Uint;
        case wgpu::TextureFormat::RGBA8Unorm:
            return kRgba8Unorm;
        default:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::BindingTypeStorageTexture::Access ToTint(wgpu::StorageTextureAccess access) {
    switch (access) {
        case wgpu::StorageTextureAccess::WriteOnly:
            return tint::BindingTypeStorageTexture::Access::kWrite;
        case wgpu::StorageTextureAccess::ReadOnly:
            return tint::BindingTypeStorageTexture::Access::kRead;
        case wgpu::StorageTextureAccess::ReadWrite:
            return tint::BindingTypeStorageTexture::Access::kReadWrite;
        case wgpu::StorageTextureAccess::BindingNotUsed:
        case wgpu::StorageTextureAccess::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::BindingTypeStorageTexture ToTint(const StorageTextureBindingInfo& binding) {
    return {
        .viewDimension = ToTint(binding.viewDimension),
        .format = ToTint(binding.format),
        .access = ToTint(binding.access),
    };
}

}  // namespace

// static
Ref<BindGroupLayout> BindGroupLayout::Create(
    DeviceBase* device,
    const UnpackedPtr<BindGroupLayoutDescriptor>& descriptor) {
    return AcquireRef(new BindGroupLayout(device, descriptor));
}

BindGroupLayout::BindGroupLayout(DeviceBase* device,
                                 const UnpackedPtr<BindGroupLayoutDescriptor>& descriptor)
    : BindGroupLayoutInternalBase(device, descriptor),
      mBindGroupAllocator(MakeFrontendBindGroupAllocator<BindGroup>(4096)) {
    if (!device->IsToggleEnabled(Toggle::MetalUseArgumentBuffers)) {
        return;
    }

    @autoreleasepool {
        std::vector<MTLArgumentDescriptor*> descriptors;
        for (BindingIndex bindingIndex : Range(GetBindingCount())) {
            auto& bindingInfo = GetBindingInfo(bindingIndex);
            if (bindingInfo.indexInArray != BindingIndex{0}) {
                // Skip binding_array elements after the first one.
                continue;
            }

            MTLArgumentDescriptor* desc = [MTLArgumentDescriptor argumentDescriptor];
            desc.index = ToMTLArgumentBufferIndex(bindingIndex);
            desc.arrayLength = static_cast<uint32_t>(bindingInfo.arraySize);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
            // FIXME: Is this the access mode of the argbuf itself? Why would Metal need to know
            // that separately for each member? It's much more likely this is the access mode of the
            // resource in that slot... but then why don't the validation layers complain? And why
            // would the *encoder* need to know this, anyway?
            desc.access = MTLArgumentAccessReadOnly;
#pragma clang diagnostic pop

            // TODO(crbug.com/363031535): Handle more then uniform/storage.  (e.g. samplers,
            // textures, etc.)
            MatchVariant(
                bindingInfo.bindingLayout,
                [&](const BufferBindingInfo& layout) { desc.dataType = MTLDataTypePointer; },
                [&](const SamplerBindingInfo&) { desc.dataType = MTLDataTypeSampler; },
                [&](const StaticSamplerBindingInfo&) {
                    // Static samplers are handled in the frontend.
                    // TODO(crbug.com/dawn/2482): Implement static samplers in the
                    // Metal backend.
                    DAWN_CHECK(false);
                },
                // TODO(crbug.com/363031535): May need to set desc.textureType.
                [&](const TextureBindingInfo& layout) {
                    desc.dataType = MTLDataTypeTexture;
                    desc.textureType =
                        MetalTextureViewType(layout.viewDimension, layout.multisampled);
                },
                [&](const StorageTextureBindingInfo& layout) {
                    desc.dataType = MTLDataTypeTexture;
                    desc.textureType = MetalTextureViewType(layout.viewDimension, false);
                },
                [&](const TexelBufferBindingInfo&) { DAWN_CHECK(false); },
                [&](const InputAttachmentBindingInfo&) { DAWN_CHECK(false); },
                [&](const ExternalTextureBindingInfo&) { DAWN_CHECK(false); });

            descriptors.push_back(desc);
        }

        if (!descriptors.empty()) {
            NSArray* ary = [NSArray arrayWithObjects:descriptors.data() count:descriptors.size()];
            mArgumentEncoder = AcquireNSPRef(
                [ToBackend(device)->GetMTLDevice() newArgumentEncoderWithArguments:ary]);
        }
    }
}

BindGroupLayout::~BindGroupLayout() = default;

Ref<BindGroup> BindGroupLayout::AllocateBindGroup(
    Device* device,
    const UnpackedPtr<BindGroupDescriptor>& descriptor) {
    return AcquireRef(mBindGroupAllocator->Allocate(device, descriptor));
}

void BindGroupLayout::DeallocateBindGroup(BindGroup* bindGroup) {
    mBindGroupAllocator->Deallocate(bindGroup);
}

void BindGroupLayout::ReduceMemoryUsage() {
    mBindGroupAllocator->DeleteEmptySlabs();
}

NSPRef<id<MTLArgumentEncoder>> BindGroupLayout::GetArgumentEncoder() const {
    return mArgumentEncoder;
}

std::vector<tint::msl::writer::ArgumentBufferInfo::Entry>
BindGroupLayout::MakeTintArgumentBufferMembers() const {
    std::vector<tint::msl::writer::ArgumentBufferInfo::Entry> structMembers{};

    uint32_t curDynamicOffset = 0;

    for (const BindingIndex bindingIndex : Range(GetBindingCount())) {
        const BindingInfo& bindingInfo = GetBindingInfo(bindingIndex);
        if (bindingInfo.indexInArray > BindingIndex{0}) {
            // Skip binding_array elements after the first one.
            continue;
        }

        auto& member = structMembers.emplace_back(tint::msl::writer::ArgumentBufferInfo::Entry{
            .type = {},  // Initialized later
            .array_size = uint32_t{bindingInfo.arraySize},
            .first_id = uint32_t{bindingIndex},
        });
        MatchVariant(
            bindingInfo.bindingLayout,  //
            [&](const BufferBindingInfo& binding) {
                member.type = ToTint(binding);
                if (binding.hasDynamicOffset) {
                    member.first_offset_index = curDynamicOffset;
                    curDynamicOffset += uint32_t{bindingInfo.arraySize};
                }
            },
            [&](const SamplerBindingInfo& binding) { member.type = ToTint(binding); },
            [&](const StaticSamplerBindingInfo&) { DAWN_CHECK(false); },
            [&](const TextureBindingInfo& binding) { member.type = ToTint(binding); },
            [&](const TexelBufferBindingInfo&) {},
            [&](const StorageTextureBindingInfo& binding) { member.type = ToTint(binding); },
            [](const InputAttachmentBindingInfo&) { DAWN_CHECK(false); },
            [](const ExternalTextureBindingInfo&) { DAWN_CHECK(false); });
    }

    return structMembers;
}

}  // namespace dawn::native::metal
