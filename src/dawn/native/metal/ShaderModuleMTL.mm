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

#include "dawn/common/MatchVariant.h"
#include "dawn/native/Adapter.h"
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
#include "tint/lang/msl/writer/ast_raise/vertex_pulling.h"

#include <tint/tint.h>

#include <sstream>

namespace dawn::native::metal {
namespace {

// The name to use when remapping entry points.
constexpr char kRemappedEntryPointName[] = "dawn_entry_point";

using OptionalVertexPullingTransformConfig =
    std::optional<tint::msl::writer::VertexPulling::Config>;

#define MSL_COMPILATION_REQUEST_MEMBERS(X)                                                       \
    X(SingleShaderStage, stage)                                                                  \
    X(const tint::Program*, inputProgram)                                                        \
    X(OptionalVertexPullingTransformConfig, vertexPullingTransformConfig)                        \
    X(std::optional<tint::ast::transform::SubstituteOverride::Config>, substituteOverrideConfig) \
    X(LimitsForCompilationRequest, limits)                                                       \
    X(CacheKey::UnsafeUnkeyedValue<const AdapterBase*>, adapter)                                 \
    X(std::string, entryPointName)                                                               \
    X(bool, disableSymbolRenaming)                                                               \
    X(tint::msl::writer::Options, tintOptions)                                                   \
    X(bool, use_tint_ir)                                                                         \
    X(CacheKey::UnsafeUnkeyedValue<dawn::platform::Platform*>, platform)

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

DAWN_SERIALIZABLE(struct, MslCompilation, MSL_COMPILATION_MEMBERS){};
#undef MSL_COMPILATION_MEMBERS

tint::msl::writer::VertexFormat ToTintVertexFormat(wgpu::VertexFormat format) {
    switch (format) {
        case wgpu::VertexFormat::Uint8:
            return tint::msl::writer::VertexFormat::kUint8;
        case wgpu::VertexFormat::Uint8x2:
            return tint::msl::writer::VertexFormat::kUint8x2;
        case wgpu::VertexFormat::Uint8x4:
            return tint::msl::writer::VertexFormat::kUint8x4;
        case wgpu::VertexFormat::Sint8:
            return tint::msl::writer::VertexFormat::kSint8;
        case wgpu::VertexFormat::Sint8x2:
            return tint::msl::writer::VertexFormat::kSint8x2;
        case wgpu::VertexFormat::Sint8x4:
            return tint::msl::writer::VertexFormat::kSint8x4;
        case wgpu::VertexFormat::Unorm8:
            return tint::msl::writer::VertexFormat::kUnorm8;
        case wgpu::VertexFormat::Unorm8x2:
            return tint::msl::writer::VertexFormat::kUnorm8x2;
        case wgpu::VertexFormat::Unorm8x4:
            return tint::msl::writer::VertexFormat::kUnorm8x4;
        case wgpu::VertexFormat::Snorm8:
            return tint::msl::writer::VertexFormat::kSnorm8;
        case wgpu::VertexFormat::Snorm8x2:
            return tint::msl::writer::VertexFormat::kSnorm8x2;
        case wgpu::VertexFormat::Snorm8x4:
            return tint::msl::writer::VertexFormat::kSnorm8x4;
        case wgpu::VertexFormat::Uint16:
            return tint::msl::writer::VertexFormat::kUint16;
        case wgpu::VertexFormat::Uint16x2:
            return tint::msl::writer::VertexFormat::kUint16x2;
        case wgpu::VertexFormat::Uint16x4:
            return tint::msl::writer::VertexFormat::kUint16x4;
        case wgpu::VertexFormat::Sint16:
            return tint::msl::writer::VertexFormat::kSint16;
        case wgpu::VertexFormat::Sint16x2:
            return tint::msl::writer::VertexFormat::kSint16x2;
        case wgpu::VertexFormat::Sint16x4:
            return tint::msl::writer::VertexFormat::kSint16x4;
        case wgpu::VertexFormat::Unorm16:
            return tint::msl::writer::VertexFormat::kUnorm16;
        case wgpu::VertexFormat::Unorm16x2:
            return tint::msl::writer::VertexFormat::kUnorm16x2;
        case wgpu::VertexFormat::Unorm16x4:
            return tint::msl::writer::VertexFormat::kUnorm16x4;
        case wgpu::VertexFormat::Snorm16:
            return tint::msl::writer::VertexFormat::kSnorm16;
        case wgpu::VertexFormat::Snorm16x2:
            return tint::msl::writer::VertexFormat::kSnorm16x2;
        case wgpu::VertexFormat::Snorm16x4:
            return tint::msl::writer::VertexFormat::kSnorm16x4;
        case wgpu::VertexFormat::Float16:
            return tint::msl::writer::VertexFormat::kFloat16;
        case wgpu::VertexFormat::Float16x2:
            return tint::msl::writer::VertexFormat::kFloat16x2;
        case wgpu::VertexFormat::Float16x4:
            return tint::msl::writer::VertexFormat::kFloat16x4;
        case wgpu::VertexFormat::Float32:
            return tint::msl::writer::VertexFormat::kFloat32;
        case wgpu::VertexFormat::Float32x2:
            return tint::msl::writer::VertexFormat::kFloat32x2;
        case wgpu::VertexFormat::Float32x3:
            return tint::msl::writer::VertexFormat::kFloat32x3;
        case wgpu::VertexFormat::Float32x4:
            return tint::msl::writer::VertexFormat::kFloat32x4;
        case wgpu::VertexFormat::Uint32:
            return tint::msl::writer::VertexFormat::kUint32;
        case wgpu::VertexFormat::Uint32x2:
            return tint::msl::writer::VertexFormat::kUint32x2;
        case wgpu::VertexFormat::Uint32x3:
            return tint::msl::writer::VertexFormat::kUint32x3;
        case wgpu::VertexFormat::Uint32x4:
            return tint::msl::writer::VertexFormat::kUint32x4;
        case wgpu::VertexFormat::Sint32:
            return tint::msl::writer::VertexFormat::kSint32;
        case wgpu::VertexFormat::Sint32x2:
            return tint::msl::writer::VertexFormat::kSint32x2;
        case wgpu::VertexFormat::Sint32x3:
            return tint::msl::writer::VertexFormat::kSint32x3;
        case wgpu::VertexFormat::Sint32x4:
            return tint::msl::writer::VertexFormat::kSint32x4;
        case wgpu::VertexFormat::Unorm10_10_10_2:
            return tint::msl::writer::VertexFormat::kUnorm10_10_10_2;
        case wgpu::VertexFormat::Unorm8x4BGRA:
            return tint::msl::writer::VertexFormat::kUnorm8x4BGRA;
    }
    DAWN_UNREACHABLE();
}

tint::msl::writer::VertexStepMode ToTintVertexStepMode(wgpu::VertexStepMode mode) {
    switch (mode) {
        case wgpu::VertexStepMode::Vertex:
            return tint::msl::writer::VertexStepMode::kVertex;
        case wgpu::VertexStepMode::Instance:
            return tint::msl::writer::VertexStepMode::kInstance;
        case wgpu::VertexStepMode::Undefined:
            break;
    }
    DAWN_UNREACHABLE();
}

tint::msl::writer::VertexPulling::Config BuildVertexPullingTransformConfig(
    const RenderPipelineBase& renderPipeline,
    BindGroupIndex pullingBufferBindingSet) {
    tint::msl::writer::VertexPulling::Config cfg;
    cfg.pulling_group = static_cast<uint32_t>(pullingBufferBindingSet);

    cfg.vertex_state.resize(renderPipeline.GetVertexBufferCount());
    for (VertexBufferSlot slot : IterateBitSet(renderPipeline.GetVertexBuffersUsed())) {
        const VertexBufferInfo& dawnInfo = renderPipeline.GetVertexBuffer(slot);
        tint::msl::writer::VertexBufferLayoutDescriptor* tintInfo =
            &cfg.vertex_state[static_cast<uint8_t>(slot)];

        tintInfo->array_stride = dawnInfo.arrayStride;
        tintInfo->step_mode = ToTintVertexStepMode(dawnInfo.stepMode);
    }

    for (VertexAttributeLocation location :
         IterateBitSet(renderPipeline.GetAttributeLocationsUsed())) {
        const VertexAttributeInfo& dawnInfo = renderPipeline.GetAttribute(location);
        tint::msl::writer::VertexAttributeDescriptor tintInfo;
        tintInfo.format = ToTintVertexFormat(dawnInfo.format);
        tintInfo.offset = dawnInfo.offset;
        tintInfo.shader_location = static_cast<uint32_t>(static_cast<uint8_t>(location));

        uint8_t vertexBufferSlot = static_cast<uint8_t>(dawnInfo.vertexBufferSlot);
        cfg.vertex_state[vertexBufferSlot].attributes.push_back(tintInfo);
    }
    return cfg;
}

}  // namespace
}  // namespace dawn::native::metal

