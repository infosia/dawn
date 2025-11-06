// Copyright 2023 The Dawn & Tint Authors
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

#include "src/tint/lang/spirv/writer/raise/shader_io.h"

#include <cstdint>
#include <memory>
#include <optional>

#include "src/tint/lang/core/enums.h"
#include "src/tint/lang/core/fluent_types.h"
#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/transform/shader_io.h"
#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/lang/core/type/array.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::spirv::writer::raise {

namespace {

/// PIMPL state for the parts of the shader IO transform specific to SPIR-V.
/// For SPIR-V, we declare a global variable for each input and output. The wrapper entry point then
/// loads from and stores to these variables. We also modify the type of the SampleMask builtin to
/// be an array, as required by Vulkan.
struct StateImpl : core::ir::transform::ShaderIOBackendState {
    /// The input variables.
    Vector<core::ir::Var*, 4> input_vars;
    /// The output variables.
    Vector<core::ir::Var*, 4> output_vars;

    /// The configuration options.
    const ShaderIOConfig& config;

    core::ir::Value* out_pos = nullptr;
    std::optional<uint32_t> center_pos_vert_idx;
    std::optional<uint32_t> center_pos_frag_idx;
    /// Constructor
    StateImpl(core::ir::Module& mod, core::ir::Function* f, const ShaderIOConfig& cfg)
        : ShaderIOBackendState(mod, f), config(cfg) {}

    /// Destructor
    ~StateImpl() override {}

    std::optional<uint32_t> MaybeAddCenterPosInterpolant(
        Vector<core::type::Manager::StructMemberDesc, 4>& entries,
        core::AddressSpace addrspace) {
        bool has_once_per_sample = false;
        bool has_position = true;
        uint32_t free_location = 0u;
        for (auto io : entries) {
            // If either 'sample' is specified in an interpolant or using the 'sample_index'
            // then the fragment is invoked once per sample (not pixel).
            if ((io.attributes.interpolation &&
                 io.attributes.interpolation->sampling == core::InterpolationSampling::kSample) ||
                (io.attributes.builtin &&
                 io.attributes.builtin == core::BuiltinValue::kSampleIndex)) {
                has_once_per_sample = true;
            }

            if (io.attributes.builtin == core::BuiltinValue::kPosition) {
                has_position = true;
            }
            // This assumes ordered and zero start which may not be true
            if (io.attributes.location) {
                free_location++;
            }
        }

        if (has_once_per_sample && has_position) {
            if (addrspace == core::AddressSpace::kOut) {
                return AddOutput(ir.symbols.New("center_pos"), ty.vec4f(),
                                 core::IOAttributes{.location = free_location,
                                                    .interpolation = core::Interpolation{
                                                        core::InterpolationType::kLinear,
                                                        core::InterpolationSampling::kCenter}});
            } else {
                return AddInput(ir.symbols.New("center_pos"), ty.vec4f(),
                                core::IOAttributes{.location = free_location,
                                                   .interpolation = core::Interpolation{
                                                       core::InterpolationType::kLinear,
                                                       core::InterpolationSampling::kCenter}});
            }
        }
        return std::nullopt;
    }

