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

    /// Tracks the vertex shader's output position value for the pixel center polyfill.
    core::ir::Value* vert_out_position = nullptr;
    /// Index of the center_pos interpolant in vertex outputs.
    std::optional<uint32_t> center_pos_vert_idx;
    /// Index of the center_pos interpolant in fragment inputs.
    std::optional<uint32_t> center_pos_frag_idx;

    /// Constructor
    StateImpl(core::ir::Module& mod, core::ir::Function* f, const ShaderIOConfig& cfg)
        : ShaderIOBackendState(mod, f), config(cfg) {}

    /// Destructor
    ~StateImpl() override {}

    /// Find the smallest unused location index among existing entries.
    /// @param entries the list of IO entries to search
    /// @returns the smallest unused location index
    uint32_t FindSmallestUnusedLocation(
        const Vector<core::type::Manager::StructMemberDesc, 4>& entries) {
        uint32_t max_loc = 0;
        for (const auto& entry : entries) {
            if (entry.attributes.location.has_value()) {
                max_loc = std::max(max_loc, entry.attributes.location.value() + 1);
            }
        }
        // Find the smallest unused location
        for (uint32_t loc = 0; loc < max_loc + 1; loc++) {
            bool used = false;
            for (const auto& entry : entries) {
                if (entry.attributes.location.has_value() &&
                    entry.attributes.location.value() == loc) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                return loc;
            }
        }
        return max_loc;
    }

    /// Add the center_pos interpolant to the IO entries.
    /// @param entries the list of IO entries
    /// @param addrspace the address space (kIn for fragment, kOut for vertex)
    /// @returns the index of the new interpolant
    uint32_t AddCenterPosInterpolant(
        Vector<core::type::Manager::StructMemberDesc, 4>& entries,
        core::AddressSpace addrspace) {
        uint32_t loc = FindSmallestUnusedLocation(entries);

        core::IOAttributes attrs;
        attrs.location = loc;
        attrs.interpolation = core::Interpolation{
            core::InterpolationType::kLinear,
            core::InterpolationSampling::kCenter,
        };

        if (addrspace == core::AddressSpace::kOut) {
            return AddOutput(ir.symbols.New("center_pos"), ty.vec4<f32>(), std::move(attrs));
        } else {
            return AddInput(ir.symbols.New("center_pos"), ty.vec4<f32>(), std::move(attrs));
        }
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
        // Add the center_pos interpolant for fragment shader inputs if the polyfill is enabled.
        if (config.apply_pixel_center_polyfill && func->IsFragment()) {
            center_pos_frag_idx = AddCenterPosInterpolant(inputs, core::AddressSpace::kIn);
        }
        MakeVars(input_vars, inputs, core::AddressSpace::kIn, core::Access::kRead, "_Input");
        return tint::Empty;
    }

    /// @copydoc ShaderIO::BackendState::FinalizeOutputs
    const core::type::Type* FinalizeOutputs() override {
        // Add the center_pos interpolant for vertex shader outputs if the polyfill is enabled.
        if (config.apply_pixel_center_polyfill && func->IsVertex()) {
            center_pos_vert_idx = AddCenterPosInterpolant(outputs, core::AddressSpace::kOut);
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

        // Convert f32 values to f16 values if needed.
        if (config.polyfill_f16_io && inputs[idx].type->DeepestElement()->Is<core::type::F16>()) {
            value = builder.Convert(inputs[idx].type, value)->Result();
        }

        // Apply the pixel center polyfill for @builtin(position) in fragment shaders.
        if (inputs[idx].attributes.builtin == core::BuiltinValue::kPosition &&
            center_pos_frag_idx.has_value()) {
            // Load the hardware position (which may be sample position in per-sample mode).
            auto* hw_position = value;

            // Swizzle position.xy, floor it, add vec2(0.5) to get pixel-centered xy.
            auto* pos_xy = builder.Swizzle(ty.vec2<f32>(), hw_position, {0u, 1u})->Result();
            auto* floored = builder.Call<vec2<f32>>(core::BuiltinFn::kFloor, pos_xy)->Result();
            auto* half_vec = builder.Construct(ty.vec2<f32>(), 0.5_f, 0.5_f)->Result();
            auto* pixel_center_xy = builder.Add<vec2<f32>>(floored, half_vec)->Result();

            // Load the center-interpolated vec4 from the new input variable.
            auto* center_var = input_vars[center_pos_frag_idx.value()];
            auto* center_val = builder.Load(center_var)->Result();

            // Extract z and w from the center-interpolated variable.
            auto* center_z = builder.Swizzle(ty.f32(), center_val, {2u})->Result();
            auto* center_w = builder.Swizzle(ty.f32(), center_val, {3u})->Result();

            // Apply viewport depth mapping to z.
            auto* mapped_z = ViewportMappedFragDepth(builder, center_z);

            // Compute w = 1.0 / center_w (undo the 1/w from vertex shader).
            auto* one = builder.Constant(1.0_f);
            auto* w = builder.Divide<f32>(one, center_w)->Result();

            // Construct the final vec4f(pixel_center_xy, mapped_z, w).
            value =
                builder.Construct(ty.vec4<f32>(), pixel_center_xy, mapped_z, w)->Result();
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

        // Capture the vertex output position for the pixel center polyfill.
        if (outputs[idx].attributes.builtin == core::BuiltinValue::kPosition) {
            vert_out_position = value;
        }

        // For the center_pos interpolant slot in vertex shader, compute the interpolant value
        // from the captured position instead of using the raw value.
        if (center_pos_vert_idx.has_value() && idx == center_pos_vert_idx.value() &&
            vert_out_position != nullptr) {
            // Compute (position.xy, position.z / position.w, 1.0 / position.w).
            auto* pos_xy =
                builder.Swizzle(ty.vec2<f32>(), vert_out_position, {0u, 1u})->Result();
            auto* pos_z = builder.Swizzle(ty.f32(), vert_out_position, {2u})->Result();
            auto* pos_w = builder.Swizzle(ty.f32(), vert_out_position, {3u})->Result();
            auto* z_over_w = builder.Divide<f32>(pos_z, pos_w)->Result();
            auto* one = builder.Constant(1.0_f);
            auto* inv_w = builder.Divide<f32>(one, pos_w)->Result();
            value = builder.Construct(ty.vec4<f32>(), pos_xy, z_over_w, inv_w)->Result();
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

    /// @copydoc ShaderIO::BackendState::SetBackendOutputs
    void SetBackendOutputs(core::ir::Builder& builder,
                           [[maybe_unused]] core::ir::Value* inner_result) override {
        if (center_pos_vert_idx.has_value()) {
            // Use a dummy value (nullptr) — SetOutput will compute the actual value from
            // vert_out_position.
            SetOutput(builder, center_pos_vert_idx.value(), nullptr);
        }
    }

    /// Apply viewport depth mapping to a frag_depth value.
    /// @param builder the builder to use for new instructions
    /// @param frag_depth the incoming frag_depth value
    /// @returns the viewport-mapped value
    core::ir::Value* ViewportMappedFragDepth(core::ir::Builder& builder,
                                             core::ir::Value* frag_depth) {
        if (!config.depth_range_offsets) {
            return frag_depth;
        }

        auto* immediate_data = config.immediate_data_layout.var;
        auto min_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->min));
        auto max_idx = u32(config.immediate_data_layout.IndexOf(config.depth_range_offsets->max));
        auto* min_val =
            builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, min_idx))->Result();
        auto* max_val =
            builder.Load(builder.Access<ptr<immediate, f32>>(immediate_data, max_idx))->Result();

        // Apply viewport remapping: min + (max - min) * frag_depth
        auto* range = builder.Subtract<f32>(max_val, min_val)->Result();
        auto* scaled = builder.Multiply<f32>(range, frag_depth)->Result();
        return builder.Add<f32>(min_val, scaled)->Result();
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
