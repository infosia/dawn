# Task: Polyfill `@builtin(position)` for Vulkan in Multisampled Fragment Shaders

## Problem

The WebGPU/WGSL specification requires that the `@builtin(position)` value in fragment shaders always reports **pixel-centered** coordinates (i.e., `floor(pos.xy) + 0.5` for x/y). On MSL (Metal) and HLSL (D3D), this is always the case, even when multisampling is enabled. However, on **Vulkan**, the `position` builtin may report the **sample position** rather than the pixel center when the fragment shader runs in per-sample mode (triggered by using `@interpolate(*, sample)` or `@builtin(sample_index)`). This causes incorrect behavior and CTS test failures on Vulkan backends.

**Reference bug:** 445242488

## Solution Overview

Implement a polyfill in the SPIR-V backend that ensures `@builtin(position)` always returns pixel-centered values in multisampled fragment shaders. The approach is:

1. **Add a new inter-stage variable** (`vec4f`, with `@interpolate(linear, center)`) that passes pre-perspective-division position data from vertex to fragment shader.
2. **In the vertex shader wrapper**: After the original vertex shader output sets `@builtin(position)`, compute `z/w` and `1/w` from the position, and write `(position.xy, z/w, 1/w)` to the new interpolant.
3. **In the fragment shader wrapper**: When the original shader reads `@builtin(position)`, replace it with a reconstructed value: `(floor(hw_position.xy) + 0.5, viewport_mapped_z, w)` where `z` and `w` come from the center-interpolated variable (applying viewport depth range mapping if depth range offsets are available).

The polyfill should activate only when ALL of these conditions hold:
- The render pipeline sample count > 1
- The fragment shader uses `@builtin(position)`
- The fragment shader uses `@builtin(sample_index)` OR any inter-stage input uses `@interpolate(*, sample)` sampling

Since this consumes an additional inter-stage variable slot, the Vulkan `maxInterStageShaderVariables` limit must be adjusted to reserve one extra slot.

## Detailed Implementation Steps

### 1. Inspector: Detect `@builtin(position)` usage in fragment shaders

**File: `src/tint/lang/wgsl/inspector/entry_point.h`**
- Add a `bool frag_position_used = false;` field to the `EntryPoint` struct (near the existing `frag_depth_used`).

**File: `src/tint/lang/wgsl/inspector/inspector.cc`**
- In the function that processes entry point parameters, when the stage is `kFragment`, check if any parameter contains `@builtin(kPosition)` using the existing `ContainsBuiltin` helper. Set `entry_point.frag_position_used` accordingly.

### 2. Shader Module Metadata: Propagate fragment position and multisampling info

**File: `src/dawn/native/ShaderModule.h`**
- Add two new fields to the entry point metadata macro list:
  - `X(bool, usesFragPosition)` — true if the fragment shader uses `@builtin(position)`
  - `X(bool, isFragMultiSampled)` — true if any inter-stage input variable uses `@interpolate(*, sample)` sampling

**File: `src/dawn/native/ShaderModule.cpp`**
- When processing fragment shader input variables, check if `interpolation_sampling == kSample` and set `metadata->isFragMultiSampled = true`.
- Set `metadata->usesFragPosition = entryPoint.frag_position_used` from the inspector data (next to where `usesFragDepth` is set).

### 3. Render Pipeline: Expose new metadata accessors

**File: `src/dawn/native/RenderPipeline.h`**
- Add three new `bool` member variables: `mUsesFragPosition`, `mIsFragMultiSampled`, `mUsesSampleIndex` (all default `false`).
- Add three new const accessor methods: `UsesFragPosition()`, `IsFragMultiSampled()`, `UsesSampleIndex()`.

**File: `src/dawn/native/RenderPipeline.cpp`**
- In the constructor where `mUsesFragDepth` is set from fragment stage metadata, also set `mUsesFragPosition`, `mIsFragMultiSampled`, and `mUsesSampleIndex` from the corresponding metadata fields.
- Implement the three accessor methods (each asserts `!IsError()` and returns the member).

### 4. Vulkan Render Pipeline: Compute `isSampled` and pass it through

**File: `src/dawn/native/vulkan/RenderPipelineVk.cpp`**
- Compute a boolean `isSampled`:
  ```cpp
  bool isSampled = (GetSampleCount() > 1u) && UsesFragPosition() &&
                   (IsFragMultiSampled() || UsesSampleIndex());
  ```
- The immediate constant mask condition should include `isSampled`: if either `isSampled` or `UsesFragDepth()` is true (and `!HasUnclippedDepth()`), set the clamp frag depth immediate constant bits. This is because the polyfill's depth reconstruction reuses the same viewport depth range immediate constants.
- Pass `isSampled` as a new parameter to `GetHandleAndSpirv()` for both vertex and fragment stages.