    /// Declare a global variable for each IO entry listed in @p entries.
    /// @param vars the list of variables
    /// @param entries the entries to emit
    /// @param addrspace the address to use for the global variables
    /// @param access the access mode to use for the global variables
    /// @param name_suffix the suffix to add to struct and variable names
    void MakeVars(Vector<core::ir::Var*, 4>& vars,
                  Vector<core::type::Manager::StructMemberDesc, 4>& entries,
                  core::AddressSpace addrspace,
                  core::Access access,
                  const char* name_suffix) {
        if (func->IsVertex() && addrspace == core::AddressSpace::kOut) {
            center_pos_vert_idx = MaybeAddCenterPosInterpolant(entries, addrspace);

        } else if (func->IsFragment() && addrspace == core::AddressSpace::kIn) {
            center_pos_frag_idx = MaybeAddCenterPosInterpolant(entries, addrspace);
        }

        for (auto io : entries) {
            StringStream name;
            name << ir.NameOf(func).Name();

            if (io.attributes.builtin) {
                // SampleMask must be an array for Vulkan.
                if (io.attributes.builtin == core::BuiltinValue::kSampleMask) {
                    io.type = ty.array<u32, 1>();
                }
                name << "_" << io.attributes.builtin.value();

                // Vulkan requires that fragment integer builtin inputs be Flat decorated.
                if (func->IsFragment() && addrspace == core::AddressSpace::kIn &&
                    io.type->IsIntegerScalarOrVector()) {
                    io.attributes.interpolation =
                        core::Interpolation{core::InterpolationType::kFlat};
                }
            }
            if (io.attributes.location) {
                name << "_loc" << io.attributes.location.value();
                if (io.attributes.blend_src.has_value()) {
                    name << "_idx" << io.attributes.blend_src.value();
                }
            }
            name << name_suffix;

            // Replace f16 types with f32 types if necessary.
            auto* store_type = io.type;
            if (config.polyfill_f16_io) {
                if (store_type->DeepestElement()->Is<core::type::F16>()) {
                    store_type = ty.MatchWidth(ty.f32(), io.type);
                }
            }

            // Create an IO variable and add it to the root block.
            auto* ptr = ty.ptr(addrspace, store_type, access);
            auto* var = b.Var(name.str(), ptr);
            var->SetAttributes(io.attributes);
            ir.root_block->Append(var);
            vars.Push(var);
        }
    }

    /// @copydoc ShaderIO::BackendState::FinalizeInputs
    Vector<core::ir::FunctionParam*, 4> FinalizeInputs() override {
        MakeVars(input_vars, inputs, core::AddressSpace::kIn, core::Access::kRead, "_Input");
        return tint::Empty;
    }

    /// @copydoc ShaderIO::BackendState::FinalizeOutputs
    const core::type::Type* FinalizeOutputs() override {
        MakeVars(output_vars, outputs, core::AddressSpace::kOut, core::Access::kWrite, "_Output");
        return ty.void_();
    }

    /// @copydoc ShaderIO::BackendState::GetInput
    core::ir::Value* GetInput(core::ir::Builder& builder, uint32_t idx) override {
        // Load the input from the global variable declared earlier.
        auto* ptr = ty.ptr(core::AddressSpace::kIn, inputs[idx].type, core::Access::kRead);
        auto* from = input_vars[idx]->Result();

        // SampleMask becomes an array for SPIR-V, so load from the first element.
        if (inputs[idx].attributes.builtin == core::BuiltinValue::kSampleMask) {
            from = builder.Access(ptr, input_vars[idx], 0_u)->Result();
        }

        auto* value = builder.Load(from)->Result();

        // Convert f32 values to f16 values if needed.
        if (config.polyfill_f16_io && inputs[idx].type->DeepestElement()->Is<core::type::F16>()) {
            value = builder.Convert(inputs[idx].type, value)->Result();
        }

        if (config.force_pixel_centers &&
            inputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            center_pos_frag_idx.has_value()) {
            // This fix is idempotent in that if it was not needed it will still apply correctly.
            auto* vec_xy = builder.Swizzle(ty.vec2f(), value, {0, 1});
            auto* floor_xy = builder.Call(ty.vec2f(), core::BuiltinFn::kFloor, vec_xy);
            auto p5_const = builder.Constant(0.5_f);
            auto* plus_p5 =
                builder
                    .Add(ty.vec2f(), floor_xy, builder.Construct(ty.vec2f(), p5_const, p5_const))
                    ->Result();

            auto* xyzw_from_user_center =
                builder.Load(input_vars[center_pos_frag_idx.value()]->Result())->Result();

            auto* user_center_z = builder.Swizzle(ty.f32(), xyzw_from_user_center, {2});
            auto* user_center_w = builder.Swizzle(ty.f32(), xyzw_from_user_center, {3});

            auto* clamped_user_center_z = ViewportMappedFragDepth(builder, user_center_z->Result());
            value = builder.Construct(ty.vec4f(), plus_p5, clamped_user_center_z, user_center_w)
                        ->Result();
        }

        return value;
    }

    /// Propagate outputs from the inner function call to their final destination.
    /// @param builder the IR builder for new instructions
    /// @param inner_result the return value from calling the original entry point function
    void SetBackendOutputs(core::ir::Builder& builder, core::ir::Value* inner_result) override {
        if (center_pos_vert_idx.has_value()) {
            SetOutput(builder, center_pos_vert_idx.value(), inner_result);
        }
    }

