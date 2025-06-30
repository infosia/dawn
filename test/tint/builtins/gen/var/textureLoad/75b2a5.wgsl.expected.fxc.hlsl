SKIP: FAILED

<dawn>/test/tint/builtins/gen/var/textureLoad/75b2a5.wgsl:45:24 error: no matching call to 'textureLoad(texel_buffer<rgb10a2uint, read_write>, i32)'

33 candidate functions:
 • 'textureLoad(texture: texture_storage_1d<F, R>  ✗ , coords: C  ✓ ) -> vec4<f32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_1d<F, R>  ✗ , coords: C  ✓ ) -> vec4<i32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_1d<F, R>  ✗ , coords: C  ✓ ) -> vec4<u32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_1d<F, RW>  ✗ , coords: C  ✓ ) -> vec4<f32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_1d<F, RW>  ✗ , coords: C  ✓ ) -> vec4<i32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_1d<F, RW>  ✗ , coords: C  ✓ ) -> vec4<u32>' where:
      ✓  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_external  ✗ , coords: vec2<C>  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_1d<T>  ✗ , coords: C  ✓ , level: L  ✗ ) -> vec4<T>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✓  'C' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d<F, R>  ✗ , coords: vec2<C>  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_2d<F, R>  ✗ , coords: vec2<C>  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_2d<F, R>  ✗ , coords: vec2<C>  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_3d<F, R>  ✗ , coords: vec3<C>  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_3d<F, R>  ✗ , coords: vec3<C>  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_3d<F, R>  ✗ , coords: vec3<C>  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'R' is 'read'
 • 'textureLoad(texture: texture_storage_2d<F, RW>  ✗ , coords: vec2<C>  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_2d<F, RW>  ✗ , coords: vec2<C>  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_2d<F, RW>  ✗ , coords: vec2<C>  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_3d<F, RW>  ✗ , coords: vec3<C>  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_3d<F, RW>  ✗ , coords: vec3<C>  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_storage_3d<F, RW>  ✗ , coords: vec3<C>  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'RW' is 'read_write'
 • 'textureLoad(texture: texture_depth_2d  ✗ , coords: vec2<C>  ✗ , level: L  ✗ ) -> f32' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_depth_multisampled_2d  ✗ , coords: vec2<C>  ✗ , sample_index: S  ✗ ) -> f32' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'S' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_2d<T>  ✗ , coords: vec2<C>  ✗ , level: L  ✗ ) -> vec4<T>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'C' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_3d<T>  ✗ , coords: vec3<C>  ✗ , level: L  ✗ ) -> vec4<T>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'C' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_multisampled_2d<T>  ✗ , coords: vec2<C>  ✗ , sample_index: S  ✗ ) -> vec4<T>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'C' is 'i32' or 'u32'
      ✗  'S' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, R>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'R' is 'read'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, R>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'R' is 'read'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, R>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'R' is 'read'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, RW>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<f32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8unorm', 'r8snorm', 'rg8unorm', 'rg8snorm', 'bgra8unorm', 'rgba8unorm', 'rgba8snorm', 'r16float', 'rg16float', 'rgba16float', 'r32float', 'rg32float', 'rgba32float', 'rgb10a2unorm' or 'rg11b10ufloat'
      ✗  'RW' is 'read_write'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, RW>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<i32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8sint', 'rg8sint', 'rgba8sint', 'r16sint', 'rg16sint', 'rgba16sint', 'r32sint', 'rg32sint' or 'rgba32sint'
      ✗  'RW' is 'read_write'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_storage_2d_array<F, RW>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ ) -> vec4<u32>' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'F' is 'r8uint', 'rg8uint', 'r16uint', 'rg16uint', 'rgba8uint', 'rgba16uint', 'r32uint', 'rg32uint', 'rgba32uint' or 'rgb10a2uint'
      ✗  'RW' is 'read_write'
      ✗  'A' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_depth_2d_array  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ , level: L  ✗ ) -> f32' where:
      ✗  'C' is 'i32' or 'u32'
      ✗  'A' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureLoad(texture: texture_2d_array<T>  ✗ , coords: vec2<C>  ✗ , array_index: A  ✗ , level: L  ✗ ) -> vec4<T>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'C' is 'i32' or 'u32'
      ✗  'A' is 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'

  var res: vec4<u32> = textureLoad(arg_0, arg_1);
                       ^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
