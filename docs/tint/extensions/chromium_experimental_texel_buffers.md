# Chromium Experimental Texel Buffers

The `chromium_experimental_texel_buffers` extension allows to store WebGPU plain color format data with buffers, supporting format conversion on load and store operations.
Texel buffers roughly translate to buffer textures in Vulkan (`Dim = Buffer`), `texture_buffer<format>` in
Metal Shading Language, and typed UAV/SRV buffer views in HLSL/D3D12.

## Status

No official WebGPU specification has been written yet, and implementations must not expose this functionality without an explicit draft-enable flag.

## Syntax

A new `texel_buffer` type, parameterized by the color format and the access mode: `texel_buffer<Format, Access>`.

* `Format` must be a WebGPU plain color format (e.g. r8unorm, rgba32float, etc.).
* `Access` is either read or read_write.

Texel buffers  overload `textureDimensions`, `textureLoad` and `textureStore` built-in functions.

## Example usage

```wgsl
enable chromium_experimental_texel_buffers;

@group(X) @binding(Y) var myBuffer : texel_buffer<rgba8unorm, read_write>;

@compute @workgroup_size(32)
fn computeMain(@builtin(global_invocation_id) threadId : vec3<u32>) {
    let length = textureDimensions(myBuffer);
    if (threadId.x >= length) {
        return;
    }

    var value = textureLoad(myBuffer, threadId.x);
    value = value * value;
    textureStore(myBuffer, threadId.x, value);
}
```