### 5. Vulkan Shader Module: Thread `isSampled` to Tint options

**File: `src/dawn/native/vulkan/ShaderModuleVk.h`**
- Add a `bool isSampled` parameter to `GetHandleAndSpirv()`.

**File: `src/dawn/native/vulkan/ShaderModuleVk.cpp`**
- Accept the new `isSampled` parameter.
- Set `req.tintOptions.apply_pixel_center_polyfill = isSampled;` in the Tint compilation options.

**File: `src/dawn/native/vulkan/ComputePipelineVk.cpp`**
- Update the call to `GetHandleAndSpirv()` to pass `false` for the new `isSampled` parameter (compute shaders never need this).

### 6. SPIR-V Writer Options: Add the new option

**File: `src/tint/lang/spirv/writer/common/options.h`**
- Add `bool apply_pixel_center_polyfill = false;` to the Options struct.
- Add it to the reflection/comparison macro list as well.

### 7. SPIR-V Writer Raise: Pass option to ShaderIO config

**File: `src/tint/lang/spirv/writer/raise/raise.cc`**
- When constructing `ShaderIOConfig`, pass `options.apply_pixel_center_polyfill` to the config.

**File: `src/tint/lang/spirv/writer/raise/shader_io.h`**
- Add `bool apply_pixel_center_polyfill = false;` to the `ShaderIOConfig` struct.

### 8. SPIR-V ShaderIO Transform: Core polyfill implementation

This is the most complex part. **File: `src/tint/lang/spirv/writer/raise/shader_io.cc`**

**New state in `StateImpl`:**
- `core::ir::Value* vert_out_position = nullptr;` — tracks the vertex shader's output position value.
- `std::optional<uint32_t> center_pos_vert_idx;` — index of the new interpolant in vertex outputs.
- `std::optional<uint32_t> center_pos_frag_idx;` — index of the new interpolant in fragment inputs.

**New method `AddCenterPosInterpolant(entries, addrspace)`:**
- Finds the smallest unused location index among existing entries.
- Creates an IO attribute with that location, `@interpolate(linear, center)`.
- Calls `AddOutput` (for vertex/out) or `AddInput` (for fragment/in) with name `"center_pos"` and type `vec4f`.

**In `EmitGlobalVariables` (the method that creates global vars for IO entries):**
- Before processing entries, if the function is a vertex shader and `addrspace == kOut` and `apply_pixel_center_polyfill` is true, call `AddCenterPosInterpolant` and store the index in `center_pos_vert_idx`.
- If the function is a fragment shader and `addrspace == kIn` and `apply_pixel_center_polyfill` is true, call `AddCenterPosInterpolant` and store the index in `center_pos_frag_idx`.

**In `GetInput` (fragment shader input loading):**
- After loading the `@builtin(position)` value, if `center_pos_frag_idx` has a value:
  1. Swizzle `position.xy`, floor it, add `vec2(0.5)` to get pixel-centered xy.
  2. Load the center-interpolated vec4 from the new input variable.
  3. Extract z and w from it.
  4. Apply viewport depth mapping to z (using `ViewportMappedFragDepth`).
  5. Construct the final `vec4f(pixel_center_xy, mapped_z, w)` and return that instead.

**New method `ViewportMappedFragDepth(builder, frag_depth)`:**
- If `config.depth_range_offsets` is not set, return `frag_depth` unchanged.
- Otherwise, load min and max from immediate data and apply the viewport remapping: `min + (max - min) * frag_depth`.

**In `SetOutput` (vertex shader output storing):**
- When storing to the `@builtin(position)` output, capture the value in `vert_out_position`.
- When storing to the `center_pos_vert_idx` slot, instead of storing the raw value, compute: `(position.xy, position.z / position.w, 1.0 / position.w)` using `vert_out_position`, and store that.

**New override `SetBackendOutputs(builder, inner_result)`:**
- Add a virtual method `SetBackendOutputs(Builder&, Value*)` to the base `ShaderIOBackendState` class (in `shader_io.h`) with an empty default implementation.
- In the base `shader_io.cc` transform, call `backend->SetBackendOutputs(wrapper, inner_result->Result())` after all other outputs are set but before the return.
- In the SPIR-V `StateImpl`, override it: if `center_pos_vert_idx` has a value, call `SetOutput(builder, center_pos_vert_idx.value(), inner_result)`. This triggers the position computation logic in `SetOutput` above. The `inner_result` value itself is not directly used; it just triggers the `SetOutput` path which uses the already-captured `vert_out_position`.

### 9. Vulkan Physical Device Limits: Reserve an extra inter-stage slot

