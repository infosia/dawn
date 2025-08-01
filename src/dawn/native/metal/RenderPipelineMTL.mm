// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/metal/RenderPipelineMTL.h"

#include "absl/strings/escaping.h"
#include "dawn/common/Log.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/AsyncTask.h"
#include "dawn/native/CreatePipelineAsyncEvent.h"
#include "dawn/native/Instance.h"
#include "dawn/native/metal/BackendMTL.h"
#include "dawn/native/metal/DeviceMTL.h"
#include "dawn/native/metal/PipelineLayoutMTL.h"
#include "dawn/native/metal/ShaderModuleMTL.h"
#include "dawn/native/metal/TextureMTL.h"
#include "dawn/native/metal/UtilsMetal.h"
#include "dawn/native/stream/Stream.h"
#include "dawn/native/utils/WGPUHelpers.h"
#include "dawn/platform/metrics/HistogramMacros.h"
#include "dawn/platform/tracing/TraceEvent.h"

namespace dawn::native::metal {

namespace {
MTLVertexFormat VertexFormatType(wgpu::VertexFormat format) {
    switch (format) {
        case wgpu::VertexFormat::Uint8:
            return MTLVertexFormatUChar;
        case wgpu::VertexFormat::Uint8x2:
            return MTLVertexFormatUChar2;
        case wgpu::VertexFormat::Uint8x4:
            return MTLVertexFormatUChar4;
        case wgpu::VertexFormat::Sint8:
            return MTLVertexFormatChar;
        case wgpu::VertexFormat::Sint8x2:
            return MTLVertexFormatChar2;
        case wgpu::VertexFormat::Sint8x4:
            return MTLVertexFormatChar4;
        case wgpu::VertexFormat::Unorm8:
            return MTLVertexFormatUCharNormalized;
        case wgpu::VertexFormat::Unorm8x2:
            return MTLVertexFormatUChar2Normalized;
        case wgpu::VertexFormat::Unorm8x4:
            return MTLVertexFormatUChar4Normalized;
        case wgpu::VertexFormat::Snorm8:
            return MTLVertexFormatCharNormalized;
        case wgpu::VertexFormat::Snorm8x2:
            return MTLVertexFormatChar2Normalized;
        case wgpu::VertexFormat::Snorm8x4:
            return MTLVertexFormatChar4Normalized;
        case wgpu::VertexFormat::Uint16:
            return MTLVertexFormatUShort;
        case wgpu::VertexFormat::Uint16x2:
            return MTLVertexFormatUShort2;
        case wgpu::VertexFormat::Uint16x4:
            return MTLVertexFormatUShort4;
        case wgpu::VertexFormat::Sint16:
            return MTLVertexFormatShort;
        case wgpu::VertexFormat::Sint16x2:
            return MTLVertexFormatShort2;
        case wgpu::VertexFormat::Sint16x4:
            return MTLVertexFormatShort4;
        case wgpu::VertexFormat::Unorm16:
            return MTLVertexFormatUShortNormalized;
        case wgpu::VertexFormat::Unorm16x2:
            return MTLVertexFormatUShort2Normalized;
        case wgpu::VertexFormat::Unorm16x4:
            return MTLVertexFormatUShort4Normalized;
        case wgpu::VertexFormat::Snorm16:
            return MTLVertexFormatShortNormalized;
        case wgpu::VertexFormat::Snorm16x2:
            return MTLVertexFormatShort2Normalized;
        case wgpu::VertexFormat::Snorm16x4:
            return MTLVertexFormatShort4Normalized;
        case wgpu::VertexFormat::Float16:
            return MTLVertexFormatHalf;
        case wgpu::VertexFormat::Float16x2:
            return MTLVertexFormatHalf2;
        case wgpu::VertexFormat::Float16x4:
            return MTLVertexFormatHalf4;
        case wgpu::VertexFormat::Float32:
            return MTLVertexFormatFloat;
        case wgpu::VertexFormat::Float32x2:
            return MTLVertexFormatFloat2;
        case wgpu::VertexFormat::Float32x3:
            return MTLVertexFormatFloat3;
        case wgpu::VertexFormat::Float32x4:
            return MTLVertexFormatFloat4;
        case wgpu::VertexFormat::Uint32:
            return MTLVertexFormatUInt;
        case wgpu::VertexFormat::Uint32x2:
            return MTLVertexFormatUInt2;
        case wgpu::VertexFormat::Uint32x3:
            return MTLVertexFormatUInt3;
        case wgpu::VertexFormat::Uint32x4:
            return MTLVertexFormatUInt4;
        case wgpu::VertexFormat::Sint32:
            return MTLVertexFormatInt;
        case wgpu::VertexFormat::Sint32x2:
            return MTLVertexFormatInt2;
        case wgpu::VertexFormat::Sint32x3:
            return MTLVertexFormatInt3;
        case wgpu::VertexFormat::Sint32x4:
            return MTLVertexFormatInt4;
        case wgpu::VertexFormat::Unorm10_10_10_2:
            return MTLVertexFormatUInt1010102Normalized;
        case wgpu::VertexFormat::Unorm8x4BGRA:
            return MTLVertexFormatUChar4Normalized_BGRA;
        default:
            DAWN_UNREACHABLE();
    }
}

MTLVertexStepFunction VertexStepModeFunction(wgpu::VertexStepMode mode) {
    switch (mode) {
        case wgpu::VertexStepMode::Vertex:
            return MTLVertexStepFunctionPerVertex;
        case wgpu::VertexStepMode::Instance:
            return MTLVertexStepFunctionPerInstance;
        case wgpu::VertexStepMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLPrimitiveType MTLPrimitiveTopology(wgpu::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
        case wgpu::PrimitiveTopology::PointList:
            return MTLPrimitiveTypePoint;
        case wgpu::PrimitiveTopology::LineList:
            return MTLPrimitiveTypeLine;
        case wgpu::PrimitiveTopology::LineStrip:
            return MTLPrimitiveTypeLineStrip;
        case wgpu::PrimitiveTopology::TriangleList:
            return MTLPrimitiveTypeTriangle;
        case wgpu::PrimitiveTopology::TriangleStrip:
            return MTLPrimitiveTypeTriangleStrip;
        case wgpu::PrimitiveTopology::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLPrimitiveTopologyClass MTLInputPrimitiveTopology(wgpu::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
        case wgpu::PrimitiveTopology::PointList:
            return MTLPrimitiveTopologyClassPoint;
        case wgpu::PrimitiveTopology::LineList:
        case wgpu::PrimitiveTopology::LineStrip:
            return MTLPrimitiveTopologyClassLine;
        case wgpu::PrimitiveTopology::TriangleList:
        case wgpu::PrimitiveTopology::TriangleStrip:
            return MTLPrimitiveTopologyClassTriangle;
        case wgpu::PrimitiveTopology::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLBlendFactor MetalBlendFactor(wgpu::BlendFactor factor, bool alpha) {
    switch (factor) {
        case wgpu::BlendFactor::Zero:
            return MTLBlendFactorZero;
        case wgpu::BlendFactor::One:
            return MTLBlendFactorOne;
        case wgpu::BlendFactor::Src:
            return MTLBlendFactorSourceColor;
        case wgpu::BlendFactor::OneMinusSrc:
            return MTLBlendFactorOneMinusSourceColor;
        case wgpu::BlendFactor::SrcAlpha:
            return MTLBlendFactorSourceAlpha;
        case wgpu::BlendFactor::OneMinusSrcAlpha:
            return MTLBlendFactorOneMinusSourceAlpha;
        case wgpu::BlendFactor::Dst:
            return MTLBlendFactorDestinationColor;
        case wgpu::BlendFactor::OneMinusDst:
            return MTLBlendFactorOneMinusDestinationColor;
        case wgpu::BlendFactor::DstAlpha:
            return MTLBlendFactorDestinationAlpha;
        case wgpu::BlendFactor::OneMinusDstAlpha:
            return MTLBlendFactorOneMinusDestinationAlpha;
        case wgpu::BlendFactor::SrcAlphaSaturated:
            return MTLBlendFactorSourceAlphaSaturated;
        case wgpu::BlendFactor::Constant:
            return alpha ? MTLBlendFactorBlendAlpha : MTLBlendFactorBlendColor;
        case wgpu::BlendFactor::OneMinusConstant:
            return alpha ? MTLBlendFactorOneMinusBlendAlpha : MTLBlendFactorOneMinusBlendColor;
        case wgpu::BlendFactor::Src1:
            return MTLBlendFactorSource1Color;
        case wgpu::BlendFactor::OneMinusSrc1:
            return MTLBlendFactorOneMinusSource1Color;
        case wgpu::BlendFactor::Src1Alpha:
            return MTLBlendFactorSource1Alpha;
        case wgpu::BlendFactor::OneMinusSrc1Alpha:
            return MTLBlendFactorOneMinusSource1Alpha;
        case wgpu::BlendFactor::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLBlendOperation MetalBlendOperation(wgpu::BlendOperation operation) {
    switch (operation) {
        case wgpu::BlendOperation::Add:
            return MTLBlendOperationAdd;
        case wgpu::BlendOperation::Subtract:
            return MTLBlendOperationSubtract;
        case wgpu::BlendOperation::ReverseSubtract:
            return MTLBlendOperationReverseSubtract;
        case wgpu::BlendOperation::Min:
            return MTLBlendOperationMin;
        case wgpu::BlendOperation::Max:
            return MTLBlendOperationMax;
        case wgpu::BlendOperation::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLColorWriteMask MetalColorWriteMask(wgpu::ColorWriteMask writeMask,
                                      bool isDeclaredInFragmentShader) {
    if (!isDeclaredInFragmentShader) {
        return MTLColorWriteMaskNone;
    }

    MTLColorWriteMask mask = MTLColorWriteMaskNone;

    if (writeMask & wgpu::ColorWriteMask::Red) {
        mask |= MTLColorWriteMaskRed;
    }
    if (writeMask & wgpu::ColorWriteMask::Green) {
        mask |= MTLColorWriteMaskGreen;
    }
    if (writeMask & wgpu::ColorWriteMask::Blue) {
        mask |= MTLColorWriteMaskBlue;
    }
    if (writeMask & wgpu::ColorWriteMask::Alpha) {
        mask |= MTLColorWriteMaskAlpha;
    }

    return mask;
}

void ComputeBlendDesc(MTLRenderPipelineColorAttachmentDescriptor* attachment,
                      const ColorTargetState* state,
                      bool isDeclaredInFragmentShader,
                      CacheKey* cacheKey) {
    attachment.blendingEnabled = state->blend != nullptr;
    StreamIn(cacheKey, attachment.blendingEnabled);

    if (attachment.blendingEnabled) {
        attachment.sourceRGBBlendFactor = MetalBlendFactor(state->blend->color.srcFactor, false);
        attachment.destinationRGBBlendFactor =
            MetalBlendFactor(state->blend->color.dstFactor, false);
        attachment.rgbBlendOperation = MetalBlendOperation(state->blend->color.operation);
        attachment.sourceAlphaBlendFactor = MetalBlendFactor(state->blend->alpha.srcFactor, true);
        attachment.destinationAlphaBlendFactor =
            MetalBlendFactor(state->blend->alpha.dstFactor, true);
        attachment.alphaBlendOperation = MetalBlendOperation(state->blend->alpha.operation);

        StreamIn(cacheKey, attachment.sourceRGBBlendFactor, attachment.destinationRGBBlendFactor,
                 attachment.rgbBlendOperation, attachment.sourceAlphaBlendFactor,
                 attachment.destinationAlphaBlendFactor, attachment.alphaBlendOperation);
    }

    attachment.writeMask = MetalColorWriteMask(state->writeMask, isDeclaredInFragmentShader);
    StreamIn(cacheKey, attachment.writeMask);
}

MTLStencilOperation MetalStencilOperation(wgpu::StencilOperation stencilOperation) {
    switch (stencilOperation) {
        case wgpu::StencilOperation::Keep:
            return MTLStencilOperationKeep;
        case wgpu::StencilOperation::Zero:
            return MTLStencilOperationZero;
        case wgpu::StencilOperation::Replace:
            return MTLStencilOperationReplace;
        case wgpu::StencilOperation::Invert:
            return MTLStencilOperationInvert;
        case wgpu::StencilOperation::IncrementClamp:
            return MTLStencilOperationIncrementClamp;
        case wgpu::StencilOperation::DecrementClamp:
            return MTLStencilOperationDecrementClamp;
        case wgpu::StencilOperation::IncrementWrap:
            return MTLStencilOperationIncrementWrap;
        case wgpu::StencilOperation::DecrementWrap:
            return MTLStencilOperationDecrementWrap;
        case wgpu::StencilOperation::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLWinding MTLFrontFace(wgpu::FrontFace face) {
    switch (face) {
        case wgpu::FrontFace::CW:
            return MTLWindingClockwise;
        case wgpu::FrontFace::CCW:
            return MTLWindingCounterClockwise;
        case wgpu::FrontFace::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

MTLCullMode ToMTLCullMode(wgpu::CullMode mode) {
    switch (mode) {
        case wgpu::CullMode::None:
            return MTLCullModeNone;
        case wgpu::CullMode::Front:
            return MTLCullModeFront;
        case wgpu::CullMode::Back:
            return MTLCullModeBack;
        case wgpu::CullMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

// Build a mapping of vertex buffer slots to packed indices
PerVertexBuffer<NSUInteger> ComputeMetalVertexBufferIndices(PipelineLayoutBase* layout,
                                                            VertexBufferMask vertexBuffersUsed) {
    // Vertex buffers are placed after all the buffers for the bind groups.
    uint32_t vertexBufferIndex =
        ToBackend(layout)->GetBufferBindingCount(SingleShaderStage::Vertex);
    PerVertexBuffer<NSUInteger> vertexBufferIndices;
    for (VertexBufferSlot slot : vertexBuffersUsed) {
        vertexBufferIndices[slot] = vertexBufferIndex;
        vertexBufferIndex++;
    }
    return vertexBufferIndices;
}

}  // anonymous namespace

// static
Ref<RenderPipelineBase> RenderPipeline::CreateUninitialized(
    Device* device,
    const UnpackedPtr<RenderPipelineDescriptor>& descriptor) {
    return AcquireRef(new RenderPipeline(device, descriptor));
}

RenderPipeline::RenderPipeline(DeviceBase* device,
                               const UnpackedPtr<RenderPipelineDescriptor>& desc)
    : RenderPipelineBase(device, desc),
      mMtlPrimitiveTopology(MTLPrimitiveTopology(GetPrimitiveTopology())),
      mMtlFrontFace(MTLFrontFace(GetFrontFace())),
      mMtlCullMode(ToMTLCullMode(GetCullMode())),
      mMtlVertexBufferIndices(
          ComputeMetalVertexBufferIndices(GetLayout(), GetVertexBuffersUsed())) {}

RenderPipeline::~RenderPipeline() = default;

MaybeError RenderPipeline::FinishInitialize() {
    TRACE_EVENT1(GetDevice()->GetPlatform(), General, "metal::RenderPipeline::FinishInitialize",
                 "label", utils::GetLabelForTrace(GetLabel()));

    auto mtlDevice = ToBackend(GetDevice())->GetMTLDevice();

    NSRef<MTLRenderPipelineDescriptor> descriptorMTLRef =
        AcquireNSRef([MTLRenderPipelineDescriptor new]);
    MTLRenderPipelineDescriptor* descriptorMTL = descriptorMTLRef.Get();

    NSRef<NSString> label = MakeDebugName(GetDevice(), "Dawn_RenderPipeline", GetLabel());
    descriptorMTL.label = label.Get();

    // Only put this flag on if the feature is enabled because it may have a performance cost.
    descriptorMTL.supportIndirectCommandBuffers =
        GetDevice()->HasFeature(Feature::MultiDrawIndirect);
    StreamIn(&mCacheKey, descriptorMTL.supportIndirectCommandBuffers);

    NSRef<MTLVertexDescriptor> vertexDesc;
    if (GetDevice()->IsToggleEnabled(Toggle::MetalEnableVertexPulling)) {
        StreamIn(&mCacheKey, false);
        vertexDesc = AcquireNSRef([MTLVertexDescriptor new]);
    } else {
        StreamIn(&mCacheKey, true);
        vertexDesc = MakeVertexDesc(&mCacheKey);
    }
    descriptorMTL.vertexDescriptor = vertexDesc.Get();

    DAWN_ASSERT(mVertexFunctionData);
    descriptorMTL.vertexFunction = mVertexFunctionData->function.Get();
    if (mVertexFunctionData->needsStorageBufferLength) {
        mStagesRequiringStorageBufferLength |= wgpu::ShaderStage::Vertex;
    }

    StreamIn(&mCacheKey, GetStage(SingleShaderStage::Vertex).module->GetHash());

    if (HasStage(SingleShaderStage::Fragment)) {
        DAWN_ASSERT(mFragmentFunctionData);

        descriptorMTL.fragmentFunction = mFragmentFunctionData->function.Get();
        if (mFragmentFunctionData->needsStorageBufferLength) {
            mStagesRequiringStorageBufferLength |= wgpu::ShaderStage::Fragment;
        }

        const ProgrammableStage& fragmentStage = GetStage(SingleShaderStage::Fragment);
        StreamIn(&mCacheKey, fragmentStage.module->GetHash());

        const auto& fragmentOutputMask = fragmentStage.metadata->fragmentOutputMask;

        for (ColorAttachmentIndex i : GetColorAttachmentsMask()) {
            NSUInteger metalIndex = static_cast<NSUInteger>(i);

            descriptorMTL.colorAttachments[metalIndex].pixelFormat =
                MetalPixelFormat(GetDevice(), GetColorAttachmentFormat(i));

            StreamIn(&mCacheKey, metalIndex,
                     descriptorMTL.colorAttachments[metalIndex].pixelFormat);

            ComputeBlendDesc(descriptorMTL.colorAttachments[metalIndex], GetColorTargetState(i),
                             fragmentOutputMask[i], &mCacheKey);
        }

        const bool hasPixelLocalStorage = GetAttachmentState()->HasPixelLocalStorage();
        StreamIn(&mCacheKey, hasPixelLocalStorage);

        if (hasPixelLocalStorage) {
            const std::vector<wgpu::TextureFormat>& storageAttachmentSlots =
                GetAttachmentState()->GetStorageAttachmentSlots();
            std::vector<ColorAttachmentIndex> storageAttachmentPacking =
                GetAttachmentState()->ComputeStorageAttachmentPackingInColorAttachments();

            for (size_t i = 0; i < storageAttachmentSlots.size(); i++) {
                const NSUInteger metalIndex = static_cast<NSUInteger>(storageAttachmentPacking[i]);
                if (storageAttachmentSlots[i] == wgpu::TextureFormat::Undefined) {
                    descriptorMTL.colorAttachments[metalIndex].pixelFormat =
                        MetalPixelFormat(GetDevice(), kImplicitPLSSlotFormat);
                } else {
                    descriptorMTL.colorAttachments[metalIndex].pixelFormat =
                        MetalPixelFormat(GetDevice(), storageAttachmentSlots[i]);
                }
                StreamIn(&mCacheKey, metalIndex,
                         descriptorMTL.colorAttachments[metalIndex].pixelFormat);
            }
        }
    }

    const bool hasDepthStencilAttachment = HasDepthStencilAttachment();
    StreamIn(&mCacheKey, hasDepthStencilAttachment);

    if (hasDepthStencilAttachment) {
        wgpu::TextureFormat depthStencilFormat = GetDepthStencilFormat();
        MTLPixelFormat metalFormat = MetalPixelFormat(GetDevice(), depthStencilFormat);

        if (GetDevice()->IsToggleEnabled(
                Toggle::MetalUseBothDepthAndStencilAttachmentsForCombinedDepthStencilFormats)) {
            if (GetDepthStencilAspects(metalFormat) & Aspect::Depth) {
                descriptorMTL.depthAttachmentPixelFormat = metalFormat;
            }
            if (GetDepthStencilAspects(metalFormat) & Aspect::Stencil) {
                descriptorMTL.stencilAttachmentPixelFormat = metalFormat;
            }
        } else {
            const Format& internalFormat = GetDevice()->GetValidInternalFormat(depthStencilFormat);
            if (internalFormat.HasDepth()) {
                descriptorMTL.depthAttachmentPixelFormat = metalFormat;
            }
            if (internalFormat.HasStencil()) {
                descriptorMTL.stencilAttachmentPixelFormat = metalFormat;
            }
        }

        StreamIn(&mCacheKey, descriptorMTL.depthAttachmentPixelFormat);
    }

    descriptorMTL.inputPrimitiveTopology = MTLInputPrimitiveTopology(GetPrimitiveTopology());
    descriptorMTL.rasterSampleCount = GetSampleCount();
    descriptorMTL.alphaToCoverageEnabled = IsAlphaToCoverageEnabled();

    StreamIn(&mCacheKey, descriptorMTL.inputPrimitiveTopology, descriptorMTL.rasterSampleCount,
             descriptorMTL.alphaToCoverageEnabled);

    platform::metrics::DawnHistogramTimer timer(GetDevice()->GetPlatform());

    NSRef<NSString> homeDir = NSHomeDirectory();
    NSRef<NSString> bundleId = [[NSBundle mainBundle] bundleIdentifier];

    NSRef<NSString> shaderCachePath = [NSString
        pathWithComponents:@[ *homeDir, @"/Library/Caches", *bundleId, @"/DawnMetalShaderCache" ]];

    NSError* error = nullptr;

    if (![[NSFileManager defaultManager] createDirectoryAtPath:*shaderCachePath
                                   withIntermediateDirectories:YES
                                                    attributes:nil
                                                         error:&error]) {
        ErrorLog() << "RenderPipelineMTL: failed to create shader cache directory at path "
                   << [*shaderCachePath UTF8String]
                   << " with error: " << [error.localizedDescription UTF8String];
    }

    Sha3_224::Output hash = Sha3_224::Hash(mCacheKey.data(), mCacheKey.size());
    std::string_view hashBytesView(reinterpret_cast<const char*>(hash.data()), hash.size());

    NSRef<NSString> shaderBinaryFilename =
        [NSString stringWithUTF8String:absl::BytesToHexString(hashBytesView).c_str()];

    NSRef<NSString> shaderBinaryPath =
        [[NSString pathWithComponents:@[ *shaderCachePath, *shaderBinaryFilename ]]
            stringByAppendingPathExtension:@"binary.metallib"];

    NSRef<NSURL> shaderBinaryURL = [NSURL fileURLWithPath:*shaderBinaryPath];

    auto loadFromBinaryArchive = [&] {
        TRACE_EVENT0(GetDevice()->GetPlatform(), General, "loadFromBinaryArchive");
        if ([[NSFileManager defaultManager] fileExistsAtPath:*shaderBinaryPath]) {
            NSRef<MTLBinaryArchiveDescriptor> binaryArchiveDesc =
                AcquireNSRef([[MTLBinaryArchiveDescriptor alloc] init]);
            (*binaryArchiveDesc).url = *shaderBinaryURL;

            NSPRef<id<MTLBinaryArchive>> binaryArchive;
            {
                TRACE_EVENT0(GetDevice()->GetPlatform(), General,
                             "MTLDevice newBinaryArchiveWithDescriptor(URL)");
                binaryArchive =
                    AcquireNSPRef([mtlDevice newBinaryArchiveWithDescriptor:*binaryArchiveDesc
                                                                      error:&error]);
            }
            if (!binaryArchive) {
                ErrorLog() << "RenderPipelineMTL: failed to load binary archive at path "
                           << [*shaderBinaryPath UTF8String]
                           << " with error: " << [error.localizedDescription UTF8String];
                return;
            }

            descriptorMTL.binaryArchives = @[ *binaryArchive ];

            {
                TRACE_EVENT0(GetDevice()->GetPlatform(), General,
                             "MTLDevice newRenderPipelineStateWithDescriptor(binaryArchive)");
                mMtlRenderPipelineState = AcquireNSPRef([mtlDevice
                    newRenderPipelineStateWithDescriptor:descriptorMTL
                                                 options:MTLPipelineOptionFailOnBinaryArchiveMiss
                                              reflection:nullptr
                                                   error:&error]);
            }
            if (!mMtlRenderPipelineState) {
                ErrorLog() << "RenderPipelineMTL: failed to create render pipeline state from "
                              "binary archive at path "
                           << [*shaderBinaryPath UTF8String]
                           << " with error: " << [error.localizedDescription UTF8String];
                descriptorMTL.binaryArchives = @[];
                return;
            }

            timer.RecordMicroseconds("Metal.newRenderPipelineStateWithDescriptor.CacheHit");
        } else {
            ErrorLog() << "RenderPipelineMTL: could not find cached binary archive at path "
                       << [*shaderBinaryPath UTF8String];
        }
    };
    loadFromBinaryArchive();

    if (!mMtlRenderPipelineState) {
        auto serializeToBinaryArchive = [&] {
            TRACE_EVENT0(GetDevice()->GetPlatform(), General, "serializeToBinaryArchive");
            if ([[NSFileManager defaultManager] fileExistsAtPath:*shaderBinaryPath] &&
                ![[NSFileManager defaultManager] removeItemAtPath:*shaderBinaryPath error:&error]) {
                ErrorLog() << "RenderPipelineMTL: failed to delete stale binary archive at path "
                           << [*shaderBinaryPath UTF8String]
                           << " with error: " << [error.localizedDescription UTF8String];
                return;
            }

            NSRef<MTLBinaryArchiveDescriptor> binaryArchiveDesc =
                AcquireNSRef([[MTLBinaryArchiveDescriptor alloc] init]);

            NSPRef<id<MTLBinaryArchive>> binaryArchive =
                AcquireNSPRef([mtlDevice newBinaryArchiveWithDescriptor:*binaryArchiveDesc
                                                                  error:&error]);
            if (!binaryArchive) {
                ErrorLog() << "RenderPipelineMTL: failed to create new binary archive with error: "
                           << [error.localizedDescription UTF8String];
                return;
            }

            {
                TRACE_EVENT0(GetDevice()->GetPlatform(), General,
                             "MTLBinaryArchive addRenderPipelineFunctionsWithDescriptor");
                if (![*binaryArchive addRenderPipelineFunctionsWithDescriptor:descriptorMTL
                                                                        error:&error]) {
                    ErrorLog() << "RenderPipelineMTL: failed to add render pipeline state to new "
                                  "binary archive with error: "
                               << [error.localizedDescription UTF8String];
                    return;
                }
            }

            auto serializeToURLTask = [device = GetDevice(), archive = std::move(binaryArchive),
                                       binaryURL = shaderBinaryURL, label = GetLabel()]() mutable {
                TRACE_EVENT1(device->GetPlatform(), General, "MTLBinaryArchive serializeToURL",
                             "label", utils::GetLabelForTrace(label));
                NSError* error = nullptr;
                if (![*archive serializeToURL:*binaryURL error:&error]) {
                    ErrorLog() << "RenderPipelineMTL: failed to save binary archive to path "
                               << [(*binaryURL).path UTF8String]
                               << " with error: " << [error.localizedDescription UTF8String];
                }
            };

            GetDevice()->GetAsyncTaskManager()->PostBackgroundTask(std::move(serializeToURLTask));
        };
        serializeToBinaryArchive();

        {
            TRACE_EVENT0(GetDevice()->GetPlatform(), General,
                         "MTLDevice newRenderPipelineStateWithDescriptor");
            mMtlRenderPipelineState =
                AcquireNSPRef([mtlDevice newRenderPipelineStateWithDescriptor:descriptorMTL
                                                                        error:&error]);
        }
        if (!mMtlRenderPipelineState) {
            std::string errorMessage = absl::StrFormat(
                "RenderPipelineMTL: error creating pipeline state: %s from vertex MSL:\n\n%s",
                [error.localizedDescription UTF8String], [*mVertexFunctionData->msl UTF8String]);
            if (GetStageMask() & wgpu::ShaderStage::Fragment) {
                absl::StrAppendFormat(&errorMessage, "\n\nand fragment MSL:\n\n%s",
                                      [*mFragmentFunctionData->msl UTF8String]);
            }
            return DAWN_INTERNAL_ERROR(errorMessage);
        }

        timer.RecordMicroseconds("Metal.newRenderPipelineStateWithDescriptor.CacheMiss");
    }
    // Create depth stencil state and cache it, fetch the cached depth stencil state when we
    // call setDepthStencilState() for a given render pipeline in CommandEncoder, in order
    // to improve performance.
    NSRef<MTLDepthStencilDescriptor> depthStencilDesc = MakeDepthStencilDesc();
    mMtlDepthStencilState =
        AcquireNSPRef([mtlDevice newDepthStencilStateWithDescriptor:depthStencilDesc.Get()]);

    mVertexFunctionData.reset();
    mFragmentFunctionData.reset();

    return {};
}

MaybeError RenderPipeline::InitializeImpl() {
    DAWN_TRY_ASSIGN_CONTEXT(mVertexFunctionData,
                            LoadShaderEntryPoint(this, SingleShaderStage::Vertex),
                            " getting vertex MTLFunction for %s", this);

    if (HasStage(SingleShaderStage::Fragment)) {
        DAWN_TRY_ASSIGN_CONTEXT(mFragmentFunctionData,
                                LoadShaderEntryPoint(this, SingleShaderStage::Fragment),
                                " getting fragment MTLFunction for %s", this);
    }

    return FinishInitialize();
}

void RenderPipeline::OnVertexShaderFunctionLoaded(ResultOrError<MetalFunctionData> result) {
    if (result.IsError() && mPendingInitializeCallback) {
        mPendingInitializeCallback(result.AcquireError());
        mPendingInitializeCallback = {};
    }
    mVertexFunctionData = result.AcquireSuccess();
    if (!HasStage(SingleShaderStage::Fragment) || mFragmentFunctionData) {
        mPendingInitializeCallback(FinishInitialize());
        mPendingInitializeCallback = {};
    }
}

void RenderPipeline::OnFragmentShaderFunctionLoaded(ResultOrError<MetalFunctionData> result) {
    if (result.IsError() && mPendingInitializeCallback) {
        mPendingInitializeCallback(result.AcquireError());
        mPendingInitializeCallback = {};
    }
    mFragmentFunctionData = result.AcquireSuccess();
    if (mVertexFunctionData) {
        mPendingInitializeCallback(FinishInitialize());
        mPendingInitializeCallback = {};
    }
}

void RenderPipeline::InitializeAsync(ScopedUseShaderPrograms scopedUseShaderPrograms,
                                     std::function<void(MaybeError)> callback) {
    Device* device = ToBackend(GetDevice());

    auto maybeError = [&]() -> MaybeError {
        DAWN_TRY_ASSIGN(mVertexShaderLibrary,
                        device->GetOrCreateMetalShaderLibrary(this, SingleShaderStage::Vertex));
        if (GetStageMask() & wgpu::ShaderStage::Fragment) {
            DAWN_TRY_ASSIGN(mFragmentShaderLibrary, device->GetOrCreateMetalShaderLibrary(
                                                        this, SingleShaderStage::Fragment));
        }
        return {};
    }();

    if (maybeError.IsError()) {
        callback(std::move(maybeError));
        return;
    }

    mPendingInitializeCallback = std::move(callback);

    auto vertexShaderCallback =
        [pipeline = Ref<RenderPipeline>(this)](ResultOrError<MetalFunctionData> result) {
            pipeline->OnVertexShaderFunctionLoaded(std::move(result));
        };

    auto fragmentShaderCallback =
        [pipeline = Ref<RenderPipeline>(this)](ResultOrError<MetalFunctionData> result) {
            pipeline->OnFragmentShaderFunctionLoaded(std::move(result));
        };

    mVertexShaderLibrary->LoadEntryPoint(
        std::move(scopedUseShaderPrograms[SingleShaderStage::Vertex]),
        std::move(vertexShaderCallback));

    if (mFragmentShaderLibrary) {
        mFragmentShaderLibrary->LoadEntryPoint(
            std::move(scopedUseShaderPrograms[SingleShaderStage::Fragment]),
            std::move(fragmentShaderCallback));
    }
}

MTLPrimitiveType RenderPipeline::GetMTLPrimitiveTopology() const {
    return mMtlPrimitiveTopology;
}

MTLWinding RenderPipeline::GetMTLFrontFace() const {
    return mMtlFrontFace;
}

MTLCullMode RenderPipeline::GetMTLCullMode() const {
    return mMtlCullMode;
}

void RenderPipeline::Encode(id<MTLRenderCommandEncoder> encoder) {
    [encoder setRenderPipelineState:mMtlRenderPipelineState.Get()];
}

id<MTLDepthStencilState> RenderPipeline::GetMTLDepthStencilState() {
    return mMtlDepthStencilState.Get();
}

NSUInteger RenderPipeline::GetMtlVertexBufferIndex(VertexBufferSlot slot) const {
    DAWN_ASSERT(slot < kMaxVertexBuffersTyped);
    return mMtlVertexBufferIndices[slot];
}

wgpu::ShaderStage RenderPipeline::GetStagesRequiringStorageBufferLength() const {
    return mStagesRequiringStorageBufferLength;
}

NSRef<MTLVertexDescriptor> RenderPipeline::MakeVertexDesc(CacheKey* cacheKey) const {
    MTLVertexDescriptor* mtlVertexDescriptor = [MTLVertexDescriptor new];

    for (VertexBufferSlot slot : GetVertexBuffersUsed()) {
        const VertexBufferInfo& info = GetVertexBuffer(slot);

        NSRef<MTLVertexBufferLayoutDescriptor> layoutDesc =
            AcquireNSRef([MTLVertexBufferLayoutDescriptor new]);
        if (info.arrayStride == 0) {
            // For MTLVertexStepFunctionConstant, the stepRate must be 0,
            // but the arrayStride must NOT be 0, so we made up it with
            // max(attrib.offset + sizeof(attrib) for each attrib)
            size_t maxArrayStride = 0;
            for (VertexAttributeLocation loc : GetAttributeLocationsUsed()) {
                const VertexAttributeInfo& attrib = GetAttribute(loc);
                // Only use the attributes that use the current input
                if (attrib.vertexBufferSlot != slot) {
                    continue;
                }
                maxArrayStride =
                    std::max(maxArrayStride,
                             GetVertexFormatInfo(attrib.format).byteSize + size_t(attrib.offset));
            }
            (*layoutDesc).stepFunction = MTLVertexStepFunctionConstant;
            (*layoutDesc).stepRate = 0;
            // Metal requires the stride must be a multiple of 4 bytes, align it with next
            // multiple of 4 if it's not.
            (*layoutDesc).stride = Align(maxArrayStride, 4);
        } else {
            (*layoutDesc).stepFunction = VertexStepModeFunction(info.stepMode);
            (*layoutDesc).stepRate = 1;
            (*layoutDesc).stride = info.arrayStride;
        }

        const NSUInteger metalIndex = GetMtlVertexBufferIndex(slot);

        StreamIn(cacheKey, metalIndex);
        StreamIn(cacheKey, (*layoutDesc).stepFunction, (*layoutDesc).stepRate,
                 (*layoutDesc).stride);

        mtlVertexDescriptor.layouts[metalIndex] = *layoutDesc;
    }

    for (VertexAttributeLocation loc : GetAttributeLocationsUsed()) {
        const VertexAttributeInfo& info = GetAttribute(loc);

        NSRef<MTLVertexAttributeDescriptor> attribDesc =
            AcquireNSRef([MTLVertexAttributeDescriptor new]);
        (*attribDesc).format = VertexFormatType(info.format);
        (*attribDesc).offset = info.offset;
        (*attribDesc).bufferIndex = GetMtlVertexBufferIndex(info.vertexBufferSlot);

        const NSUInteger metalIndex = static_cast<NSUInteger>(loc);

        StreamIn(cacheKey, metalIndex);
        StreamIn(cacheKey, (*attribDesc).format, (*attribDesc).offset, (*attribDesc).bufferIndex);

        mtlVertexDescriptor.attributes[metalIndex] = *attribDesc;
    }

    return AcquireNSRef(mtlVertexDescriptor);
}

NSRef<MTLDepthStencilDescriptor> RenderPipeline::MakeDepthStencilDesc() {
    const DepthStencilState* descriptor = GetDepthStencilState();

    NSRef<MTLDepthStencilDescriptor> mtlDepthStencilDescRef =
        AcquireNSRef([MTLDepthStencilDescriptor new]);
    MTLDepthStencilDescriptor* mtlDepthStencilDescriptor = mtlDepthStencilDescRef.Get();

    mtlDepthStencilDescriptor.depthCompareFunction =
        ToMetalCompareFunction(descriptor->depthCompare);
    mtlDepthStencilDescriptor.depthWriteEnabled =
        descriptor->depthWriteEnabled == wgpu::OptionalBool::True;

    if (UsesStencil()) {
        NSRef<MTLStencilDescriptor> backFaceStencilRef = AcquireNSRef([MTLStencilDescriptor new]);
        MTLStencilDescriptor* backFaceStencil = backFaceStencilRef.Get();
        NSRef<MTLStencilDescriptor> frontFaceStencilRef = AcquireNSRef([MTLStencilDescriptor new]);
        MTLStencilDescriptor* frontFaceStencil = frontFaceStencilRef.Get();

        backFaceStencil.stencilCompareFunction =
            ToMetalCompareFunction(descriptor->stencilBack.compare);
        backFaceStencil.stencilFailureOperation =
            MetalStencilOperation(descriptor->stencilBack.failOp);
        backFaceStencil.depthFailureOperation =
            MetalStencilOperation(descriptor->stencilBack.depthFailOp);
        backFaceStencil.depthStencilPassOperation =
            MetalStencilOperation(descriptor->stencilBack.passOp);
        backFaceStencil.readMask = descriptor->stencilReadMask;
        backFaceStencil.writeMask = descriptor->stencilWriteMask;

        frontFaceStencil.stencilCompareFunction =
            ToMetalCompareFunction(descriptor->stencilFront.compare);
        frontFaceStencil.stencilFailureOperation =
            MetalStencilOperation(descriptor->stencilFront.failOp);
        frontFaceStencil.depthFailureOperation =
            MetalStencilOperation(descriptor->stencilFront.depthFailOp);
        frontFaceStencil.depthStencilPassOperation =
            MetalStencilOperation(descriptor->stencilFront.passOp);
        frontFaceStencil.readMask = descriptor->stencilReadMask;
        frontFaceStencil.writeMask = descriptor->stencilWriteMask;

        mtlDepthStencilDescriptor.backFaceStencil = backFaceStencil;
        mtlDepthStencilDescriptor.frontFaceStencil = frontFaceStencil;
    }

    return mtlDepthStencilDescRef;
}

}  // namespace dawn::native::metal
