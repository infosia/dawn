# Work Log: Polyfill @builtin(position) for Vulkan in Multisampled Fragment Shaders

## Steps

1. [x] Study the codebase and understand the task
2. [x] Inspector: Add `frag_position_used` to EntryPoint struct
3. [x] Inspector: Detect `@builtin(position)` usage in fragment shaders
4. [x] ShaderModule.h: Add `usesFragPosition` and `isFragMultiSampled` metadata fields
5. [x] ShaderModule.cpp: Propagate fragment position and multisampling info
6. [x] RenderPipeline.h: Add member variables and accessors
7. [x] RenderPipeline.cpp: Set new members and implement accessors
8. [x] SPIR-V writer options: Add `apply_pixel_center_polyfill`
9. [x] SPIR-V writer raise: Pass option to ShaderIO config
10. [x] SPIR-V ShaderIO transform: Core polyfill implementation
11. [x] Vulkan RenderPipeline: Compute `isSampled` and pass through
12. [x] Vulkan ShaderModule: Thread `isSampled` to Tint options
13. [x] Vulkan ComputePipeline: No change needed (default param = false)
14. [x] Vulkan Physical Device: Reserve extra inter-stage slot
15. [x] Base ShaderIO: Add `SetBackendOutputs` virtual method
16. [x] Tests: BUILD.gn, DawnTest.h, MultisampledInterpolationTests.cpp
17. [x] Tests: shader_io_test.cc unit tests (6 tests, all passing)
18. [x] Tests: ArchTierLimitsExhaustive.cpp updates (Vulkan entries)
19. [x] Remove CTS failure expectations (position tests for Pixel 6, Intel Linux, NVIDIA Linux)
20. [x] Build and test

## Test Results

### Tint Unit Tests
- All 23 SpirvWriter_ShaderIOTest tests pass (17 existing + 6 new)

### Dawn End2End Tests (Vulkan)
- MultisampledInterpolation tests: 3/3 passed (1 skipped on SwiftShader)
  - Intel: PASSED
  - NVIDIA: PASSED
  - llvmpipe: PASSED
- Multisampled/InterStage/Interpolation/FragmentBuiltin: 323 passed, 25 skipped, 0 failed
- Depth/FragDepth/RenderPipeline/VertexState: Pre-existing NVIDIA RTX 5090 stencil copy failures only (not related)

## Verification (Re-run)

### Tint Unit Tests (re-verified)
- All 23 SpirvWriter_ShaderIOTest tests pass (17 existing + 6 new ForcePixelCenters tests)

### Dawn End2End Tests - Vulkan (re-verified)
- MultisampledInterpolation: 3/3 passed (1 skipped on SwiftShader)
- Broader suite (Multisampled/InterStage/Interpolation/FragmentBuiltin/FragDepth): 347 passed, 25 skipped, 0 FAILED
- Full Vulkan end2end suite: 29,476 PASSED, 6,151 SKIPPED, 70 FAILED
  - All 70 failures are pre-existing hardware-specific issues (NVIDIA RTX 5090 stencil/SharedTexture/SubgroupMatrix, Intel ARL memory limit, llvmpipe limitations)
  - Zero failures related to the polyfill changes

## Final Verification (2026-02-15)

### Tint Unit Tests
- All 23 SpirvWriter_ShaderIOTest tests pass (17 existing + 6 new ForcePixelCenters tests)

### Dawn End2End Tests - Full Vulkan Suite
- **29,475 PASSED, 6,151 SKIPPED, 71 FAILED** out of 36,397 tests
- MultisampledInterpolation: 3/3 passed (1 skipped on SwiftShader)
- Multisampled/InterStage/Interpolation/FragmentBuiltin/FragDepth: 347 passed, 25 skipped, 0 FAILED
- All 71 failures are pre-existing hardware/driver issues:
  - NVIDIA RTX 5090: stencil copy (24), SharedTextureMemory (24), SubgroupMatrix (10), SubresourceRenderAttachment (1), DepthStencilLoadOp (8)
  - Intel ARL: BufferHostMappedPointer (1), MaxBufferBindingSize (1)
  - llvmpipe: MultiDrawIndexedIndirect (2)
- **Zero failures related to the polyfill changes**

## Commits
1. `2b2d34d418` - Main polyfill implementation (21 files, 465 insertions, 31 deletions)
2. `61785a88c0` - Unit tests for pixel center polyfill (6 tests)
