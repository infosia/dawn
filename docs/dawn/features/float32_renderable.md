# Float32 Renderable

Available only in compatibility mode. Support of this feature is mandatory in core WebGPU.

When enabled, allows the `RENDER_ATTACHMENT` usage on textures with format `wgpu::TextureFormat::R32Float`, `wgpu::TextureFormat::RG32Float`, and `wgpu::TextureFormat::RGBA32Float`.

Without support, an error will occur at texture creation time as described in [WebGPU spec section 6.1.3.](https://gpuweb.github.io/gpuweb/#texture-creation)