namespace dawn::native::metal {

// static
ResultOrError<Ref<ShaderModule>> ShaderModule::Create(
    Device* device,
    const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
    const std::vector<tint::wgsl::Extension>& internalExtensions,
    ShaderModuleParseResult* parseResult,
    OwnedCompilationMessages* compilationMessages) {
    Ref<ShaderModule> module = AcquireRef(new ShaderModule(device, descriptor, internalExtensions));
    DAWN_TRY(module->Initialize(parseResult, compilationMessages));
    return module;
}

ShaderModule::ShaderModule(Device* device,
                           const UnpackedPtr<ShaderModuleDescriptor>& descriptor,
                           std::vector<tint::wgsl::Extension> internalExtensions)
    : ShaderModuleBase(device, descriptor, std::move(internalExtensions)) {}

ShaderModule::~ShaderModule() = default;

MaybeError ShaderModule::Initialize(ShaderModuleParseResult* parseResult,
                                    OwnedCompilationMessages* compilationMessages) {
    ScopedTintICEHandler scopedICEHandler(GetDevice());
    return InitializeBase(parseResult, compilationMessages);
}

namespace {

tint::msl::writer::Bindings generateBindingInfo(
    SingleShaderStage stage,
    const PipelineLayout* layout,
    const BindingInfoArray& moduleBindingInfo,
    tint::msl::writer::ArrayLengthFromUniformOptions& arrayLengthFromUniform) {
    tint::msl::writer::Bindings bindings;

    for (BindGroupIndex group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
        const BindGroupLayout* bgl = ToBackend(layout->GetBindGroupLayout(group));

        for (const auto& currentModuleBindingInfo : moduleBindingInfo[group]) {
            // We cannot use structured binding here because lambda expressions can only capture
            // variables, while structured binding doesn't introduce variables.
            const auto& binding = currentModuleBindingInfo.first;
            const auto& shaderBindingInfo = currentModuleBindingInfo.second;

            tint::BindingPoint srcBindingPoint{static_cast<uint32_t>(group),
                                               static_cast<uint32_t>(binding)};

            BindingIndex bindingIndex = bgl->GetBindingIndex(binding);
            auto& bindingIndexInfo = layout->GetBindingIndexInfo(stage)[group];
            uint32_t shaderIndex = bindingIndexInfo[bindingIndex];

            tint::BindingPoint dstBindingPoint{0, shaderIndex};

            MatchVariant(
                shaderBindingInfo.bindingInfo,
                [&](const BufferBindingInfo& bindingInfo) {
                    switch (bindingInfo.type) {
                        case wgpu::BufferBindingType::Uniform:
                            bindings.uniform.emplace(
                                srcBindingPoint,
                                tint::msl::writer::binding::Uniform{dstBindingPoint.binding});
                            break;
                        case kInternalStorageBufferBinding:
                        case wgpu::BufferBindingType::Storage:
                        case wgpu::BufferBindingType::ReadOnlyStorage:
                            bindings.storage.emplace(
                                srcBindingPoint,
                                tint::msl::writer::binding::Storage{dstBindingPoint.binding});
                            // Use the ShaderIndex as the indices for the buffer size lookups in
                            // the array length uniform transform. This is used to compute the
                            // size of variable length arrays in storage buffers.
                            arrayLengthFromUniform.bindpoint_to_size_index.emplace(
                                srcBindingPoint, dstBindingPoint.binding);
                            break;
                        case wgpu::BufferBindingType::BindingNotUsed:
                        case wgpu::BufferBindingType::Undefined:
                            DAWN_UNREACHABLE();
                            break;
                    }
                },
                [&](const SamplerBindingInfo& bindingInfo) {
                    bindings.sampler.emplace(srcBindingPoint, tint::msl::writer::binding::Sampler{
                                                                  dstBindingPoint.binding});
                },
                [&](const TextureBindingInfo& bindingInfo) {
                    bindings.texture.emplace(srcBindingPoint, tint::msl::writer::binding::Texture{
                                                                  dstBindingPoint.binding});
                },
                [&](const StorageTextureBindingInfo& bindingInfo) {
                    bindings.storage_texture.emplace(
                        srcBindingPoint,
                        tint::msl::writer::binding::StorageTexture{dstBindingPoint.binding});
                },
                [&](const ExternalTextureBindingInfo& bindingInfo) {
                    const auto& etBindingMap = bgl->GetExternalTextureBindingExpansionMap();
                    const auto& expansion = etBindingMap.find(binding);
                    DAWN_ASSERT(expansion != etBindingMap.end());

                    const auto& bindingExpansion = expansion->second;
                    tint::msl::writer::binding::BindingInfo plane0{
                        bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.plane0)]};
                    tint::msl::writer::binding::BindingInfo plane1{
                        bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.plane1)]};
                    tint::msl::writer::binding::BindingInfo metadata{
                        bindingIndexInfo[bgl->GetBindingIndex(bindingExpansion.params)]};

                    bindings.external_texture.emplace(
                        srcBindingPoint,
                        tint::msl::writer::binding::ExternalTexture{metadata, plane0, plane1});
                },
                [](const InputAttachmentBindingInfo&) { DAWN_UNREACHABLE(); });
        }
    }
    return bindings;
}

