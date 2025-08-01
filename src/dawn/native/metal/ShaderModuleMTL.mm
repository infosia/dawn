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

#include "dawn/native/metal/ShaderModuleMTL.h"

#include "absl/strings/escaping.h"
#include "dawn/common/Log.h"
#include "dawn/common/MatchVariant.h"
#include "dawn/common/Range.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/AsyncTask.h"
#include "dawn/native/BindGroupLayout.h"
#include "dawn/native/CacheRequest.h"
#include "dawn/native/Serializable.h"
#include "dawn/native/TintUtils.h"
#include "dawn/native/metal/BindGroupLayoutMTL.h"
#include "dawn/native/metal/DeviceMTL.h"
#include "dawn/native/metal/PipelineLayoutMTL.h"
#include "dawn/native/metal/RenderPipelineMTL.h"
#include "dawn/native/metal/UtilsMetal.h"
#include "dawn/native/stream/BlobSource.h"
#include "dawn/native/stream/ByteVectorSink.h"
#include "dawn/native/utils/WGPUHelpers.h"
#include "dawn/platform/DawnPlatform.h"
#include "dawn/platform/metrics/HistogramMacros.h"
#include "dawn/platform/tracing/TraceEvent.h"

#include <tint/tint.h>

#include <sstream>

namespace dawn::native::metal {
namespace {

using SubstituteOverrideConfig = std::unordered_map<tint::OverrideId, double>;

#define MSL_COMPILATION_REQUEST_MEMBERS(X)                                           \
    X(SingleShaderStage, stage)                                                      \
    X(ShaderModuleBase::ShaderModuleHash, shaderModuleHash)                          \
    X(UnsafeUnserializedValue<ShaderModuleBase::ScopedUseTintProgram>, inputProgram) \
    X(SubstituteOverrideConfig, substituteOverrideConfig)                            \
    X(LimitsForCompilationRequest, limits)                                           \
    X(UnsafeUnserializedValue<LimitsForCompilationRequest>, adapterSupportedLimits)  \
    X(uint32_t, maxSubgroupSize)                                                     \
    X(std::string_view, entryPointName)                                              \
    X(bool, usesSubgroupMatrix)                                                      \
    X(bool, useStrictMath)                                                           \
    X(const tint::msl::writer::Options*, tintOptions)                                \
    X(UnsafeUnserializedValue<dawn::platform::Platform*>, platform)

DAWN_MAKE_CACHE_REQUEST(MslCompilationRequest, MSL_COMPILATION_REQUEST_MEMBERS);
#undef MSL_COMPILATION_REQUEST_MEMBERS

using WorkgroupAllocations = std::vector<uint32_t>;

#define MSL_COMPILATION_MEMBERS(X)                \
    X(std::string, msl)                           \
    X(std::string, remappedEntryPointName)        \
    X(bool, needsStorageBufferLength)             \
    X(bool, hasInvariantAttribute)                \
    X(WorkgroupAllocations, workgroupAllocations) \
    X(Extent3D, localWorkgroupSize)

// clang-format off
DAWN_SERIALIZABLE(struct, MslCompilation, MSL_COMPILATION_MEMBERS) {
    static ResultOrError<MslCompilation> FromValidatedBlob(Blob blob) {
        MslCompilation result;
        DAWN_TRY_ASSIGN(result, FromBlob(std::move(blob)));
        DAWN_INVALID_IF(result.msl.empty() || result.remappedEntryPointName.empty(),
                        "Cached MslCompilation result has no MSL");
        return result;
    }
};
// clang-format on
#undef MSL_COMPILATION_MEMBERS

tint::msl::writer::Bindings GenerateBindingInfo(
    SingleShaderStage stage,
    const PipelineLayout* layout,
    const BindingInfoArray& moduleBindingInfo,
    tint::msl::writer::ArrayLengthOptions& arrayLengthFromConstants,
    bool useArgumentBuffers) {
    tint::msl::writer::Bindings bindings;

    for (BindGroupIndex group : layout->GetBindGroupLayoutsMask()) {
        const BindGroupLayout* bgl = ToBackend(layout->GetBindGroupLayout(group));

        for (const auto& currentModuleBindingInfo : moduleBindingInfo[group]) {
            // We cannot use structured binding here because lambda expressions can only capture
            // variables, while structured binding doesn't introduce variables.
            const auto& binding = currentModuleBindingInfo.first;
            const auto& shaderBindingInfo = currentModuleBindingInfo.second;

            tint::BindingPoint srcBindingPoint{
                .group = uint32_t(group),
                .binding = uint32_t(binding),
            };

            BindingIndex bindingIndex = bgl->GetBindingIndex(binding);
            auto& bindingIndexInfo = layout->GetBindingIndexInfo(stage)[group];
            uint32_t shaderIndex = bindingIndexInfo[bindingIndex];

            tint::BindingPoint dstBindingPoint{
                .group = useArgumentBuffers ? uint32_t(group) : 0,
                .binding = shaderIndex,
            };

            MatchVariant(
                shaderBindingInfo.bindingInfo,
                [&](const BufferBindingInfo& bindingInfo) {
                    switch (bindingInfo.type) {
                        case wgpu::BufferBindingType::Uniform:
                            bindings.uniform.emplace(srcBindingPoint, dstBindingPoint);
                            break;
                        case kInternalStorageBufferBinding:
                        case wgpu::BufferBindingType::Storage:
                        case wgpu::BufferBindingType::ReadOnlyStorage:
                        case kInternalReadOnlyStorageBufferBinding:
                            bindings.storage.emplace(srcBindingPoint, dstBindingPoint);

                            // Use the ShaderIndex as the indices for the buffer size lookups in
                            // the array length uniform transform. This is used to compute the
                            // size of variable length arrays in storage buffers.
                            arrayLengthFromConstants.bindpoint_to_size_index.emplace(
                                srcBindingPoint, dstBindingPoint.binding);
                            break;
                        case wgpu::BufferBindingType::BindingNotUsed:
                        case wgpu::BufferBindingType::Undefined:
                            DAWN_UNREACHABLE();
                            break;
                    }
                },
                [&](const SamplerBindingInfo& bindingInfo) {
                    bindings.sampler.emplace(srcBindingPoint, dstBindingPoint);
                },
                [&](const TextureBindingInfo& bindingInfo) {
                    bindings.texture.emplace(srcBindingPoint, dstBindingPoint);
                },
                [&](const StorageTextureBindingInfo& bindingInfo) {
                    bindings.storage_texture.emplace(srcBindingPoint, dstBindingPoint);
                },
                [&](const ExternalTextureBindingInfo& bindingInfo) {
                    const auto& etBindingMap = bgl->GetExternalTextureBindingExpansionMap();
                    const auto& expansion = etBindingMap.find(binding);
                    DAWN_ASSERT(expansion != etBindingMap.end());

                    const auto& bindingExpansion = expansion->second;
                    tint::BindingPoint plane0{
                        .group = dstBindingPoint.group,
                        .binding = bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.plane0)],
                    };
                    tint::BindingPoint plane1{
                        .group = dstBindingPoint.group,
                        .binding = bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.plane1)],
                    };
                    tint::BindingPoint metadata{
                        .group = dstBindingPoint.group,
                        .binding = bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.params)],
                    };

