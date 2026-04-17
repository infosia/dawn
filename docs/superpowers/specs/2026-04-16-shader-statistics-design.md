# Shader Statistics: Compiled Shader Register Usage Reporting

## Overview

Add a Dawn-native feature that reports compiled shader register usage and related
statistics after pipeline creation. Backends expose what they can: Vulkan provides
full hardware register counts via `VK_KHR_pipeline_executable_properties`, D3D12
provides IL-level stats via shader reflection, and Metal provides indirect occupancy
signals.

## Motivation

Shader register pressure directly impacts GPU occupancy and performance. Developers
need visibility into how many registers their compiled shaders consume to diagnose
performance bottlenecks and make informed decisions about shader complexity. In a
later phase, Dawn itself can use these stats for runtime auto-tuning (e.g., adjusting
workgroup sizes based on register pressure).

## Backend Capability Analysis

### Vulkan — Full hardware stats

Source: `VK_KHR_pipeline_executable_properties` extension (KHR, supported on AMD,
NVIDIA, Intel desktop drivers; not on Qualcomm/ARM mobile).

Available data per pipeline executable:
- VGPR count (vector general-purpose registers)
- SGPR count (scalar general-purpose registers)
- Spilled VGPR/SGPR counts
- Scratch memory bytes (per-thread)
- Code size (ISA bytes)
- LDS/shared memory usage
- Subgroup size

Also supports ISA disassembly via internal representations API.

Requires:
- Device extension enabled at creation
- `pipelineExecutableInfo` feature enabled
- `VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR` flag on pipeline creation

Stat names are driver-defined strings, not standardized. Common names across drivers:
`"SGPRs"`, `"VGPRs"`, `"Spilled SGPRs"`, `"Spilled VGPRs"`, `"Scratch size"`,
`"Code size"`.

### D3D12 — IL-level statistics

Source: `ID3D12ShaderReflection` via `D3DReflect()` (DXBC) or
`IDxcUtils::CreateReflection()` (DXIL/SM6+).

Available from `D3D12_SHADER_DESC`:
- `TempRegisterCount` — virtual register count (DXIL IR level, not hardware)
- `InstructionCount` — total IL instructions
- `FloatInstructionCount`, `IntInstructionCount`, `UintInstructionCount`
- `StaticFlowControlCount`, `DynamicFlowControlCount`
- `TextureNormalInstructions`, `TextureLoadInstructions`, etc.
- `BarrierInstructions`, `InterlockedInstructions` (compute)

Limitations: These are intermediate representation stats. The vendor driver compiles
DXIL to hardware ISA privately; actual hardware register allocation is opaque.

### Metal — Indirect occupancy signals only

Available from `MTLComputePipelineState` / `MTLRenderPipelineState`:
- `maxTotalThreadsPerThreadgroup` — reduced from device max when register pressure
  is high (indirect occupancy signal)
- `threadExecutionWidth` — SIMD width (typically 32 on Apple GPU)
- `staticThreadgroupMemoryLength` — declared threadgroup memory bytes

Limitations: No register counts, no instruction counts, no scratch memory, no ISA
access. Apple GPU ISA is not publicly documented.

### Summary Table

| Capability               | Vulkan | D3D12               | Metal                 |
|--------------------------|--------|----------------------|-----------------------|
| Hardware VGPR/SGPR       | Yes    | No                   | No                    |
| Spill counts             | Yes    | No                   | No                    |
| Scratch memory           | Yes    | No                   | No                    |
| Code size (ISA)          | Yes    | No                   | No                    |
| Virtual register count   | N/A    | Yes (TempRegisterCount) | No                 |
| Instruction count        | No     | Yes (with breakdown) | No                    |
| Max threads (occupancy)  | Yes    | No                   | Yes                   |
| Threadgroup memory       | Yes    | No                       | Yes                   |
| SIMD/wave width          | Yes    | No                   | Yes                   |
| ISA disassembly          | Yes    | No                   | No                    |

## API Design

### Feature Gate

New Dawn feature: `wgpu::FeatureName::ShaderStatistics`

Must be requested at device creation. Physical device advertises support when the
backend can provide at least some statistics.

### Data Model (Hybrid: fixed core + extension bag)

