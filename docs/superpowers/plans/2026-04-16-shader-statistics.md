# Shader Statistics Implementation Plan (Branch 1: Developer Profiling)

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `ShaderStatistics` Dawn-native feature that reports compiled shader register usage and related statistics from pipeline objects across Vulkan, D3D12, and Metal backends.

**Architecture:** New feature gated behind `wgpu::FeatureName::ShaderStatistics`. A `GetShaderStatistics()` method on pipeline objects delegates to a virtual backend method. Vulkan uses `VK_KHR_pipeline_executable_properties`, D3D12 uses shader reflection, Metal reads pipeline state properties. Hybrid data model: fixed core struct with validity bitmask + extension key-value bag for driver-specific stats.

**Tech Stack:** C++20, Dawn codegen (dawn.json → generator), Vulkan API, D3D12 shader reflection, Metal pipeline state API.

**Spec:** `docs/superpowers/specs/2026-04-16-shader-statistics-design.md`

**Commit strategy:** Per project CLAUDE.md, one commit per CL. Task 1 creates the initial commit. All subsequent tasks (2–10) amend that commit via `git commit --amend --no-edit`.

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `src/dawn/dawn.json` | Modify | Feature enum, new structs/enums, pipeline methods |
| `src/dawn/native/ShaderStatistics.h` | Create | Internal `ShaderStatisticsData` owning structs |
| `src/dawn/native/ShaderStatistics.cpp` | Create | `ShaderStatisticsData` → C API conversion + caching logic |
| `src/dawn/native/ComputePipeline.h` | Modify | Add virtual `GetShaderStatisticsImpl` + public API entry |
| `src/dawn/native/ComputePipeline.cpp` | Modify | Default impl returning empty |
| `src/dawn/native/RenderPipeline.h` | Modify | Same as ComputePipeline |
| `src/dawn/native/RenderPipeline.cpp` | Modify | Same as ComputePipeline |
| `src/dawn/native/BUILD.gn` | Modify | Add ShaderStatistics.h/.cpp |
| `src/dawn/native/vulkan/VulkanExtensions.h` | Modify | Add `PipelineExecutableProperties` to `DeviceExt` |
| `src/dawn/native/vulkan/VulkanExtensions.cpp` | Modify | Extension name mapping |
| `src/dawn/native/vulkan/VulkanInfo.h` | Modify | Add feature struct field |
| `src/dawn/native/vulkan/VulkanInfo.cpp` | Modify | Chain feature struct |
| `src/dawn/native/vulkan/PhysicalDeviceVk.cpp` | Modify | Advertise feature |
| `src/dawn/native/vulkan/DeviceVk.cpp` | Modify | Enable extension at device creation |
| `src/dawn/native/vulkan/ComputePipelineVk.h` | Modify | Declare override |
| `src/dawn/native/vulkan/ComputePipelineVk.cpp` | Modify | Capture flag + stats query |
| `src/dawn/native/vulkan/RenderPipelineVk.h` | Modify | Declare override |
| `src/dawn/native/vulkan/RenderPipelineVk.cpp` | Modify | Capture flag + stats query |
| `src/dawn/native/d3d12/PhysicalDeviceD3D12.cpp` | Modify | Advertise feature |
| `src/dawn/native/d3d12/ComputePipelineD3D12.h` | Modify | Declare override + reflection storage |
| `src/dawn/native/d3d12/ComputePipelineD3D12.cpp` | Modify | Reflection capture + stats query |
| `src/dawn/native/d3d12/RenderPipelineD3D12.h` | Modify | Same |
| `src/dawn/native/d3d12/RenderPipelineD3D12.cpp` | Modify | Same |
| `src/dawn/native/d3d/ShaderUtils.cpp` | Modify | Return reflection data from compilation |
| `src/dawn/native/metal/PhysicalDeviceMTL.mm` | Modify | Advertise feature |
| `src/dawn/native/metal/ComputePipelineMTL.h` | Modify | Declare override |
| `src/dawn/native/metal/ComputePipelineMTL.mm` | Modify | Stats from MTL pipeline state |
| `src/dawn/native/metal/RenderPipelineMTL.h` | Modify | Same |
| `src/dawn/native/metal/RenderPipelineMTL.mm` | Modify | Same |
| `src/dawn/tests/unittests/validation/ShaderStatisticsValidationTests.cpp` | Create | Validation tests |
| `src/dawn/tests/unittests/validation/BUILD.gn` | Modify | Add test source |
| `src/dawn/tests/end2end/ShaderStatisticsTests.cpp` | Create | End-to-end tests |
| `src/dawn/tests/end2end/BUILD.gn` | Modify | Add test source |
| `src/dawn/tests/unittests/wire/WireShaderStatisticsTests.cpp` | Create | Wire round-trip tests |
| `src/dawn/tests/unittests/wire/BUILD.gn` | Modify | Add test source |