                    bindings.external_texture.emplace(
                        srcBindingPoint,
                        tint::msl::writer::ExternalTexture{metadata, plane0, plane1});
                },
                [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
        }
    }
    return bindings;
}

std::unordered_map<uint32_t, tint::msl::writer::ArgumentBufferInfo> GenerateArgumentBufferInfo(
    SingleShaderStage stage,
    const PipelineLayout* layout,
    const BindingInfoArray& moduleBindingInfo,
    bool useArgumentBuffers) {
    if (!useArgumentBuffers) {
        return {};
    }

    // TODO(363031535): The dynamic offsets should all move to be immediates and contained into a
    // single buffer.
    std::unordered_map<uint32_t, tint::msl::writer::ArgumentBufferInfo> info = {};

    uint32_t curBufferIdx = kArgumentBufferSlotMax;
    for (BindGroupIndex group : layout->GetBindGroupLayoutsMask()) {
        const BindGroupLayout* bgl = ToBackend(layout->GetBindGroupLayout(group));

        // Node, this buffer index value needs to match up to the value set in the
        // CommandBufferMTL #argument-buffer-index
        tint::msl::writer::ArgumentBufferInfo argBufferInfo = {
            .id = curBufferIdx--,
        };

        uint32_t curDynamicOffset = 0;

        for (BindingIndex bindingIndex : Range(bgl->GetBindingCount())) {
            const BindingInfo& bindingInfo = bgl->GetBindingInfo(bindingIndex);

            MatchVariant(
                bindingInfo.bindingLayout,  //
                [&](const BufferBindingInfo& binding) {
                    if (binding.hasDynamicOffset) {
                        argBufferInfo.dynamic_buffer_id = curBufferIdx--;

                        argBufferInfo.binding_info_to_offset_index.insert(
                            {static_cast<uint32_t>(bindingInfo.binding), curDynamicOffset++});
                    }
                },
                [&](const SamplerBindingInfo& bindingInfo) {},
                [&](const StaticSamplerBindingInfo& bindingInfo) {},
                [&](const TextureBindingInfo& bindingInfo) {},
                [&](const StorageTextureBindingInfo& bindingInfo) {},
                [](const InputAttachmentBindingInfo&) { DAWN_CHECK(false); });
        }
        info.insert({static_cast<uint32_t>(group), argBufferInfo});
    }

    return info;
}

}  // namespace

