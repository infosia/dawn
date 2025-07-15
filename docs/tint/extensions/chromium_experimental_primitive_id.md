# Chromium Experimental Primitive ID

The `chromium_experimental_primitive_id` is an experimental extension that allows the use of the
`primitive_id` builtin in fragment shaders in WGSL.

# Status

This extension is experimental and the syntax is being discussed. No official WebGPU specification
has been written yet.

# Availability

The usage is restricted to `fragment` shaders.

| SPIR-V | 32-bit integer | Requires geometry shaders, mesh shaders, or raytracing. Available as the `PrimitiveID` builtin |
| HLSL | u32 | Requires D3D10. Available as the `SV_PrimitiveID` semantic |
| GLSL | i32 | Requires GLSL 1.50 and later. Available as the `gl_primitiveID` builtin |
| Metal | u32 | Requires Metal 2.2 on MacOS or Metal 2.3 on iOS. Available as `[[primitive_id]]` |

Due to the limited availability, this will need a `require` statement to be used. For this
experimental extension it would be `require chromium_experimental_primitive_id`.

# Specification

This extension adds a new `builtin_value_name` entry for `primitive_id`.

An entry is added to the _Built-in input and output values_ table:

| Name | `primitive_id` |
| Stage | `fragment` |
| Direction | `input` |
| Type | u32 |
| Extension | `chromium_experimental_primitive_id` |

* The index of the first primitive is zero, incrementing by one for each subsequent primitive.
* `primitive_id` resets to zero between each instance drawn.
* If the instance id overflows (exceeds 2^32 – 1), it wraps to 0. (This is from HLSL, is it true for
  other backends?)
* There is no support for automatically generating a primitive id for adjacent primitives.
  * For an adjacent primitive, the id is only maintained for the internal non-adjacent primitives.

# Example usage

```wgsl
@vertex fn vs_main(@builtin(primitive_id) my_id: u32) -> @builtin(position) vec4f {
    return vec4f(f32(my_id));
}
```

# References
* [GLSL gl_PrimitiveID](https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_PrimitiveID.xhtml)
* [HLSL PrimitiveId](https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-input-assembler-stage-using#primitiveid)
* [HLSL SV_PrimitiveId](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics)
* [Metal p.119](https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf)
* [Vulkan PrimitiveId](https://registry.khronos.org/vulkan/specs/latest/man/html/PrimitiveId.html)