**Note:** `src/dawn/native/Features.cpp` is auto-generated from `dawn.json` — no manual edit needed. D3D11 backend is explicitly excluded; all D3D12-specific code lives in `src/dawn/native/d3d12/`, not in the shared `src/dawn/native/d3d/` layer.

---

## Task 1: dawn.json — Define Types and Feature

**Files:**
- Modify: `src/dawn/dawn.json`

- [ ] **Step 1: Add feature enum value**

In the `"feature name"` enum `"values"` array, add a new entry. Find the last value number and use the next one:
```json
{"value": <next>, "name": "shader statistics", "tags": ["dawn"]}
```

- [ ] **Step 2: Add ShaderStatisticFormat enum**

```json
"shader statistic format": {
    "category": "enum",
    "tags": ["dawn"],
    "values": [
        {"value": 0, "name": "undefined", "valid": false},
        {"value": 1, "name": "bool32"},
        {"value": 2, "name": "int64"},
        {"value": 3, "name": "uint64"},
        {"value": 4, "name": "float64"}
    ]
}
```

- [ ] **Step 3: Add ShaderStatisticValidField bitmask**

```json
"shader statistic valid field": {
    "category": "bitmask",
    "tags": ["dawn"],
    "values": [
        {"value": 0, "name": "none"},
        {"value": 1, "name": "register count"},
        {"value": 2, "name": "scalar register count"},
        {"value": 4, "name": "instruction count"},
        {"value": 8, "name": "code size bytes"},
        {"value": 16, "name": "scratch memory bytes"},
        {"value": 32, "name": "threadgroup memory bytes"},
        {"value": 64, "name": "simd width"},
        {"value": 128, "name": "max threads per group"}
    ]
}
```

- [ ] **Step 4: Add ShaderStatisticValue struct**

```json
"shader statistic value": {
    "category": "structure",
    "tags": ["dawn"],
    "members": [
        {"name": "value bool", "type": "bool", "default": "false"},
        {"name": "value int64", "type": "int64_t", "default": 0},
        {"name": "value uint64", "type": "uint64_t", "default": 0},
        {"name": "value float64", "type": "double", "default": 0}
    ]
}
```

- [ ] **Step 5: Add ShaderStatisticsCore struct**

```json
"shader statistics core": {
    "category": "structure",
    "tags": ["dawn"],
    "members": [
        {"name": "valid fields", "type": "shader statistic valid field"},
        {"name": "register count", "type": "uint64_t", "default": 0},
        {"name": "scalar register count", "type": "uint64_t", "default": 0},
        {"name": "instruction count", "type": "uint64_t", "default": 0},
        {"name": "code size bytes", "type": "uint64_t", "default": 0},
        {"name": "scratch memory bytes", "type": "uint64_t", "default": 0},
        {"name": "threadgroup memory bytes", "type": "uint64_t", "default": 0},
        {"name": "simd width", "type": "uint64_t", "default": 0},
        {"name": "max threads per group", "type": "uint64_t", "default": 0}
    ]
}
```

- [ ] **Step 6: Add ShaderStatisticEntry struct**

```json
"shader statistic entry": {
    "category": "structure",
    "tags": ["dawn"],
    "members": [
        {"name": "name", "type": "string view"},
        {"name": "description", "type": "string view"},
        {"name": "format", "type": "shader statistic format"},
        {"name": "value", "type": "shader statistic value"}
    ]
}
```

- [ ] **Step 7: Add ShaderStageStatistics struct**

```json
"shader stage statistics": {
    "category": "structure",
    "tags": ["dawn"],
    "members": [
        {"name": "stage", "type": "shader stage"},
        {"name": "core", "type": "shader statistics core"},
        {"name": "extension entry count", "type": "size_t", "default": 0},
        {"name": "extension entries", "type": "shader statistic entry", "annotation": "const*", "length": "extension entry count", "default": "nullptr"}
    ]
}
```

- [ ] **Step 8: Add ShaderStatistics struct**

```json
"shader statistics": {
    "category": "structure",
    "tags": ["dawn"],
    "members": [
        {"name": "stage count", "type": "size_t", "default": 0},
        {"name": "stages", "type": "shader stage statistics", "annotation": "const*", "length": "stage count", "default": "nullptr"}
    ]
}
```

- [ ] **Step 9: Add GetShaderStatistics method to ComputePipeline and RenderPipeline**

In the `"compute pipeline"` object's `"methods"` array:
```json
{
    "name": "get shader statistics",
    "tags": ["dawn"],
    "returns": "shader statistics"
}
```

Add the same method to `"render pipeline"` object's `"methods"` array.

- [ ] **Step 10: Regenerate and verify build**

Run: `ninja -C out/Debug`
Expected: Build succeeds with new generated types.