MetalShaderParams ComputeShaderParams(PipelineBase* pipeline, SingleShaderStage stage) {
    DeviceBase* device = pipeline->GetDevice();
    RenderPipeline* renderPipeline = ToBackend(pipeline->AsRenderPipeline());

    // Vertex and fragment stages must specify a renderPipeline
    DAWN_ASSERT((stage != SingleShaderStage::Vertex && stage != SingleShaderStage::Fragment) ||
                renderPipeline != nullptr);

    const ProgrammableStage& programmableStage = pipeline->GetStage(stage);
    const PipelineLayout* layout = ToBackend(pipeline->GetLayout());
    const BindingInfoArray& moduleBindingInfo = programmableStage.metadata->bindings;

    tint::msl::writer::ArrayLengthOptions arrayLengthFromConstants;
    arrayLengthFromConstants.ubo_binding = kBufferLengthBufferSlot;

    bool useArgumentBuffers = device->IsToggleEnabled(Toggle::MetalUseArgumentBuffers);

    tint::msl::writer::Bindings bindings = GenerateBindingInfo(
        stage, layout, moduleBindingInfo, arrayLengthFromConstants, useArgumentBuffers);

    std::unordered_map<uint32_t, tint::msl::writer::ArgumentBufferInfo> argumentBufferInfo =
        GenerateArgumentBufferInfo(stage, layout, moduleBindingInfo, useArgumentBuffers);

    std::optional<tint::VertexPullingConfig> vertexPullingConfig;
    if (stage == SingleShaderStage::Vertex &&
        device->IsToggleEnabled(Toggle::MetalEnableVertexPulling)) {
        vertexPullingConfig =
            BuildVertexPullingTransformConfig(*renderPipeline, kPullingBufferBindingSet);

        for (VertexBufferSlot slot : renderPipeline->GetVertexBuffersUsed()) {
            uint32_t metalIndex = renderPipeline->GetMtlVertexBufferIndex(slot);

            // Tell Tint to map (kPullingBufferBindingSet, slot) to this MSL buffer index.
            tint::BindingPoint srcBindingPoint{
                .group = uint32_t(kPullingBufferBindingSet),
                .binding = uint8_t(slot),
            };
            tint::BindingPoint dstBindingPoint{
                .group = 0,
                .binding = metalIndex,
            };
            if (srcBindingPoint != dstBindingPoint) {
                bindings.storage.emplace(srcBindingPoint, tint::BindingPoint{
                                                              .group = 0,
                                                              .binding = dstBindingPoint.binding,
                                                          });
            }

            // Use the ShaderIndex as the indices for the buffer size lookups in the array
            // length uniform transform.
            arrayLengthFromConstants.bindpoint_to_size_index.emplace(srcBindingPoint,
                                                                     dstBindingPoint.binding);
        }
    }

    std::unordered_map<uint32_t, uint32_t> pixelLocalAttachments;
    if (stage == SingleShaderStage::Fragment && pipeline->GetLayout()->HasPixelLocalStorage()) {
        const AttachmentState* attachmentState = renderPipeline->GetAttachmentState();
        const std::vector<wgpu::TextureFormat>& storageAttachmentSlots =
            attachmentState->GetStorageAttachmentSlots();
        std::vector<ColorAttachmentIndex> storageAttachmentPacking =
            attachmentState->ComputeStorageAttachmentPackingInColorAttachments();

        for (size_t i = 0; i < storageAttachmentSlots.size(); i++) {
            pixelLocalAttachments[i] = uint8_t(storageAttachmentPacking[i]);
        }
    }

    uint32_t sampleMask =
        stage == SingleShaderStage::Fragment ? renderPipeline->GetSampleMask() : 0xFFFFFFFF;

    MetalShaderParams params;
    params.stage = stage;
    params.shaderModuleHash = programmableStage.module->GetHash();
    params.entryPointName = programmableStage.entryPoint;
    params.pipelineConstants = programmableStage.constants;

    params.tintOptions.strip_all_names = !device->IsToggleEnabled(Toggle::DisableSymbolRenaming);
    params.tintOptions.remapped_entry_point_name = device->GetIsolatedEntryPointName();
    params.tintOptions.disable_robustness = !device->IsRobustnessEnabled();
    params.tintOptions.buffer_size_ubo_index = kBufferLengthBufferSlot;
    params.tintOptions.fixed_sample_mask = sampleMask;
    params.tintOptions.disable_workgroup_init = false;
    params.tintOptions.disable_demote_to_helper =
        device->IsToggleEnabled(Toggle::DisableDemoteToHelper);
    params.tintOptions.emit_vertex_point_size =
        params.stage == SingleShaderStage::Vertex &&
        renderPipeline->GetPrimitiveTopology() == wgpu::PrimitiveTopology::PointList;
    params.tintOptions.array_length_from_constants = std::move(arrayLengthFromConstants);
    params.tintOptions.pixel_local_attachments = std::move(pixelLocalAttachments);
    params.tintOptions.bindings = std::move(bindings);
    params.tintOptions.disable_polyfill_integer_div_mod =
        device->IsToggleEnabled(Toggle::DisablePolyfillsOnIntegerDivisonAndModulo);
    params.tintOptions.scalarize_max_min_clamp =
        device->IsToggleEnabled(Toggle::ScalarizeMaxMinClamp);
    params.tintOptions.disable_module_constant_f16 =
        device->IsToggleEnabled(Toggle::MetalDisableModuleConstantF16);
    params.tintOptions.polyfill_subgroup_broadcast_f16 =
        device->IsToggleEnabled(Toggle::EnableSubgroupsIntelGen9);
    params.tintOptions.polyfill_unpack_2x16_snorm =
        device->IsToggleEnabled(Toggle::MetalPolyfillUnpack2x16snorm);
    params.tintOptions.vertex_pulling_config = std::move(vertexPullingTransformConfig);
    params.tintOptions.enable_integer_range_analysis =
        device->IsToggleEnabled(Toggle::EnableIntegerRangeAnalysisInRobustness);
    params.tintOptions.use_argument_buffers = useArgumentBuffers;
    params.tintOptions.group_to_argument_buffer_info = std::move(argumentBufferInfo);

    params.useStrictMath = programmableStage.module->GetStrictMath().value_or(false);

    return params;
}

