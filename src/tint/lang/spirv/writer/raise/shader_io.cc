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

#include <memory>

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

    /// The index of the input/output for the sample-mask-aware position polyfill.
    std::optional<uint32_t> sample_mask_aware_position_idx;

    /// Constructor
    StateImpl(core::ir::Module& mod, core::ir::Function* f, const ShaderIOConfig& cfg)
        : ShaderIOBackendState(mod, f), config(cfg) {}

    /// Destructor
    ~StateImpl() override {}

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
        if (config.polyfill_sample_mask_aware_position && func->IsFragment()) {
            for (auto& input : inputs) {
                if (input.attributes.builtin == core::BuiltinValue::kPosition) {
                    sample_mask_aware_position_idx = AddInput(
                        ir.symbols.New("tint_sample_mask_aware_position"), ty.vec4f(),
                        core::IOAttributes{
                            .location = config.sample_mask_aware_position_location,
                            .interpolation =
                                core::Interpolation{core::InterpolationType::kLinear,
                                                    core::InterpolationSampling::kCenter},
                            .invariant = input.attributes.invariant,
                        });
                    break;
                }
            }
        }
        MakeVars(input_vars, inputs, core::AddressSpace::kIn, core::Access::kRead, "_Input");
        return tint::Empty;
    }

    /// @copydoc ShaderIO::BackendState::FinalizeOutputs
    const core::type::Type* FinalizeOutputs() override {
        if (config.polyfill_sample_mask_aware_position && func->IsVertex()) {
            bool is_invariant = false;
            for (auto& output : outputs) {
                if (output.attributes.builtin == core::BuiltinValue::kPosition) {
                    is_invariant = output.attributes.invariant;
                    break;
                }
            }
            sample_mask_aware_position_idx = AddOutput(
                ir.symbols.New("tint_sample_mask_aware_position"), ty.vec4f(),
                core::IOAttributes{
                    .location = config.sample_mask_aware_position_location,
                    .interpolation = core::Interpolation{core::InterpolationType::kLinear,
                                                        core::InterpolationSampling::kCenter},
                    .invariant = is_invariant,
                });
        }
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

        if (inputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            sample_mask_aware_position_idx.has_value()) {
            // Reconstruct pixel-centered position.
            auto* center_pos = GetInput(builder, sample_mask_aware_position_idx.value());
            auto* vec2f = ty.vec2f();
            auto* xy = builder.Swizzle(vec2f, value, {0, 1});
            auto* floor_xy = builder.Call(vec2f, core::BuiltinFn::kFloor, xy);
            auto* centered_xy = builder.Add(vec2f, floor_xy, builder.Splat(vec2f, 0.5_f));
            auto* z = builder.Access(ty.f32(), center_pos, 2_u)->Result();
            if (config.depth_range_offsets) {
                auto* immediate_data = config.immediate_data_layout.var;
                auto min_idx =
                    u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->min));
                auto max_idx =
                    u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->max));
                auto* min =
                    builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, min_idx));
                auto* max =
                    builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, max_idx));
                auto* range = builder.Subtract(ty.f32(), max, min);
                z = builder.Add(ty.f32(), builder.Multiply(ty.f32(), z, range), min)->Result();
            }
            auto* w = builder.Access(ty.f32(), center_pos, 3_u);
            value = builder.Construct(ty.vec4f(), centered_xy, z, w)->Result();
        }

        // Convert f32 values to f16 values if needed.
        if (config.polyfill_f16_io && inputs[idx].type->DeepestElement()->Is<core::type::F16>()) {
            value = builder.Convert(inputs[idx].type, value)->Result();
        }

        return value;
    }

    /// @copydoc ShaderIO::BackendState::SetOutput
    void SetOutput(core::ir::Builder& builder, uint32_t idx, core::ir::Value* value) override {
        // Store the output to the global variable declared earlier.
        auto* ptr = ty.ptr(core::AddressSpace::kOut, outputs[idx].type, core::Access::kWrite);
        auto* to = output_vars[idx]->Result();

        // SampleMask becomes an array for SPIR-V, so store to the first element.
        if (outputs[idx].attributes.builtin == core::BuiltinValue::kSampleMask) {
            to = builder.Access(ptr, to, 0_u)->Result();
        }

        // Clamp frag_depth values if necessary.
        if (outputs[idx].attributes.builtin == core::BuiltinValue::kFragDepth) {
            value = ClampFragDepth(builder, value);
        }

        // Convert f16 values to f32 values if needed.
        if (config.polyfill_f16_io && value->Type()->DeepestElement()->Is<core::type::F16>()) {
            value = builder.Convert(to->Type()->UnwrapPtr(), value)->Result();
        }

        builder.Store(to, value);

        if (outputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            sample_mask_aware_position_idx.has_value()) {
            auto* pos_w = builder.Access(ty.f32(), value, 3_u);
            auto* inv_w = builder.Divide(ty.f32(), 1.0_f, pos_w);
            auto* pos_xyz = builder.Swizzle(ty.vec3f(), value, {0, 1, 2});
            auto* xyz_div_w = builder.Multiply(ty.vec3f(), pos_xyz, builder.Splat(ty.vec3f(), inv_w));
            auto* center_pos = builder.Construct(ty.vec4f(), xyz_div_w, inv_w);
            SetOutput(builder, sample_mask_aware_position_idx.value(), center_pos->Result());
        }
    }

    /// Clamp a frag_depth builtin value if necessary.
    /// @param builder the builder to use for new instructions
    /// @param frag_depth the incoming frag_depth value
    /// @returns the clamped value
    core::ir::Value* ClampFragDepth(core::ir::Builder& builder, core::ir::Value* frag_depth) {
        if (!config.depth_range_offsets) {
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