- [ ] **Step 11: Commit**

```bash
git add src/dawn/dawn.json
git commit -m "dawn.json: add ShaderStatistics feature types and pipeline methods"
```

---

## Task 2: Core Layer — Internal Types, Virtual Method, and C API Conversion

**Files:**
- Create: `src/dawn/native/ShaderStatistics.h`
- Create: `src/dawn/native/ShaderStatistics.cpp`
- Modify: `src/dawn/native/BUILD.gn`
- Modify: `src/dawn/native/ComputePipeline.h`
- Modify: `src/dawn/native/ComputePipeline.cpp`
- Modify: `src/dawn/native/RenderPipeline.h`
- Modify: `src/dawn/native/RenderPipeline.cpp`

- [ ] **Step 1: Create ShaderStatistics.h with internal owning types**

```cpp
// src/dawn/native/ShaderStatistics.h
#ifndef SRC_DAWN_NATIVE_SHADERSTATISTICS_H_
#define SRC_DAWN_NATIVE_SHADERSTATISTICS_H_

#include <string>
#include <vector>
#include "dawn/webgpu.h"

namespace dawn::native {

struct ShaderStatisticEntryData {
    std::string name;
    std::string description;
    wgpu::ShaderStatisticFormat format;
    wgpu::ShaderStatisticValue value;
};

struct ShaderStageStatisticsData {
    wgpu::ShaderStage stage;
    wgpu::ShaderStatisticsCore core = {};
    std::vector<ShaderStatisticEntryData> extensionEntries;
};

struct ShaderStatisticsData {
    std::vector<ShaderStageStatisticsData> stages;
};

// Cached conversion result that holds stable pointers for the C API.
// Lives as long as the pipeline object that owns it.
struct ShaderStatisticsCache {
    ShaderStatisticsData data;

    // Flattened C API arrays built from `data`. Pointers into these vectors
    // are returned by the public API and remain valid for the cache lifetime.
    std::vector<wgpu::ShaderStatisticEntry> flatEntries;
    std::vector<wgpu::ShaderStageStatistics> flatStages;

    // Builds flatEntries and flatStages from `data`.
    // Must be called once after `data` is populated.
    void BuildCApiView();

    // Returns the C API struct with pointers into this cache.
    wgpu::ShaderStatistics GetCApiView() const;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_SHADERSTATISTICS_H_
```

- [ ] **Step 2: Create ShaderStatistics.cpp with conversion logic**

```cpp
// src/dawn/native/ShaderStatistics.cpp
#include "dawn/native/ShaderStatistics.h"

namespace dawn::native {

void ShaderStatisticsCache::BuildCApiView() {
    flatStages.clear();
    flatEntries.clear();

    // First pass: count total extension entries to pre-allocate
    size_t totalEntries = 0;
    for (const auto& stage : data.stages) {
        totalEntries += stage.extensionEntries.size();
    }
    flatEntries.reserve(totalEntries);

    // Second pass: build flat arrays
    for (const auto& stageData : data.stages) {
        // Copy extension entries into flat array
        size_t entryOffset = flatEntries.size();
        for (const auto& entry : stageData.extensionEntries) {
            wgpu::ShaderStatisticEntry cEntry = {};
            cEntry.name = {entry.name.data(), entry.name.size()};
            cEntry.description = {entry.description.data(), entry.description.size()};
            cEntry.format = entry.format;
            cEntry.value = entry.value;
            flatEntries.push_back(cEntry);
        }

        wgpu::ShaderStageStatistics cStage = {};
        cStage.stage = stageData.stage;
        cStage.core = stageData.core;
        cStage.extensionEntryCount = stageData.extensionEntries.size();
        cStage.extensionEntries = stageData.extensionEntries.empty()
            ? nullptr
            : &flatEntries[entryOffset];
        flatStages.push_back(cStage);
    }
}

wgpu::ShaderStatistics ShaderStatisticsCache::GetCApiView() const {
    wgpu::ShaderStatistics result = {};
    result.stageCount = flatStages.size();
    result.stages = flatStages.empty() ? nullptr : flatStages.data();
    return result;
}

}  // namespace dawn::native
```

- [ ] **Step 3: Add to BUILD.gn**

In `src/dawn/native/BUILD.gn`, in the sources list, add:
```
"ShaderStatistics.cpp",
"ShaderStatistics.h",
```

- [ ] **Step 4: Add virtual method and cache to ComputePipelineBase**

In `src/dawn/native/ComputePipeline.h`, add to `ComputePipelineBase`:
```cpp
#include "dawn/native/ShaderStatistics.h"

  public:
    wgpu::ShaderStatistics APIGetShaderStatistics();

  protected:
    virtual ShaderStatisticsData GetShaderStatisticsImpl() const;

  private:
    mutable std::optional<ShaderStatisticsCache> mShaderStatisticsCache;
```