```
enum ShaderStatisticFormat {
    Bool32,
    Int64,
    Uint64,
    Float64,
}

bitmask ShaderStatisticValidField {
    RegisterCount,
    ScalarRegisterCount,
    InstructionCount,
    CodeSizeBytes,
    ScratchMemoryBytes,
    ThreadgroupMemoryBytes,
    SimdWidth,
    MaxThreadsPerGroup,
}

struct ShaderStatisticsCore {
    validFields: ShaderStatisticValidField,
    registerCount: uint64,          // VGPR (Vulkan), TempRegisterCount (D3D12)
    scalarRegisterCount: uint64,    // SGPR (Vulkan only)
    instructionCount: uint64,       // D3D12 InstructionCount (IL ops)
    codeSizeBytes: uint64,          // Vulkan ISA code size in bytes
    scratchMemoryBytes: uint64,     // Vulkan scratch size
    threadgroupMemoryBytes: uint64, // all backends
    simdWidth: uint64,              // subgroupSize (Vk), threadExecutionWidth (Mtl)
    maxThreadsPerGroup: uint64,     // Metal occupancy, Vulkan subgroup info
}

// Value union — in dawn.json, represented as a struct with all fields;
// only the field matching `format` is valid. This matches Dawn's pattern
// for variant types in the C API (no true C union in the wire protocol).
struct ShaderStatisticValue {
    valueBool: bool,
    valueInt64: int64,
    valueUint64: uint64,
    valueFloat64: float64,
}

struct ShaderStatisticEntry {
    name: string,
    description: string,
    format: ShaderStatisticFormat,
    value: ShaderStatisticValue,
}

struct ShaderStageStatistics {
    stage: ShaderStage,
    core: ShaderStatisticsCore,
    extensionEntryCount: uint32,
    extensionEntries: ShaderStatisticEntry*,  // pointer + count pattern
}

struct ShaderStatistics {
    stageCount: uint32,
    stages: ShaderStageStatistics*,           // pointer + count pattern
}
```

### Pipeline Methods

```
ComputePipeline.GetShaderStatistics() -> ShaderStatistics
RenderPipeline.GetShaderStatistics() -> ShaderStatistics
```

Returns empty result if feature not enabled or backend has no data.

### Memory Ownership and Wire Protocol

`GetShaderStatistics()` returns a snapshot. Dawn internally caches the statistics
on first query (lazy evaluation) and returns a pointer to the cached data. The cache
lives as long as the pipeline object. The caller must not free the returned pointers.

For Dawn's client/server wire protocol, `ShaderStatistics` is serialized using the
standard pointer+count pattern already used by other Dawn structs. Strings are
serialized as length-prefixed byte arrays. The wire serialization code is
auto-generated from dawn.json like all other types.

### D3D11 Backend

D3D11 is explicitly excluded from this feature. While `src/dawn/native/d3d/` has
shared code between D3D11 and D3D12, the shader statistics implementation lives
entirely in the D3D12-specific layer (`src/dawn/native/d3d12/`). The D3D11
physical device does not advertise `ShaderStatistics` support.

## Implementation Architecture

### dawn.json

Add to the `"feature name"` enum:
- `"shader statistics"`

Add new types:
- `ShaderStatisticFormat` enum
- `ShaderStatisticValidField` bitmask
- `ShaderStatisticsCore` struct
- `ShaderStatisticEntry` struct
- `ShaderStageStatistics` struct
- `ShaderStatistics` struct

Add methods to `ComputePipeline` and `RenderPipeline`:
- `GetShaderStatistics` returning `ShaderStatistics`

### Core layer (`src/dawn/native/`)

`ComputePipelineBase` and `RenderPipelineBase`:
- Add virtual `GetShaderStatisticsImpl()` returning internal stats representation
- Add public `GetShaderStatistics()` that validates feature is enabled, then
  delegates to the virtual method
- Default implementation returns empty stats

### Vulkan backend (`src/dawn/native/vulkan/`)

**Extension registration:**
- Add `PipelineExecutableProperties` to `DeviceExt` enum in `VulkanExtensions.h`
- Query extension support and enable during device creation in
  `VulkanDeviceInfo` / `Device::Initialize`
- Chain `VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR` into device
  creation when the feature is requested

**Pipeline creation:**
- In `ComputePipelineVk::InitializeImpl` and `RenderPipelineVk::InitializeImpl`,
  add `VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR` to pipeline create flags
  when the feature is enabled