**File: `src/dawn/native/vulkan/PhysicalDeviceVk.cpp`**
- Change the minimum component check from `* 4 + 4` to `* 4 + 8` (need 2 reserved vec4 slots now: one for position, one for the emulated pixel center).
- Define `auto constexpr kNumReservedVariables = 1 + 1;` (1 for position + 1 for emulated pixel center).
- Change the limit calculation from `/ 4 - 1` to `/ 4 - kNumReservedVariables`.
- Update the error message comment to explain the new reservation.

### 10. Update Tests

**File: `src/dawn/tests/BUILD.gn`**
- Add `"end2end/MultisampledInterpolationTests.cpp"` to the end2end test sources list.

**File: `src/dawn/tests/DawnTest.h`**
- Add a macro `EXPECT_BUFFER_FLOAT_RANGE_TOLERANCE_EQ(expected, buffer, offset, count, tolerance)` similar to the existing `EXPECT_BUFFER_FLOAT_RANGE_EQ` but accepting a tolerance parameter.

**New file: `src/dawn/tests/end2end/MultisampledInterpolationTests.cpp`**
- Create an end2end test `MultisampledInterpolationTest::SamplePositions` that:
  - Draws a single triangle to a 10x10 multisampled (4x MSAA) render target.
  - The vertex shader outputs position and inter-stage values with `@interpolate(linear, center)`.
  - The fragment shader uses `@builtin(sample_index)` (to trigger per-sample shading) and encodes `@builtin(position)` as rgba8unorm across 4 render targets.
  - A compute shader reads back the multisampled textures and reconstructs the float position values.
  - Verifies the first pixel's 4 samples all report pixel-centered position (0.5, 0.5 for xy) rather than sample positions.
  - Skip on compatibility mode, Swiftshader, and ANGLE Swiftshader.
  - Instantiate for all backends: D3D11, D3D12, Metal, OpenGL, OpenGLES, Vulkan, WebGPU.

**File: `src/tint/lang/spirv/writer/raise/shader_io_test.cc`**
- Add unit tests for the SPIR-V ShaderIO transform with `apply_pixel_center_polyfill = true`:
  1. `ForcePixelCenters_SampleInterpolation` — fragment shader with `@builtin(position)` and a `@interpolate(linear, sample)` input. Verify the polyfill adds a `@location(1) @interpolate(linear, center)` input and reconstructs position.
  2. `ForcePixelCenters_SampleIndex` — fragment shader with `@builtin(position)` and `@builtin(sample_index)`. Same verification.
  3. `ForcePixelCenters_NoModification` — fragment shader with `@builtin(position)` but NO sample interpolation or sample_index. The polyfill should still add the interpolant and reconstruct position (the polyfill is idempotent — if it wasn't needed, it still produces correct results since pixel center == pixel center).
  4. `ForcePixelCenters_SampleInterpolation_InStruct` — same as #1 but the sample-interpolated input is in a struct parameter. Verify the polyfill finds an unused location (location 0 since struct uses location 1).
  5. `ForcePixelCenters_SampleIndex_InStruct` — same as #2 but `sample_index` is in a struct parameter.
  6. `ForcePixelCenters_PositionNotUsed` — fragment shader declares `@builtin(position)` but only uses `color`. The polyfill should still apply (the flag is set at the pipeline level, not based on actual usage within the shader body).

**File: `src/dawn/tests/end2end/ArchTierLimitsExhaustive.cpp`**
- Update the expected `maxInterStageShaderVariables` values for Intel Vulkan devices that were previously reporting 28 to now report 16 (due to the extra reserved slot reducing the limit).

### 11. Remove CTS Failure Expectations

**File: `webgpu-cts/expectations.txt`**
- Remove the failure expectations for `fragment_builtins:inputs,position` tests with `sampleCount=4` and `sampling=sample` on:
  - Android Pixel 6
  - Intel Linux
  - Linux NVIDIA
- These tests should now pass because the polyfill fixes the position values.
- Keep the `sample_mask` failure expectations as those are unrelated.

## Key Design Decisions

1. **The polyfill is idempotent**: Even if the hardware already reports pixel-centered values, the polyfill produces correct results (floor(0.5) + 0.5 = 0.5). This means it's safe to always apply when the conditions are met.

2. **Location assignment**: The new interpolant uses the smallest free location index to avoid conflicts with user-defined inter-stage variables.

3. **Depth reconstruction**: The z component is passed as `z/w` from vertex shader (pre-perspective-division, to be interpolated correctly by the rasterizer with linear/center interpolation), then viewport-mapped in the fragment shader using the same depth range constants as frag_depth clamping.

4. **The polyfill applies at the Tint IR level**: Both vertex and fragment shader wrappers are modified in the same ShaderIO transform pass, ensuring consistent interpolant layout.
