SKIP: FAILED

<dawn>/test/tint/builtins/gen/var/textureDimensions/210977.wgsl:44:18 error: no matching call to 'textureDimensions(texel_buffer<r32sint, read>)'

31 candidate functions:
 • 'textureDimensions(texture: texture_depth_2d  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_depth_2d_array  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_depth_cube  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_depth_cube_array  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_depth_multisampled_2d  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_external  ✗ ) -> vec2<u32>'
 • 'textureDimensions(texture: texture_1d<T>  ✗ ) -> u32' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_2d<T>  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_2d_array<T>  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_3d<T>  ✗ ) -> vec3<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_cube<T>  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_cube_array<T>  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_multisampled_2d<T>  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
 • 'textureDimensions(texture: texture_storage_1d<F, R>  ✗ ) -> u32' where:
      ✗  'R' is 'read'
 • 'textureDimensions(texture: texture_storage_2d<F, R>  ✗ ) -> vec2<u32>' where:
      ✗  'R' is 'read'
 • 'textureDimensions(texture: texture_storage_2d_array<F, R>  ✗ ) -> vec2<u32>' where:
      ✗  'R' is 'read'
 • 'textureDimensions(texture: texture_storage_3d<F, R>  ✗ ) -> vec3<u32>' where:
      ✗  'R' is 'read'
 • 'textureDimensions(texture: texture_storage_1d<F, W>  ✗ ) -> u32' where:
      ✗  'W' is 'write' or 'read_write'
 • 'textureDimensions(texture: texture_storage_2d<F, W>  ✗ ) -> vec2<u32>' where:
      ✗  'W' is 'write' or 'read_write'
 • 'textureDimensions(texture: texture_storage_2d_array<F, W>  ✗ ) -> vec2<u32>' where:
      ✗  'W' is 'write' or 'read_write'
 • 'textureDimensions(texture: texture_storage_3d<F, W>  ✗ ) -> vec3<u32>' where:
      ✗  'W' is 'write' or 'read_write'
 • 'textureDimensions(texture: texture_depth_2d  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_depth_2d_array  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_depth_cube  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_depth_cube_array  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_1d<T>  ✗ , level: L  ✗ ) -> u32' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_2d<T>  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_2d_array<T>  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_3d<T>  ✗ , level: L  ✗ ) -> vec3<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_cube<T>  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'
 • 'textureDimensions(texture: texture_cube_array<T>  ✗ , level: L  ✗ ) -> vec2<u32>' where:
      ✗  'T' is 'f32', 'i32' or 'u32'
      ✗  'L' is 'i32' or 'u32'

  var res: u32 = textureDimensions(arg_0);
                 ^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