// static
ResultOrError<Ref<ShaderModule>> ShaderModule::Create(
    Device* device,
    const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
    const std::vector<tint::wgsl::Extension>& internalExtensions,
    ShaderModuleParseResult* parseResult) {
    Ref<ShaderModule> module = AcquireRef(new ShaderModule(device, descriptor, internalExtensions));
    DAWN_TRY(module->Initialize(parseResult));
    return module;
}

ShaderModule::ShaderModule(Device* device,
                           const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
                           std::vector<tint::wgsl::Extension> internalExtensions)
    : ShaderModuleBase(device, descriptor, std::move(internalExtensions)) {}

ShaderModule::~ShaderModule() = default;

MaybeError ShaderModule::Initialize(ShaderModuleParseResult* parseResult) {
    return InitializeBase(parseResult);
}

namespace {

ResultOrError<CacheResult<MslCompilation>> TranslateToMSL(
    DeviceBase* device,
    ShaderModuleBase::ScopedUseTintProgram useTintProgram,
    const MetalShaderParams& params) {
    const EntryPointMetadata& entryPointMetadata =
        useTintProgram->GetEntryPoint(params.entryPointName);
    MslCompilationRequest req = {};
    req.stage = params.stage;
    req.shaderModuleHash = params.shaderModuleHash;
    req.inputProgram = UnsafeUnserializedValue(std::move(useTintProgram));
    req.substituteOverrideConfig =
        BuildSubstituteOverridesTransformConfig(entryPointMetadata, params.pipelineConstants);
    req.entryPointName = params.entryPointName;
    req.usesSubgroupMatrix = entryPointMetadata.usesSubgroupMatrix;
    req.platform = UnsafeUnserializedValue(device->GetPlatform());
    req.useStrictMath = params.useStrictMath;
    req.tintOptions = &params.tintOptions;
    req.limits = LimitsForCompilationRequest::Create(device->GetLimits().v1);
    req.adapterSupportedLimits = UnsafeUnserializedValue(
        LimitsForCompilationRequest::Create(device->GetAdapter()->GetLimits().v1));
    req.maxSubgroupSize = device->GetAdapter()->GetPhysicalDevice()->GetSubgroupMaxSize();

    CacheResult<MslCompilation> mslCompilation;
    DAWN_TRY_LOAD_OR_RUN(
        mslCompilation, device, std::move(req), MslCompilation::FromValidatedBlob,
        [](MslCompilationRequest r) -> ResultOrError<MslCompilation> {
            TRACE_EVENT0(r.platform.UnsafeGetValue(), General, "tint::msl::writer::Generate");
            // Requires Tint Program here right before actual using.
            auto shaderModule = r.inputProgram.UnsafeGetValue();
            auto inputProgram = shaderModule->GetTintProgram();
            const tint::Program* tintInputProgram = &(inputProgram->program);
            // Convert the AST program to an IR module.
            tint::Result<tint::core::ir::Module> ir;
            {
                SCOPED_DAWN_HISTOGRAM_TIMER_MICROS(r.platform.UnsafeGetValue(),
                                                   "ShaderModuleProgramToIR");
                ir = tint::wgsl::reader::ProgramToLoweredIR(
                    *tintInputProgram,
                    shaderModule->GetDevice()->GetTintInternalCompilerErrorCallback());
                DAWN_INVALID_IF(ir != tint::Success,
                                "An error occurred while generating Tint IR\n%s",
                                ir.Failure().reason);
            }

            {
                SCOPED_DAWN_HISTOGRAM_TIMER_MICROS(r.platform.UnsafeGetValue(),
                                                   "ShaderModuleSingleEntryPoint");
                auto singleEntryPointResult =
                    tint::core::ir::transform::SingleEntryPoint(ir.Get(), r.entryPointName);
                DAWN_INVALID_IF(singleEntryPointResult != tint::Success,
                                "Pipeline single entry point (IR) failed:\n%s",
                                singleEntryPointResult.Failure().reason);
            }

            // this needs to run after SingleEntryPoint transform which removes unused
            // overrides for the current entry point.
            {
                SCOPED_DAWN_HISTOGRAM_TIMER_MICROS(r.platform.UnsafeGetValue(),
                                                   "ShaderModuleSubstituteOverrides");
                tint::core::ir::transform::SubstituteOverridesConfig cfg;
                cfg.map = std::move(r.substituteOverrideConfig);
                auto substituteOverridesResult =
                    tint::core::ir::transform::SubstituteOverrides(ir.Get(), cfg);
                DAWN_INVALID_IF(substituteOverridesResult != tint::Success,
                                "Pipeline override substitution (IR) failed:\n%s",
                                substituteOverridesResult.Failure().reason);
            }

            // Generate MSL.
            tint::Result<tint::msl::writer::Output> result;
            {
                SCOPED_DAWN_HISTOGRAM_TIMER_MICROS(r.platform.UnsafeGetValue(),
                                                   "ShaderModuleGenerateMSL");
                result = tint::msl::writer::Generate(ir.Get(), *r.tintOptions);
                DAWN_INVALID_IF(result != tint::Success,
                                "An error occurred while generating MSL:\n%s",
                                result.Failure().reason);
            }

            // Workgroup validation has to come after `Generate` because it may require
            // overrides to have been substituted.
            Extent3D localSize{0, 0, 0};
            if (r.stage == SingleShaderStage::Compute) {
                // Validate workgroup size and workgroup storage size.
                DAWN_TRY_ASSIGN(localSize,
                                ValidateComputeStageWorkgroupSize(
                                    result->workgroup_info.x, result->workgroup_info.y,
                                    result->workgroup_info.z, result->workgroup_info.storage_size,
                                    r.usesSubgroupMatrix, r.maxSubgroupSize, r.limits,
                                    r.adapterSupportedLimits.UnsafeGetValue()));
            }

            auto msl = std::move(result->msl);
            // Metal supports math_mode as both compiler option and as a pragma. We add the
            // math_mode here as a string conditional on OSx version as the compiler option only
            // exists for MacOS after 15. See the Metal 4 spec for more information.
            // Note: this math_mode takes precedence over global flags provide to the compiler
            // (including the deprecated fastMathEnabled compiler option).
            std::string math_mode_heading;
            if (@available(macOS 15.0, iOS 18.0, *)) {
                math_mode_heading = "\n#pragma METAL fp math_mode(";
                math_mode_heading += r.useStrictMath ? "safe" : "relaxed";
                math_mode_heading += +")\n";
            }
            // Metal uses Clang to compile the shader as C++14. Disable everything in the -Wall
            // category. -Wunused-variable in particular comes up a lot in generated code, and
            // some (old?) Metal drivers accidentally treat it as a MTLLibraryErrorCompileError
            // instead of a warning.
            msl = R"(
                    #ifdef __clang__
                    #pragma clang diagnostic ignored "-Wall"
                    #endif
                )" +
                  math_mode_heading + msl;

            return MslCompilation{{
                std::move(msl),
                r.tintOptions->remapped_entry_point_name,
                result->needs_storage_buffer_sizes,
                result->has_invariant_attribute,
                std::move(result->workgroup_info.allocations),
                localSize,
            }};
        },
        "Metal.CompileShaderToMSL");