In `src/dawn/native/ComputePipeline.cpp`:
```cpp
ShaderStatisticsData ComputePipelineBase::GetShaderStatisticsImpl() const {
    return {};
}

wgpu::ShaderStatistics ComputePipelineBase::APIGetShaderStatistics() {
    if (!GetDevice()->HasFeature(Feature::ShaderStatistics)) {
        return {};
    }
    if (!mShaderStatisticsCache.has_value()) {
        mShaderStatisticsCache.emplace();
        mShaderStatisticsCache->data = GetShaderStatisticsImpl();
        mShaderStatisticsCache->BuildCApiView();
    }
    return mShaderStatisticsCache->GetCApiView();
}
```

- [ ] **Step 5: Add same virtual method and cache to RenderPipelineBase**

Same pattern in `src/dawn/native/RenderPipeline.h` and `.cpp`.

- [ ] **Step 6: Build and verify**

Run: `ninja -C out/Debug`
Expected: Compiles. No functional change yet.

- [ ] **Step 7: Commit**

```bash
git add src/dawn/native/ShaderStatistics.h src/dawn/native/ShaderStatistics.cpp \
        src/dawn/native/BUILD.gn \
        src/dawn/native/ComputePipeline.h src/dawn/native/ComputePipeline.cpp \
        src/dawn/native/RenderPipeline.h src/dawn/native/RenderPipeline.cpp
git commit --amend --no-edit
```

---

## Task 3: Vulkan — Extension Registration and Feature Advertisement

**Files:**
- Modify: `src/dawn/native/vulkan/VulkanExtensions.h`
- Modify: `src/dawn/native/vulkan/VulkanExtensions.cpp`
- Modify: `src/dawn/native/vulkan/VulkanInfo.h`
- Modify: `src/dawn/native/vulkan/VulkanInfo.cpp`
- Modify: `src/dawn/native/vulkan/PhysicalDeviceVk.cpp`
- Modify: `src/dawn/native/vulkan/DeviceVk.cpp`

- [ ] **Step 1: Add DeviceExt enum value**

In `VulkanExtensions.h`, add `PipelineExecutableProperties` to the `DeviceExt` enum before `EnumCount`, in the "Others" section:
```cpp
PipelineExecutableProperties,
```

- [ ] **Step 2: Register extension name**

In `VulkanExtensions.cpp`, in `CreateDeviceExtNameMap()` or equivalent, add:
```cpp
{DeviceExt::PipelineExecutableProperties, "VK_KHR_pipeline_executable_properties"},
```

Also add dependency handling in `EnsureDependencies()` if needed (this extension requires `VK_KHR_get_physical_device_properties2`, which Dawn already requires via Vulkan 1.1+).

- [ ] **Step 3: Add feature struct to VulkanDeviceKnobs**

In `VulkanInfo.h`, inside `VulkanDeviceKnobs`:
```cpp
VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pipelineExecutablePropertiesFeatures = {};
```

- [ ] **Step 4: Chain feature struct during device info gathering**

In `VulkanInfo.cpp`, in the feature chaining section:
```cpp
if (info.extensions[DeviceExt::PipelineExecutableProperties]) {
    featuresChain.Add(&info.pipelineExecutablePropertiesFeatures,
                      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR);
}
```

- [ ] **Step 5: Advertise feature in PhysicalDeviceVk.cpp**

In `PhysicalDeviceVk.cpp`, in the feature initialization section:
```cpp
if (mDeviceInfo.HasExt(DeviceExt::PipelineExecutableProperties) &&
    mDeviceInfo.pipelineExecutablePropertiesFeatures.pipelineExecutableInfo == VK_TRUE) {
    EnableFeature(Feature::ShaderStatistics);
}
```

- [ ] **Step 6: Enable extension and feature at device creation**

In `DeviceVk.cpp`, where extensions are enabled and feature structs chained:
```cpp
if (IsFeatureEnabled(Feature::ShaderStatistics)) {
    extensionsToRequest.push_back(DeviceExt::PipelineExecutableProperties);
    usedKnobs.pipelineExecutablePropertiesFeatures.pipelineExecutableInfo = VK_TRUE;
    featuresChain.Add(&usedKnobs.pipelineExecutablePropertiesFeatures,
                      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR);
}
```

- [ ] **Step 7: Build and verify**

Run: `ninja -C out/Debug`
Expected: Compiles. Feature advertised on capable Vulkan devices.

- [ ] **Step 8: Commit**

```bash
git add src/dawn/native/vulkan/VulkanExtensions.h src/dawn/native/vulkan/VulkanExtensions.cpp \
        src/dawn/native/vulkan/VulkanInfo.h src/dawn/native/vulkan/VulkanInfo.cpp \
        src/dawn/native/vulkan/PhysicalDeviceVk.cpp src/dawn/native/vulkan/DeviceVk.cpp
git commit --amend --no-edit
```

