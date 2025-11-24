SKIP: INVALID

<dawn>/test/tint/extensions/dynamic_binding/resource_table/access_as_uniform.wgsl:7:36 error: no matching call to 'getBinding<texture_3d<f32>>(resource_table, u32)'

12 candidate functions:
 • 'getBinding<T  ✓ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✓  'T' is 'texture_3d<K>'
      ✓  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_depth_2d'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_depth_2d_array'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_depth_cube'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_depth_cube_array'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_depth_multisampled_2d'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_1d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_2d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_2d_array<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_cube<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_cube_array<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'getBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> T' where:
      ✗  'T' is 'texture_multisampled_2d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'

    let texture_load = textureLoad(getBinding<texture_3d<f32>>(sampled_textures, index), vec3(0, 0, 0), 0);
                                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