    if (device->IsToggleEnabled(Toggle::DumpShaders)) {
        std::ostringstream dumpedMsg;
        dumpedMsg << "/* Dumped generated MSL */\n" << mslCompilation->msl;
        device->EmitLog(wgpu::LoggingType::Info, dumpedMsg.str().c_str());
        ErrorLog() << dumpedMsg.str();
    }

    return mslCompilation;
}

ResultOrError<MetalFunctionData> LoadEntryPointFunction(
    DeviceBase* device,
    ShaderModuleBase::ScopedUseTintProgram useTintProgram,
    const MetalShaderParams& params,
    const std::string& label) {
    CacheResult<MslCompilation> mslCompilation;
    {
        TRACE_EVENT1(device->GetPlatform(), General, "metal::ShaderModule::TranslateToMSL", "label",
                     utils::GetLabelForTrace(label));
        DAWN_TRY_ASSIGN(mslCompilation, TranslateToMSL(device, useTintProgram, params));
    }

    NSRef<NSString> mslSource =
        AcquireNSRef([[NSString alloc] initWithUTF8String:mslCompilation->msl.c_str()]);

    NSRef<MTLCompileOptions> compileOptions = AcquireNSRef([[MTLCompileOptions alloc] init]);
    if (mslCompilation->hasInvariantAttribute) {
        (*compileOptions).preserveInvariance = true;
    }

    // TODO(433534277): Warn if the pipeline uses print but it is not available or enabled.
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 150000
    if (device->IsToggleEnabled(Toggle::EnableShaderPrint)) {
        if (@available(macOS 15.0, iOS 18.0, *)) {
            (*compileOptions).enableLogging = true;
            (*compileOptions).languageVersion = MTLLanguageVersion3_2;
        }
    }
#endif

    // If possible we will use relaxed math as a pragma in the source rather than this fast math
    // global compiler option. See crbug.com/425650181
    if (@available(macOS 15.0, iOS 18.0, *)) {
        // This empty bock is intentional due to the mechanism to support @available.
        // (the @available must be in a plain 'if' statement)
        // Without this empty block one would get '-Wunsupported-availability-guard'
    } else {
// Silence the warning that fastMathEnabled is deprecated since we cannot remove it until the
// minimum support macOS version is macOS 15.0.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        (*compileOptions).fastMathEnabled = !params.useStrictMath;
#pragma clang diagnostic pop
    }

    auto mtlDevice = ToBackend(device)->GetMTLDevice();
    platform::metrics::DawnHistogramTimer timer(device->GetPlatform());

    NSError* error = nullptr;
    NSPRef<id<MTLLibrary>> library;
    {
        TRACE_EVENT0(device->GetPlatform(), General, "MTLDevice::newLibraryWithSource");
        library = AcquireNSPRef([mtlDevice newLibraryWithSource:mslSource.Get()
                                                        options:compileOptions.Get()
                                                          error:&error]);
    }
    if (!library) {
        // clang-format formats `mslCompilation->msl` below oddly as `mslCompilation -> msl`.
        // clang-format off
        return DAWN_FORMAT_INTERNAL_ERROR("ShaderModuleMTL: Unable to create library object: %s "
                                          "from produced MSL source below:\n\n%s",
                                          [error.localizedDescription UTF8String],
                                          mslCompilation->msl);
        // clang-format on
    }
    timer.RecordMicroseconds("Metal.newLibraryWithSource.CacheMiss");

    NSRef<NSString> mslEntryPointName =
        [NSString stringWithUTF8String:mslCompilation->remappedEntryPointName.c_str()];
    NSPRef<id<MTLFunction>> entryPointFunction;
    {
        TRACE_EVENT0(device->GetPlatform(), General, "MTLLibrary::newFunctionWithName");
        entryPointFunction = AcquireNSPRef([*library newFunctionWithName:mslEntryPointName.Get()]);
        // TODO(372181030): Remove this unnecessary check when we understand why the MTLFunction
        // might be nil here.
        if (!entryPointFunction) {
            std::string availableFunctions;
            for (NSString* fn : [*library functionNames]) {
                availableFunctions += "\n - \"";
                availableFunctions += [fn UTF8String];
            }
            return DAWN_FORMAT_INTERNAL_ERROR(
                "ShaderModuleMTL: failed to get the MTLFunction \'%s\' from produced MSL "
                "shader below:\n\n%s\n\nAvailable functions are:%s",
                mslCompilation->remappedEntryPointName, mslCompilation->msl, availableFunctions);
        }
    }

    std::string functionLabel = absl::StrFormat("%s::%s", label, params.entryPointName);
    SetDebugName(device, entryPointFunction.Get(), "Dawn_ShaderModule", functionLabel);

    device->GetBlobCache()->EnsureStored(mslCompilation);

    MetalFunctionData result;
    result.needsStorageBufferLength = mslCompilation->needsStorageBufferLength;
    result.workgroupAllocations = std::move(mslCompilation->workgroupAllocations);
    result.localWorkgroupSize = MTLSizeMake(mslCompilation->localWorkgroupSize.width,
                                            mslCompilation->localWorkgroupSize.height,
                                            mslCompilation->localWorkgroupSize.depthOrArrayLayers);
    result.function = std::move(entryPointFunction);
    if (device->IsToggleEnabled(Toggle::MetalEnableVertexPulling) &&
        useTintProgram->GetEntryPoint(params.entryPointName).usedVertexInputs.any()) {
        result.needsStorageBufferLength = true;
    }
    // For emitting MSL in error message if render pipeline creation fails.
    result.msl = std::move(mslSource);

    return result;
}

}  // namespace

