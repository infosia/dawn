# Shader pipeline summary

- Inputs:
  - WGSL modules from application or tests.
- Transforms:
  - WGSL parsed → Tint IR; IR passes; codegen to backend shader formats (SPIR-V for Vulkan, HLSL/DXIL for D3D12, MSL for Metal).
  - Validation: SPIR-V validated with spirv-val when applicable.
- Integration points in Dawn:
  - `ShaderModuleBase` stores WGSL source and the parsed Tint program (`ShaderModule.cpp`: `TintProgram`, `SetFailedCompilation`, `GetTintProgram`).
  - On pipeline creation (`RenderPipeline.cpp` / `ComputePipeline.cpp`), backend compilation requests codegen from Tint to target format (SPIR-V/HLSL/MSL) and streams results into cache keys.
  - Validation errors can be cached (`CachedValidationError`), short-circuiting repeated compiles.
- Outputs:
  - Backend-specific shader binaries / modules, reflection data for pipeline layouts.
- Failure modes:
  - WGSL parse / semantic errors; translation errors; backend compiler errors; validation failures.
- Debugging aids:
  - Toggles: dump_shaders, disable_symbol_renaming, emit_hlsl_debug_symbols.
  - Tracing via environment and backend tools (e.g., PIX, Xcode).

Notes
- SPIR-V validation (if enabled) runs after Tint SPIR-V generation; DXC/FXC toggles impact D3D12.
- Pipeline caches integrate shader binary hashes to avoid redundant backend compiles.