---

## Task 4: Vulkan — Pipeline Creation Flag + Statistics Query

**Files:**
- Modify: `src/dawn/native/vulkan/ComputePipelineVk.h`
- Modify: `src/dawn/native/vulkan/ComputePipelineVk.cpp`
- Modify: `src/dawn/native/vulkan/RenderPipelineVk.h`
- Modify: `src/dawn/native/vulkan/RenderPipelineVk.cpp`

- [ ] **Step 1: Add capture flag to ComputePipeline creation**

In `ComputePipelineVk.cpp`, in `InitializeSpecialization` where `createInfo.flags = 0`:
```cpp
if (GetDevice()->HasFeature(Feature::ShaderStatistics)) {
    createInfo.flags |= VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
}
```

- [ ] **Step 2: Add capture flag to RenderPipeline creation**

Same pattern in `RenderPipelineVk.cpp`, in `InitializeSpecialization`.

- [ ] **Step 3: Implement GetShaderStatisticsImpl for ComputePipeline (Vulkan)**

Override in `ComputePipelineVk.h`:
```cpp
ShaderStatisticsData GetShaderStatisticsImpl() const override;
```

Implementation in `ComputePipelineVk.cpp`:
1. Call `device->fn.GetPipelineExecutablePropertiesKHR` (two-call pattern: first with nullptr to get count, allocate, call again)
2. For each executable, call `device->fn.GetPipelineExecutableStatisticsKHR` (same two-call pattern)
3. Map the executable's `stages` field to `wgpu::ShaderStage::Compute`
4. For each statistic, map name to core fields using exact string matches:

| Driver | Stat name | Core field | Valid bit |
|--------|-----------|------------|-----------|
| AMD (RADV/AMDVLK) | `"VGPRs"` | `registerCount` | `RegisterCount` |
| AMD | `"SGPRs"` | `scalarRegisterCount` | `ScalarRegisterCount` |
| AMD | `"Scratch size"` | `scratchMemoryBytes` | `ScratchMemoryBytes` |
| AMD | `"Code size"` | `codeSizeBytes` | `CodeSizeBytes` |
| AMD | `"LDS size"` | `threadgroupMemoryBytes` | `ThreadgroupMemoryBytes` |
| Intel (ANV) | `"GRFs"` | `registerCount` | `RegisterCount` |
| Intel | `"Scratch size"` | `scratchMemoryBytes` | `ScratchMemoryBytes` |
| Intel | `"ISA size"` | `codeSizeBytes` | `CodeSizeBytes` |

Any stat name not in this table → push to `extensionEntries` with the driver-provided name, description, format, and value. This is the safe fallback for NVIDIA (whose stat names vary by driver version) and any unknown driver.

5. Set `core.simdWidth` from executable properties' `subgroupSize` field + `SimdWidth` valid bit
6. Return populated `ShaderStatisticsData`

- [ ] **Step 4: Implement GetShaderStatisticsImpl for RenderPipeline (Vulkan)**

Same query logic, but map Vulkan stage flags to WebGPU stages:
- `VK_SHADER_STAGE_VERTEX_BIT` → `wgpu::ShaderStage::Vertex`
- `VK_SHADER_STAGE_FRAGMENT_BIT` → `wgpu::ShaderStage::Fragment`
- If an executable covers multiple stages (merged, indicated by multiple bits set in `stages`), duplicate the statistics into each corresponding `ShaderStageStatisticsData` entry

- [ ] **Step 5: Build and verify**

Run: `ninja -C out/Debug`
Expected: Compiles.

- [ ] **Step 6: Commit**

```bash
git add src/dawn/native/vulkan/ComputePipelineVk.h src/dawn/native/vulkan/ComputePipelineVk.cpp \
        src/dawn/native/vulkan/RenderPipelineVk.h src/dawn/native/vulkan/RenderPipelineVk.cpp
git commit --amend --no-edit
```

---

## Task 5: D3D12 — Shader Reflection + Statistics Query

**Files:**
- Modify: `src/dawn/native/d3d12/PhysicalDeviceD3D12.cpp`
- Modify: `src/dawn/native/d3d/ShaderUtils.cpp` (or `src/dawn/native/d3d/ShaderUtils.h`)
- Modify: `src/dawn/native/d3d12/ComputePipelineD3D12.h`
- Modify: `src/dawn/native/d3d12/ComputePipelineD3D12.cpp`
- Modify: `src/dawn/native/d3d12/RenderPipelineD3D12.h`
- Modify: `src/dawn/native/d3d12/RenderPipelineD3D12.cpp`

- [ ] **Step 1: Advertise feature**