ResultOrError<CacheResult<MslCompilation>> TranslateToMSL(
    DeviceBase* device,
    const ProgrammableStage& programmableStage,
    SingleShaderStage stage,
    const PipelineLayout* layout,
    ShaderModule::MetalFunctionData* out,
    uint32_t sampleMask,
    const RenderPipeline* renderPipeline,
    const BindingInfoArray& moduleBindingInfo) {
    ScopedTintICEHandler scopedICEHandler(device);

    std::ostringstream errorStream;
    errorStream << "Tint MSL failure:\n";

    tint::msl::writer::ArrayLengthFromUniformOptions arrayLengthFromUniform;
    arrayLengthFromUniform.ubo_binding = kBufferLengthBufferSlot;

    tint::msl::writer::Bindings bindings =
        generateBindingInfo(stage, layout, moduleBindingInfo, arrayLengthFromUniform);

    std::optional<tint::msl::writer::VertexPulling::Config> vertexPullingTransformConfig;
    if (stage == SingleShaderStage::Vertex &&
        device->IsToggleEnabled(Toggle::MetalEnableVertexPulling)) {
        vertexPullingTransformConfig =
            BuildVertexPullingTransformConfig(*renderPipeline, kPullingBufferBindingSet);

        for (VertexBufferSlot slot : IterateBitSet(renderPipeline->GetVertexBuffersUsed())) {
            uint32_t metalIndex = renderPipeline->GetMtlVertexBufferIndex(slot);

            // Tell Tint to map (kPullingBufferBindingSet, slot) to this MSL buffer index.
            tint::BindingPoint srcBindingPoint{static_cast<uint32_t>(kPullingBufferBindingSet),
                                               static_cast<uint8_t>(slot)};
            tint::BindingPoint dstBindingPoint{0, metalIndex};
            if (srcBindingPoint != dstBindingPoint) {
                bindings.storage.emplace(
                    srcBindingPoint, tint::msl::writer::binding::Storage{dstBindingPoint.binding});
            }

            // Use the ShaderIndex as the indices for the buffer size lookups in the array
            // length uniform transform.
            arrayLengthFromUniform.bindpoint_to_size_index.emplace(srcBindingPoint,
                                                                   dstBindingPoint.binding);
        }
    }

    std::optional<tint::ast::transform::SubstituteOverride::Config> substituteOverrideConfig;
    if (!programmableStage.metadata->overrides.empty()) {
        substituteOverrideConfig = BuildSubstituteOverridesTransformConfig(programmableStage);
    }

    std::unordered_map<uint32_t, uint32_t> pixelLocalAttachments;
    if (stage == SingleShaderStage::Fragment && layout->HasPixelLocalStorage()) {
        const AttachmentState* attachmentState = renderPipeline->GetAttachmentState();
        const std::vector<wgpu::TextureFormat>& storageAttachmentSlots =
            attachmentState->GetStorageAttachmentSlots();
        std::vector<ColorAttachmentIndex> storageAttachmentPacking =
            attachmentState->ComputeStorageAttachmentPackingInColorAttachments();

        for (size_t i = 0; i < storageAttachmentSlots.size(); i++) {
            pixelLocalAttachments[i] = uint8_t(storageAttachmentPacking[i]);
        }
    }

    MslCompilationRequest req = {};
    req.stage = stage;
    auto tintProgram = programmableStage.module->GetTintProgram();
    req.inputProgram = &(tintProgram->program);
    req.vertexPullingTransformConfig = std::move(vertexPullingTransformConfig);
    req.substituteOverrideConfig = std::move(substituteOverrideConfig);
    req.entryPointName = programmableStage.entryPoint.c_str();
    req.disableSymbolRenaming = device->IsToggleEnabled(Toggle::DisableSymbolRenaming);
    req.platform = UnsafeUnkeyedValue(device->GetPlatform());

    req.use_tint_ir = device->IsToggleEnabled(Toggle::UseTintIR);
    if (req.use_tint_ir) {
        req.tintOptions.strip_all_names = !req.disableSymbolRenaming;
        req.tintOptions.remapped_entry_point_name = kRemappedEntryPointName;
    }
    req.tintOptions.disable_robustness = !device->IsRobustnessEnabled();
    req.tintOptions.buffer_size_ubo_index = kBufferLengthBufferSlot;
    req.tintOptions.fixed_sample_mask = sampleMask;
    req.tintOptions.disable_workgroup_init = false;
    req.tintOptions.disable_demote_to_helper =
        device->IsToggleEnabled(Toggle::DisableDemoteToHelper);
    req.tintOptions.emit_vertex_point_size =
        stage == SingleShaderStage::Vertex &&
        renderPipeline->GetPrimitiveTopology() == wgpu::PrimitiveTopology::PointList;
    req.tintOptions.array_length_from_uniform = std::move(arrayLengthFromUniform);
    req.tintOptions.pixel_local_attachments = std::move(pixelLocalAttachments);
    req.tintOptions.bindings = std::move(bindings);
    req.tintOptions.disable_polyfill_integer_div_mod =
        device->IsToggleEnabled(Toggle::DisablePolyfillsOnIntegerDivisonAndModulo);

    const CombinedLimits& limits = device->GetLimits();
    req.limits = LimitsForCompilationRequest::Create(limits.v1);
    req.adapter = UnsafeUnkeyedValue(static_cast<const AdapterBase*>(device->GetAdapter()));

    CacheResult<MslCompilation> mslCompilation;
    DAWN_TRY_LOAD_OR_RUN(
        mslCompilation, device, std::move(req), MslCompilation::FromBlob,
        [](MslCompilationRequest r) -> ResultOrError<MslCompilation> {
            tint::ast::transform::Manager transformManager;
            tint::ast::transform::DataMap transformInputs;

            // We only remap bindings for the target entry point, so we need to strip all other
            // entry points to avoid generating invalid bindings for them.
            // Run before the renamer so that the entry point name matches `entryPointName` still.
            transformManager.Add<tint::ast::transform::SingleEntryPoint>();
            transformInputs.Add<tint::ast::transform::SingleEntryPoint::Config>(r.entryPointName);

            if (!r.use_tint_ir) {
                // Needs to run before other transforms so that they can use builtin names safely.
                tint::ast::transform::Renamer::Remappings requestedNames = {
                    {r.entryPointName, kRemappedEntryPointName}};
                transformManager.Add<tint::ast::transform::Renamer>();
                transformInputs.Add<tint::ast::transform::Renamer::Config>(
                    r.disableSymbolRenaming ? tint::ast::transform::Renamer::Target::kMslKeywords
                                            : tint::ast::transform::Renamer::Target::kAll,
                    std::move(requestedNames));
            }

            if (r.vertexPullingTransformConfig) {
                transformManager.Add<tint::msl::writer::VertexPulling>();
                transformInputs.Add<tint::msl::writer::VertexPulling::Config>(
                    std::move(r.vertexPullingTransformConfig).value());
            }

            if (r.substituteOverrideConfig) {
                // This needs to run after SingleEntryPoint transform which removes unused overrides
                // for current entry point.
                transformManager.Add<tint::ast::transform::SubstituteOverride>();
                transformInputs.Add<tint::ast::transform::SubstituteOverride::Config>(
                    std::move(r.substituteOverrideConfig).value());
            }

            tint::Program program;
            tint::ast::transform::DataMap transformOutputs;
            {
                TRACE_EVENT0(r.platform.UnsafeGetValue(), General, "RunTransforms");
                DAWN_TRY_ASSIGN(program,
                                RunTransforms(&transformManager, r.inputProgram, transformInputs,
                                              &transformOutputs, nullptr));
            }

            Extent3D localSize{0, 0, 0};

            TRACE_EVENT0(r.platform.UnsafeGetValue(), General, "tint::msl::writer::Generate");
            tint::Result<tint::msl::writer::Output> result;
            if (r.use_tint_ir) {
                // Convert the AST program to an IR module.
                auto ir = tint::wgsl::reader::ProgramToLoweredIR(program);
                DAWN_INVALID_IF(ir != tint::Success,
                                "An error occurred while generating Tint IR\n%s",
                                ir.Failure().reason.Str());

                result = tint::msl::writer::Generate(ir.Get(), r.tintOptions);

                // Workgroup validation has to come after `Generate` because it may require
                // overrides to have been substituted.
                if (r.stage == SingleShaderStage::Compute) {
                    // Validate workgroup size and workgroup storage size.
                    DAWN_TRY_ASSIGN(
                        localSize,
                        ValidateComputeStageWorkgroupSize(
                            result->workgroup_info.x, result->workgroup_info.y,
                            result->workgroup_info.z, result->workgroup_info.storage_size, r.limits,
                            r.adapter.UnsafeGetValue()));
                }
            } else {
                if (r.stage == SingleShaderStage::Compute) {
                    // Validate workgroup size after program runs transforms.
                    DAWN_TRY_ASSIGN(localSize, ValidateComputeStageWorkgroupSize(
                                                   program, kRemappedEntryPointName, r.limits,
                                                   r.adapter.UnsafeGetValue()));
                }

                result = tint::msl::writer::Generate(program, r.tintOptions);
            }

            DAWN_INVALID_IF(result != tint::Success, "An error occurred while generating MSL:\n%s",
                            result.Failure().reason.Str());

            // Metal uses Clang to compile the shader as C++14. Disable everything in the -Wall
            // category. -Wunused-variable in particular comes up a lot in generated code, and some
            // (old?) Metal drivers accidentally treat it as a MTLLibraryErrorCompileError instead
            // of a warning.
            auto msl = std::move(result->msl);
            msl = R"(
                #ifdef __clang__
                #pragma clang diagnostic ignored "-Wall"
                #endif
            )" + msl;

            auto workgroupAllocations =
                std::move(result->workgroup_info.allocations.at(kRemappedEntryPointName));
            return MslCompilation{{
                std::move(msl),
                std::move(kRemappedEntryPointName),
                result->needs_storage_buffer_sizes,
                result->has_invariant_attribute,
                std::move(workgroupAllocations),
                localSize,
            }};
        },
        "Metal.CompileShaderToMSL");

    if (device->IsToggleEnabled(Toggle::DumpShaders)) {
        std::ostringstream dumpedMsg;
        dumpedMsg << "/* Dumped generated MSL */\n" << mslCompilation->msl;
        device->EmitLog(WGPULoggingType_Info, dumpedMsg.str().c_str());
    }

    return mslCompilation;
}

}  // namespace