MetalShaderLibrary::MetalShaderLibrary(Device* device,
                                       MetalShaderParams params,
                                       std::string_view label)
    : mDevice(device), mParams(params), mLabel(label) {
    SetContentHash(ComputeContentHash());
}

MetalShaderLibrary::~MetalShaderLibrary() = default;

void MetalShaderLibrary::DeleteThis() {
    ContentLessObjectCacheable::Uncache();
    RefCounted::DeleteThis();
}

void MetalShaderLibrary::LoadEntryPoint(ShaderModuleBase::ScopedUseTintProgram useTintProgram,
                                        LoadEntryPointCallback callback) {
    TRACE_EVENT1(mDevice->GetPlatform(), General, "MetalShaderLibrary::LoadEntryPoint", "label",
                 utils::GetLabelForTrace(mLabel));
    auto copyResult = [](std::optional<MetalFunctionData>& result,
                         std::unique_ptr<ErrorData>& error) -> ResultOrError<MetalFunctionData> {
        if (result.has_value()) [[likely]] {
            return ResultOrError<MetalFunctionData>(result.value());
        }
        return ResultOrError<MetalFunctionData>(std::make_unique<ErrorData>(*error.get()));
    };

    std::optional<ResultOrError<MetalFunctionData>> readyResult;
    {
        Mutex::AutoLock lock(&mMutex);
        if (mResultState == ResultState::kPending) {
            ErrorLog() << "Pending library cache hit: " << mLabel;
            mPendingCallbacks.push_back(std::move(callback));
            return;
        }
        if (mResultState == ResultState::kNotStarted) {
            ErrorLog() << "Not started library cache miss: " << mLabel;
            mResultState = ResultState::kPending;
        } else {
            ErrorLog() << "Ready library cache hit: " << mLabel;
            DAWN_ASSERT(mResultState == ResultState::kReady);
            readyResult = copyResult(mResult, mError);
        }
    }

    if (readyResult) {
        callback(std::move(readyResult.value()));
        return;
    }

    auto result = LoadEntryPointFunction(mDevice, std::move(useTintProgram), mParams, mLabel);

    std::optional<MetalFunctionData> resultData;
    std::unique_ptr<ErrorData> resultError;

    if (result.IsSuccess()) [[likely]] {
        resultData = result.AcquireSuccess();
    } else {
        resultError = result.AcquireError();
    }

    auto syncResult = copyResult(resultData, resultError);

    {
        Mutex::AutoLock lock(&mMutex);
        mResultState = ResultState::kReady;
        mResult = std::move(resultData);
        mError = std::move(resultError);
        for (auto& pendingCallback : mPendingCallbacks) {
            auto asyncResult =
                std::make_shared<ResultOrError<MetalFunctionData>>(copyResult(mResult, mError));
            auto asyncTask = [asyncCallback = std::move(pendingCallback),
                              asyncResult = std::move(asyncResult)]() {
                asyncCallback(std::move(*asyncResult));
            };
            mDevice->GetAsyncTaskManager()->PostTask(std::move(asyncTask));
        }
        mPendingCallbacks.clear();
    }

    callback(std::move(syncResult));
}

ResultOrError<MetalFunctionData> LoadShaderEntryPoint(PipelineBase* pipeline,
                                                      SingleShaderStage stage) {
    DeviceBase* device = pipeline->GetDevice();
    ShaderModuleBase* shaderModule = pipeline->GetStage(stage).module.Get();

    DAWN_ASSERT(!shaderModule->IsError());

    TRACE_EVENT1(device->GetPlatform(), General, "metal::ShaderModule::CreateFunction", "label",
                 utils::GetLabelForTrace(shaderModule->GetLabel()));

    MetalShaderParams params = ComputeShaderParams(pipeline, stage);

    return LoadEntryPointFunction(device, shaderModule->UseTintProgram(), params,
                                  shaderModule->GetLabel());
}

}  // namespace dawn::native::metal