In `PhysicalDeviceD3D12.cpp`, unconditionally enable (shader reflection is always available):
```cpp
EnableFeature(Feature::ShaderStatistics);
```

- [ ] **Step 2: Capture shader reflection during compilation**

In `ShaderUtils.cpp`, after `CompileShaderDXC` or `CompileShaderFXC` returns the compiled blob, call `D3DReflect` (FXC path) or `IDxcUtils::CreateReflection` (DXC path) to obtain `ID3D12ShaderReflection`. Call `GetDesc()` to extract `D3D12_SHADER_DESC`. Store a lightweight copy of the relevant fields in the `CompiledShader` struct (or a new companion struct).

Fields to capture from `D3D12_SHADER_DESC`:
- `TempRegisterCount`, `InstructionCount`
- `FloatInstructionCount`, `IntInstructionCount`, `UintInstructionCount`
- `StaticFlowControlCount`, `DynamicFlowControlCount`
- `TextureNormalInstructions`, `TextureLoadInstructions`, `TextureCompInstructions`
- `cBarrierInstructions`, `cInterlockedInstructions`

- [ ] **Step 3: Implement GetShaderStatisticsImpl for ComputePipeline (D3D12)**

Override in `ComputePipelineD3D12.h`:
```cpp
ShaderStatisticsData GetShaderStatisticsImpl() const override;
```

Mapping:
- `TempRegisterCount` → `core.registerCount` + `RegisterCount` valid bit
- `InstructionCount` → `core.instructionCount` + `InstructionCount` valid bit
- All typed instruction counts → `extensionEntries` with `Uint64` format:
  - `{"FloatInstructionCount", "...", Uint64, value}` etc.
  - `cBarrierInstructions`, `cInterlockedInstructions` also as extension entries
- Note: D3D12 shader reflection does not expose threadgroup/shared memory size directly.
  `D3D12_SHADER_DESC` has no groupshared memory field. Do not set `ThreadgroupMemoryBytes` valid bit.
- Stage: `wgpu::ShaderStage::Compute`

- [ ] **Step 4: Implement GetShaderStatisticsImpl for RenderPipeline (D3D12)**

Same mapping, but produce entries for both vertex and fragment stages using their respective reflection data captured during pipeline creation.

- [ ] **Step 5: Build and verify**

Run: `ninja -C out/Debug`
Expected: Compiles.

- [ ] **Step 6: Commit**

```bash
git add src/dawn/native/d3d12/PhysicalDeviceD3D12.cpp \
        src/dawn/native/d3d/ShaderUtils.cpp src/dawn/native/d3d/ShaderUtils.h \
        src/dawn/native/d3d12/ComputePipelineD3D12.h src/dawn/native/d3d12/ComputePipelineD3D12.cpp \
        src/dawn/native/d3d12/RenderPipelineD3D12.h src/dawn/native/d3d12/RenderPipelineD3D12.cpp
git commit --amend --no-edit
```

---

## Task 6: Metal — Pipeline State Properties

**Files:**
- Modify: `src/dawn/native/metal/PhysicalDeviceMTL.mm`
- Modify: `src/dawn/native/metal/ComputePipelineMTL.h`
- Modify: `src/dawn/native/metal/ComputePipelineMTL.mm`
- Modify: `src/dawn/native/metal/RenderPipelineMTL.h`
- Modify: `src/dawn/native/metal/RenderPipelineMTL.mm`

- [ ] **Step 1: Advertise feature**

In `PhysicalDeviceMTL.mm`, unconditionally enable:
```cpp
EnableFeature(Feature::ShaderStatistics);
```

- [ ] **Step 2: Implement GetShaderStatisticsImpl for ComputePipeline (Metal)**

Override in `ComputePipelineMTL.h`:
```cpp
ShaderStatisticsData GetShaderStatisticsImpl() const override;
```

Read from `mMtlComputePipelineState`:
```objc
ShaderStageStatisticsData stageData;
stageData.stage = wgpu::ShaderStage::Compute;
stageData.core.maxThreadsPerGroup = [mMtlComputePipelineState.Get() maxTotalThreadsPerThreadgroup];
stageData.core.simdWidth = [mMtlComputePipelineState.Get() threadExecutionWidth];
stageData.core.threadgroupMemoryBytes = [mMtlComputePipelineState.Get() staticThreadgroupMemoryLength];
stageData.core.validFields = wgpu::ShaderStatisticValidField::MaxThreadsPerGroup |
                             wgpu::ShaderStatisticValidField::SimdWidth |
                             wgpu::ShaderStatisticValidField::ThreadgroupMemoryBytes;
```

No extension entries.

- [ ] **Step 3: Implement GetShaderStatisticsImpl for RenderPipeline (Metal)**