MaybeError ShaderModule::CreateFunction(SingleShaderStage stage,
                                        const ProgrammableStage& programmableStage,
                                        const PipelineLayout* layout,
                                        ShaderModule::MetalFunctionData* out,
                                        uint32_t sampleMask,
                                        const RenderPipeline* renderPipeline) {
    TRACE_EVENT1(GetDevice()->GetPlatform(), General, "metal::ShaderModule::CreateFunction",
                 "label", utils::GetLabelForTrace(GetLabel()));

    DAWN_ASSERT(!IsError());
    DAWN_ASSERT(out);

    const char* entryPointName = programmableStage.entryPoint.c_str();

    // Vertex stages must specify a renderPipeline
    if (stage == SingleShaderStage::Vertex) {
        DAWN_ASSERT(renderPipeline != nullptr);
    }

    CacheResult<MslCompilation> mslCompilation;
    DAWN_TRY_ASSIGN(mslCompilation,
                    TranslateToMSL(GetDevice(), programmableStage, stage, layout, out, sampleMask,
                                   renderPipeline, GetEntryPoint(entryPointName).bindings));

    out->needsStorageBufferLength = mslCompilation->needsStorageBufferLength;
    out->workgroupAllocations = std::move(mslCompilation->workgroupAllocations);
    out->localWorkgroupSize = MTLSizeMake(mslCompilation->localWorkgroupSize.width,
                                          mslCompilation->localWorkgroupSize.height,
                                          mslCompilation->localWorkgroupSize.depthOrArrayLayers);

    NSRef<NSString> mslSource =
        AcquireNSRef([[NSString alloc] initWithUTF8String:mslCompilation->msl.c_str()]);

    NSRef<MTLCompileOptions> compileOptions = AcquireNSRef([[MTLCompileOptions alloc] init]);
    if (mslCompilation->hasInvariantAttribute) {
        if (@available(macOS 11.0, iOS 13.0, *)) {
            (*compileOptions).preserveInvariance = true;
        }
    }

    (*compileOptions).fastMathEnabled = !GetStrictMath().value_or(false);

    auto mtlDevice = ToBackend(GetDevice())->GetMTLDevice();
    NSError* error = nullptr;

    NSPRef<id<MTLLibrary>> library;
    platform::metrics::DawnHistogramTimer timer(GetDevice()->GetPlatform());
    {
        TRACE_EVENT0(GetDevice()->GetPlatform(), General, "MTLDevice::newLibraryWithSource");
        library = AcquireNSPRef([mtlDevice newLibraryWithSource:mslSource.Get()
                                                        options:compileOptions.Get()
                                                          error:&error]);
    }

    if (error != nullptr) {
        DAWN_INVALID_IF(error.code != MTLLibraryErrorCompileWarning,
                        "Unable to create library object: %s.",
                        [error.localizedDescription UTF8String]);
    }
    DAWN_ASSERT(library != nil);
    timer.RecordMicroseconds("Metal.newLibraryWithSource.CacheMiss");

    NSRef<NSString> name = AcquireNSRef(
        [[NSString alloc] initWithUTF8String:mslCompilation->remappedEntryPointName.c_str()]);

    {
        TRACE_EVENT0(GetDevice()->GetPlatform(), General, "MTLLibrary::newFunctionWithName");
        out->function = AcquireNSPRef([*library newFunctionWithName:name.Get()]);
        // TODO(372181030): Remove this unnecessary check when we understand why the MTLFunction
        // might be nil here.
        if (out->function == nil) {
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

    std::ostringstream labelStream;
    labelStream << GetLabel() << "::" << entryPointName;
    SetDebugName(GetDevice(), out->function.Get(), "Dawn_ShaderModule", labelStream.str());
    GetDevice()->GetBlobCache()->EnsureStored(mslCompilation);

    if (GetDevice()->IsToggleEnabled(Toggle::MetalEnableVertexPulling) &&
        GetEntryPoint(entryPointName).usedVertexInputs.any()) {
        out->needsStorageBufferLength = true;
    }

    return {};
}
}  // namespace dawn::native::metal
