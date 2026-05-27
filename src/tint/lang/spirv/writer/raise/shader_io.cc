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

    /// The index of the sampled position polyfill variable.
    std::optional<uint32_t> sampled_position_index;

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
        if (config.polyfill_vulkan_sampled_position && func->IsFragment()) {
            for (auto& input : inputs) {
                if (input.attributes.builtin == core::BuiltinValue::kPosition) {
                    sampled_position_index = inputs.Length();
                    AddInput(ir.symbols.New("tint_sampled_position"), ty.vec4<f32>(),
                             core::IOAttributes{
                                 .location = config.polyfill_vulkan_sampled_position_location,
                                 .interpolation =
                                     core::Interpolation{
                                         core::InterpolationType::kLinear,
                                         core::InterpolationSampling::kCenter,
                                     },
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
        if (config.polyfill_vulkan_sampled_position && func->IsVertex()) {
            for (auto& output : outputs) {
                if (output.attributes.builtin == core::BuiltinValue::kPosition) {
                    sampled_position_index = outputs.Length();
                    AddOutput(ir.symbols.New("tint_sampled_position"), ty.vec4<f32>(),
                              core::IOAttributes{
                                  .location = config.polyfill_vulkan_sampled_position_location,
                                  .interpolation =
                                      core::Interpolation{
                                          core::InterpolationType::kLinear,
                                          core::InterpolationSampling::kCenter,
                                      },
                              });
                    break;
                }
            }
        }
        MakeVars(output_vars, outputs, core::AddressSpace::kOut, core::Access::kWrite, "_Output");
        return ty.void_();
    }

    /// Load an input from its global variable.
    /// @param builder the builder to use for new instructions
    /// @param idx the index of the input
    /// @returns the loaded value
    core::ir::Value* LoadInput(core::ir::Builder& builder, uint32_t idx) {
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

        return value;
    }

    /// Store an output to its global variable.
    /// @param builder the builder to use for new instructions
    /// @param idx the index of the output
    /// @param value the value to store
    void StoreOutput(core::ir::Builder& builder, uint32_t idx, core::ir::Value* value) {
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
    }

    /// @copydoc ShaderIO::BackendState::GetInput
    core::ir::Value* GetInput(core::ir::Builder& builder, uint32_t idx) override {
        if (config.polyfill_vulkan_sampled_position &&
            inputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            sampled_position_index) {
            return ReconstructSampledPosition(builder, idx);
        }

        return LoadInput(builder, idx);
    }

    /// @copydoc ShaderIO::BackendState::SetOutput
    void SetOutput(core::ir::Builder& builder, uint32_t idx, core::ir::Value* value) override {
        if (config.polyfill_vulkan_sampled_position &&
            outputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            sampled_position_index) {
            auto* xyz = builder.Swizzle(ty.vec3<f32>(), value, {0, 1, 2})->Result();
            auto* w = builder.Swizzle(ty.f32(), value, {3})->Result();
            auto* inv_w = builder.Divide(ty.f32(), 1_f, w)->Result();
            auto* xyz_over_w = builder.Multiply(ty.vec3<f32>(), xyz, inv_w)->Result();
            auto* polyfill_value = builder.Construct(ty.vec4<f32>(), xyz_over_w, inv_w)->Result();
            StoreOutput(builder, *sampled_position_index, polyfill_value);
        }

        StoreOutput(builder, idx, value);
    }

    /// Reconstruct the sampled position from the polyfill inter-stage variable.
    /// @param builder the builder to use for new instructions
    /// @param idx the index of the position builtin input
    /// @returns the reconstructed position value
    core::ir::Value* ReconstructSampledPosition(core::ir::Builder& builder, uint32_t idx) {
        auto* position = LoadInput(builder, idx);
        auto* polyfill = LoadInput(builder, *sampled_position_index);

        auto* xy = builder.Swizzle(ty.vec2<f32>(), position, {0, 1})->Result();
        auto* xy_centered =
            builder
                .Add(ty.vec2<f32>(), builder.Call(ty.vec2<f32>(), core::BuiltinFn::kFloor, xy),
                     builder.Splat(ty.vec2<f32>(), 0.5_f))
                ->Result();

        auto* center_zw = builder.Swizzle(ty.vec2<f32>(), polyfill, {2, 3})->Result();
        auto* center_z_over_w = builder.Swizzle(ty.f32(), center_zw, {0})->Result();
        auto* center_inv_w = builder.Swizzle(ty.f32(), center_zw, {1})->Result();

        auto* z = builder.Divide(ty.f32(), center_z_over_w, center_inv_w)->Result();
        if (config.depth_range_offsets) {
            auto* immediate_data = config.immediate_data_layout.var;
            auto min_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->min));
            auto max_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->max));
            auto* min = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, min_idx))
                            ->Result();
            auto* max = builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, max_idx))
                            ->Result();
            auto* range = builder.Subtract(ty.f32(), max, min)->Result();
            z = builder.Add(ty.f32(), builder.Multiply(ty.f32(), z, range)->Result(), min)
                    ->Result();
        }

        return builder.Construct(ty.vec4<f32>(), xy_centered, z, center_inv_w)->Result();
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