Override in `RenderPipelineMTL.h`. For render pipelines, Metal exposes less:
- No `maxTotalThreadsPerThreadgroup` on render pipeline state
- `threadExecutionWidth` may be available on mesh shader pipelines only
- Set only available valid bits. If no meaningful data is available, return empty stages (the feature is still advertised because compute pipelines work).

- [ ] **Step 4: Build and verify**

Run: `ninja -C out/Debug`
Expected: Compiles.

- [ ] **Step 5: Commit**

```bash
git add src/dawn/native/metal/PhysicalDeviceMTL.mm \
        src/dawn/native/metal/ComputePipelineMTL.h src/dawn/native/metal/ComputePipelineMTL.mm \
        src/dawn/native/metal/RenderPipelineMTL.h src/dawn/native/metal/RenderPipelineMTL.mm
git commit --amend --no-edit
```

---

## Task 7: Validation Tests

**Files:**
- Create: `src/dawn/tests/unittests/validation/ShaderStatisticsValidationTests.cpp`
- Modify: `src/dawn/tests/unittests/validation/BUILD.gn` — add `"ShaderStatisticsValidationTests.cpp"` to sources

- [ ] **Step 1: Add test file to BUILD.gn**

In `src/dawn/tests/unittests/validation/BUILD.gn`, add to the sources list:
```
"ShaderStatisticsValidationTests.cpp",
```

- [ ] **Step 2: Write test — returns empty when feature not requested**

Reference pattern: `src/dawn/tests/unittests/validation/BufferValidationTests.cpp`

```cpp
#include "dawn/tests/unittests/validation/ValidationTest.h"

class ShaderStatisticsValidationTest : public ValidationTest {};

TEST_F(ShaderStatisticsValidationTest, ReturnsEmptyWithoutFeature) {
    // Default device does not have ShaderStatistics feature
    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @compute @workgroup_size(1) fn main() {}
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);
    auto stats = pipeline.GetShaderStatistics();
    EXPECT_EQ(stats.stageCount, 0u);
}
```

- [ ] **Step 3: Write test — succeeds when feature is requested**

```cpp
TEST_F(ShaderStatisticsValidationTest, SucceedsWithFeature) {
    // Create device with ShaderStatistics feature
    wgpu::DeviceDescriptor deviceDesc;
    wgpu::FeatureName feature = wgpu::FeatureName::ShaderStatistics;
    deviceDesc.requiredFeatureCount = 1;
    deviceDesc.requiredFeatures = &feature;
    // ... create device, create compute pipeline ...
    auto stats = pipeline.GetShaderStatistics();
    EXPECT_GE(stats.stageCount, 1u);
    EXPECT_EQ(stats.stages[0].stage, wgpu::ShaderStage::Compute);
}
```

- [ ] **Step 4: Write test — RenderPipeline returns per-stage statistics**

```cpp
TEST_F(ShaderStatisticsValidationTest, RenderPipelineHasPerStageStats) {
    // Create device with feature, create render pipeline with VS + FS
    auto stats = pipeline.GetShaderStatistics();
    // Verify entries exist for Vertex and Fragment stages
    bool hasVertex = false, hasFragment = false;
    for (size_t i = 0; i < stats.stageCount; i++) {
        if (stats.stages[i].stage == wgpu::ShaderStage::Vertex) hasVertex = true;
        if (stats.stages[i].stage == wgpu::ShaderStage::Fragment) hasFragment = true;
    }
    EXPECT_TRUE(hasVertex);
    EXPECT_TRUE(hasFragment);
}
```

- [ ] **Step 5: Run tests**

Run: `out/Debug/dawn_unittests --gtest_filter=ShaderStatisticsValidation*`
Expected: All pass.

- [ ] **Step 6: Commit**

```bash
git add src/dawn/tests/unittests/validation/ShaderStatisticsValidationTests.cpp \
        src/dawn/tests/unittests/validation/BUILD.gn
git commit --amend --no-edit
```

---

## Task 8: End-to-End Tests

**Files:**
- Create: `src/dawn/tests/end2end/ShaderStatisticsTests.cpp`
- Modify: `src/dawn/tests/end2end/BUILD.gn` — add `"ShaderStatisticsTests.cpp"` to sources

Reference pattern: `src/dawn/tests/end2end/BasicTests.cpp`

- [ ] **Step 1: Add test file to BUILD.gn**

In `src/dawn/tests/end2end/BUILD.gn`, add to the sources list:
```
"ShaderStatisticsTests.cpp",
```

- [ ] **Step 2: Write test — compute pipeline reports non-empty stats**