**Statistics query:**
- Override `GetShaderStatisticsImpl()` in both pipeline classes
- Call `vkGetPipelineExecutablePropertiesKHR` to enumerate executables
- For each executable, call `vkGetPipelineExecutableStatisticsKHR`
- Map Vulkan shader stage flags to WebGPU `ShaderStage`
- Map known stat names to core fields using exact string matches:
  - AMD (RADV/AMDVLK): `"VGPRs"` → `registerCount`, `"SGPRs"` →
    `scalarRegisterCount`, `"Scratch size"` → `scratchMemoryBytes`,
    `"Code size"` → `codeSizeBytes`, `"LDS size"` → `threadgroupMemoryBytes`,
    `"Spilled VGPRs"`, `"Spilled SGPRs"` → extension entries
  - NVIDIA: register/instruction stats vary by driver version → map any
    stat containing register keywords to core fields; remainder to extension
  - Intel (ANV): GRF-based stats → `registerCount`; spill/fill → extension
  - Unknown stat names always go to extension entries (safe fallback)
- Read `subgroupSize` from executable properties → `simdWidth`
- Handle merged executables: when a Vulkan executable covers multiple stages
  (e.g., VS+FS merged), duplicate the statistics into each corresponding
  `ShaderStageStatistics` entry so the consumer always gets per-stage data

### D3D12 backend (`src/dawn/native/d3d12/`)

**Shader reflection capture:**
- After compiling HLSL → DXBC/DXIL, call `D3DReflect()` or DXC reflection API
- Extract `D3D12_SHADER_DESC` and store alongside the pipeline state

**Statistics query:**
- Override `GetShaderStatisticsImpl()`
- Map reflection data to core fields:
  - `TempRegisterCount` → `registerCount`
  - `InstructionCount` → `instructionCount`
- Put typed instruction breakdowns (float, int, uint, texture, flow control)
  into `extensionEntries`

### Metal backend (`src/dawn/native/metal/`)

**Pipeline property capture:**
- After pipeline creation, read properties from the Metal pipeline state object
- Store or query on-demand (properties are cheap to read)

**Statistics query:**
- Override `GetShaderStatisticsImpl()`
- Map Metal properties to core fields:
  - `maxTotalThreadsPerThreadgroup` → `maxThreadsPerGroup`
  - `threadExecutionWidth` → `simdWidth`
  - `staticThreadgroupMemoryLength` → `threadgroupMemoryBytes`
- No extension entries

## Phased Delivery

### Branch 1: Developer Profiling

Feature flag + read-only query API. All three backends. No impact on pipeline
creation performance when feature is not enabled. When enabled on Vulkan, adds
capture statistics flag to pipeline creation (documented performance note: may
increase pipeline creation time and memory usage).

### Branch 2: Runtime Auto-Tuning

Internal APIs for Dawn to query stats and adjust behavior. Vulkan-first.
Potential auto-tuning decisions:
- Adjust compute dispatch workgroup size based on register pressure
- Select shader variants based on occupancy estimates
- Log warnings when register spilling is detected

### Branch 3: Unified

Merge profiling and auto-tuning into a single feature. Expose both the query API
(for developers) and internal auto-tuning (for Dawn runtime), controlled by the
same feature flag.

## Risks and Mitigations

- **Pipeline creation overhead (Vulkan):** `CAPTURE_STATISTICS_BIT_KHR` may slow
  pipeline creation, increase memory, and affect pipeline cache behavior (cached
  pipelines created without the flag won't have stats; the flag becomes part of
  the cache key). Mitigated by gating behind opt-in feature flag.
- **Non-standardized stat names (Vulkan):** Driver-specific strings for statistics.
  Mitigated by exact string matching for known drivers with fallback to extension
  entries for unknown names.
- **Uneven backend coverage:** Metal exposes very little. Mitigated by `validFields`
  bitmask — consumers check what's available.
- **Mobile Vulkan:** Qualcomm/ARM don't support the extension. Physical device
  won't advertise the feature on those devices.

## Testing Strategy

- Unit tests: verify core struct population, field validity, empty results when
  feature is disabled
- End-to-end tests (Vulkan): create pipeline with feature enabled, verify
  non-empty statistics are returned, verify known stat names appear
- End-to-end tests (D3D12/Metal): verify backend-appropriate fields are populated
- Negative tests: calling `GetShaderStatistics` without feature enabled returns
  empty result (not an error)
- Wire tests: verify `ShaderStatistics` round-trips correctly through Dawn's
  client/server wire protocol, including variable-length arrays and strings
