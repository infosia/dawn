# Chromium Experimental Subgroup Size Control

The `chromium_experimental_subgroup_size_control` is an experimental extension that allows the use of the
`subgroup_size` attribute in compute shaders in WGSL. When the attribute `subgroup_size` is declared, the compute shader will only be executed with the specified subgroup size, and the built-in value `subgroup_size` will be the the value of the attribute `subgroup_size`. Otherwise, the compute shader may be executed with a subgroup size chosen by the GPU driver, which is unknown before the execution of the compute shader.

# Status

This extension is experimental and the syntax is being discussed. No official WebGPU specification
has been written yet.

The overal status can be tracked at [463721943](https://issues.chromium.org/issues/463721943).

# Availability

The usage is restricted to `compute` shaders.

|Backend|Implementation|Requirements|
|---|---|---|
|D3D12|`[WaveSize(<numLanes>)]` in HLSL|Shader Model 6.6|
|Vulkan|`requiredSubgroupSize` in `VkPipelineShaderStageRequiredSubgroupSizeCreateInfo` on the API side|The `subgroupSizeControl` feature in `VK_EXT_subgroup_size_control` or Vulkan 1.3|
|Metal|Not Supported|Not Supported|

- About [71.5%](https://vulkan.gpuinfo.org/listfeaturesextensions.php?extension=VK_EXT_subgroup_size_control) devices support `subgroupSizeControl` according to the report on GPUInfo.org.
- This extension cannot be supported on Metal.
- Due to the limited availability, this will need a `enable` statement to be used. For this
experimental extension it would be `enable chromium_experimental_subgroup_size_control`.

# Specification

This extension adds a new `subgroup_size` attribute for `compute` shaders.

This extension must be used together with the extension `subgroups`.

```
subgroup_size_attr :
 '@' 'subgroup_size' '(' expression ',' ? ')'
```
- The parameter must be a const-expression that resolves to an `i32` or `u32`.
- The parameter must be resolved to one of the below integers which is required by [HLSL Spec](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_WaveSize.html#allowed-wave-sizes):
`[4, 8, 16, 32, 64, 128]`.
- The parameter must be greater than or equal to the `subgroupMinSize` on the current `adapter`.
- The parameter must be less than or equal to the `subgroupMaxSize` on the current `adapter`. We need to fix the D3D12 backend to report correct `subgroupMaxSize` on current `adapter`.

# Example usage

```wgsl
enable subgroups;
enable chromium_experimental_subgroup_size_control;

@subgroup_size(32)
@compute @workgroup_size(64, 1, 1)
fn main(@builtin(subgroup_invocation_id) sg_id : u32,
        @builtin(subgroup_size) sg_size : u32) {

}
```

# References

* [DirectX Specs HLSL Wave Size](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_WaveSize.html#allowed-wave-sizes)
* [Vulkan VK_EXT_subgroup_size_control](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_subgroup_size_control.html)