    /// @copydoc ShaderIO::BackendState::SetOutput
    void SetOutput(core::ir::Builder& builder, uint32_t idx, core::ir::Value* value) override {
        // Store the output to the global variable declared earlier.
        auto& output = outputs[idx];
        auto* ptr = ty.ptr(core::AddressSpace::kOut, output.type, core::Access::kWrite);
        auto* to = output_vars[idx]->Result();

        // SampleMask becomes an array for SPIR-V, so store to the first element.
        if (output.attributes.builtin == core::BuiltinValue::kSampleMask) {
            to = builder.Access(ptr, to, 0_u)->Result();
        }

        if (output.attributes.builtin == core::BuiltinValue::kPosition) {
            out_pos = value;
        }

        if (center_pos_vert_idx == idx) {
            // Special center position polyfill. From within vertex
            auto one_div_w =
                builder.Divide(ty.f32(), 1_f, builder.Swizzle(ty.f32(), out_pos, {3u}));
            auto z_div_w =
                builder.Multiply(ty.f32(), one_div_w, builder.Swizzle(ty.f32(), out_pos, {2u}))
                    ->Result();
            value = builder
                        .Construct(ty.vec4f(), builder.Swizzle(ty.vec2f(), out_pos, {0, 1}),
                                   z_div_w, one_div_w)
                        ->Result();
        }

        // Clamp frag_depth values if necessary.
        if (output.attributes.builtin == core::BuiltinValue::kFragDepth) {
            value = ClampFragDepth(builder, value);
        }

        // Convert f16 values to f32 values if needed.
        if (config.polyfill_f16_io && value->Type()->DeepestElement()->Is<core::type::F16>()) {
            value = builder.Convert(to->Type()->UnwrapPtr(), value)->Result();
        }

        builder.Store(to, value);
    }

    /// Clamp a frag_depth builtin value if necessary.
    /// @param builder the builder to use for new instructions
    /// @param frag_depth the incoming frag_depth value
    /// @returns the clamped value
    core::ir::Value* ViewportMappedFragDepth(core::ir::Builder& builder,
                                             core::ir::Value* frag_depth) {
        if (!config.depth_range_offsets) {
            return frag_depth;
        }

        auto* immediate_data = config.immediate_data_layout.var;
        auto min_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->min));
        auto max_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->max));
        auto* min = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, min_idx));
        auto* max = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, max_idx));
        auto* max_minus_min = builder.Subtract(ty.f32(), max, min);
        auto* rhs = builder.Multiply(ty.f32(), max_minus_min, frag_depth);
        auto res = builder.Add(ty.f32(), min, rhs);
        return res->Result();
    }

    /// Clamp a frag_depth builtin value if necessary.
    /// @param builder the builder to use for new instructions
    /// @param frag_depth the incoming frag_depth value
    /// @returns the clamped value
    core::ir::Value* ClampFragDepth(core::ir::Builder& builder, core::ir::Value* frag_depth) {
        if (!config.depth_range_offsets) {
            printf("NO DEPTH RANGE OFFSET SET!!!!!!\n");
            return frag_depth;
        }

        auto* immediate_data = config.immediate_data_layout.var;
        auto min_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->min));
        auto max_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->max));
        auto* min = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, min_idx));
        auto* max = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, max_idx));
        return builder.Call<f32>(core::BuiltinFn::kClamp, frag_depth, min, max)->Result();
    }

    /// @copydoc ShaderIO::BackendState::NeedsVertexPointSize
    bool NeedsVertexPointSize() const override { return config.emit_vertex_point_size; }
};
}  // namespace

Result<SuccessType> ShaderIO(core::ir::Module& ir, const ShaderIOConfig& config) {
    auto result = ValidateAndDumpIfNeeded(ir, "spirv.ShaderIO", kShaderIOCapabilities);
    if (result != Success) {
        return result;
    }

    core::ir::transform::RunShaderIOBase(ir, [&](core::ir::Module& mod, core::ir::Function* func) {
        return std::make_unique<StateImpl>(mod, func, config);
    });

    return Success;
}

}  // namespace tint::spirv::writer::raise