```cpp
#include "dawn/tests/DawnTest.h"

class ShaderStatisticsTests : public DawnTest {
  protected:
    std::vector<wgpu::FeatureName> GetRequiredFeatures() override {
        if (!SupportsFeatures({wgpu::FeatureName::ShaderStatistics})) {
            return {};
        }
        return {wgpu::FeatureName::ShaderStatistics};
    }
};

TEST_P(ShaderStatisticsTests, ComputePipelineReportsStats) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));

    wgpu::ComputePipelineDescriptor desc;
    desc.compute.module = utils::CreateShaderModule(device, R"(
        @group(0) @binding(0) var<storage, read_write> buf: array<f32>;
        @compute @workgroup_size(64)
        fn main(@builtin(global_invocation_id) id: vec3u) {
            buf[id.x] = buf[id.x] * 2.0;
        }
    )");
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&desc);
    auto stats = pipeline.GetShaderStatistics();
    EXPECT_GE(stats.stageCount, 1u);
    EXPECT_EQ(stats.stages[0].stage, wgpu::ShaderStage::Compute);
}
```

- [ ] **Step 3: Write test — render pipeline reports per-stage stats**

```cpp
TEST_P(ShaderStatisticsTests, RenderPipelineReportsStats) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));
    // Create minimal render pipeline with VS + FS
    auto stats = pipeline.GetShaderStatistics();
    EXPECT_GE(stats.stageCount, 1u);
}
```

- [ ] **Step 4: Write test — ValidFields bitmask is consistent with data**

```cpp
TEST_P(ShaderStatisticsTests, ValidFieldsAreConsistent) {
    DAWN_TEST_UNSUPPORTED_IF(!device.HasFeature(wgpu::FeatureName::ShaderStatistics));
    // Create compute pipeline, get stats
    for (size_t i = 0; i < stats.stageCount; i++) {
        const auto& core = stats.stages[i].core;
        if (core.validFields & wgpu::ShaderStatisticValidField::RegisterCount) {
            EXPECT_GT(core.registerCount, 0u);
        }
        if (core.validFields & wgpu::ShaderStatisticValidField::SimdWidth) {
            EXPECT_GT(core.simdWidth, 0u);
        }
        if (core.validFields & wgpu::ShaderStatisticValidField::MaxThreadsPerGroup) {
            EXPECT_GT(core.maxThreadsPerGroup, 0u);
        }
    }
}
```

- [ ] **Step 5: Add DAWN_INSTANTIATE_TEST**

```cpp
DAWN_INSTANTIATE_TEST(ShaderStatisticsTests,
                      D3D12Backend(),
                      MetalBackend(),
                      VulkanBackend());
```

- [ ] **Step 6: Run tests**

Run: `out/Debug/dawn_end2end_tests --gtest_filter=ShaderStatistics*`
Expected: Pass on supported backends, skip on others.

- [ ] **Step 7: Commit**

```bash
git add src/dawn/tests/end2end/ShaderStatisticsTests.cpp \
        src/dawn/tests/end2end/BUILD.gn
git commit --amend --no-edit
```

---

## Task 9: Wire Tests

**Files:**
- Create: `src/dawn/tests/unittests/wire/WireShaderStatisticsTests.cpp`
- Modify: `src/dawn/tests/unittests/wire/BUILD.gn` — add `"WireShaderStatisticsTests.cpp"` to sources

- [ ] **Step 1: Add test file to BUILD.gn**

In `src/dawn/tests/unittests/wire/BUILD.gn`, add to sources:
```
"WireShaderStatisticsTests.cpp",
```

- [ ] **Step 2: Write wire round-trip test**

Verify that `ShaderStatistics` structs with variable-length arrays (stages, extension entries) and string views round-trip correctly through Dawn's wire serialization.

Reference existing wire tests in `src/dawn/tests/unittests/wire/` for the pattern: create a mock server-side object, set up expected calls, verify client receives correct data.

Test cases:
- Empty stats (stageCount = 0) round-trips correctly
- Stats with one stage and no extension entries
- Stats with extension entries containing string names

- [ ] **Step 3: Run wire tests**

Run: `out/Debug/dawn_unittests --gtest_filter=WireShaderStatistics*`
Expected: All pass.

- [ ] **Step 4: Commit**

```bash
git add src/dawn/tests/unittests/wire/WireShaderStatisticsTests.cpp \
        src/dawn/tests/unittests/wire/BUILD.gn
git commit --amend --no-edit
```

---

## Task 10: Presubmit and Final Verification

- [ ] **Step 1: Run clang-format**

Run: `git cl format`
Expected: Files are formatted.

- [ ] **Step 2: Run presubmit**

Run: `git cl presubmit`
Expected: All checks pass.

- [ ] **Step 3: Run full unit test suite**

Run: `out/Debug/dawn_unittests`
Expected: No regressions.

- [ ] **Step 4: Run full end-to-end test suite**

Run: `out/Debug/dawn_end2end_tests`
Expected: No regressions. ShaderStatistics tests pass on supported backends.

- [ ] **Step 5: Amend if format changed anything**

```bash
git commit --amend --no-edit
```
