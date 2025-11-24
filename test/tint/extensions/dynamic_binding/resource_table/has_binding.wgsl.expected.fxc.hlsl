SKIP: FAILED

<dawn>/test/tint/extensions/dynamic_binding/resource_table/has_binding.wgsl:6:13 error: no matching call to 'hasBinding<texture_2d<i32>>(resource_table, abstract-int)'

12 candidate functions:
 • 'hasBinding<T  ✓ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✓  'T' is 'texture_2d<K>'
      ✓  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_depth_2d'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_depth_2d_array'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_depth_cube'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_depth_cube_array'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_depth_multisampled_2d'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_1d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_2d_array<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_3d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_cube<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_cube_array<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'
 • 'hasBinding<T  ✗ >(resource_binding  ✗ , index: I  ✓ ) -> bool' where:
      ✗  'T' is 'texture_multisampled_2d<K>'
      ✗  'K' is 'f32', 'i32' or 'u32'
      ✓  'I' is 'i32' or 'u32'

    let t = hasBinding<texture_2d<i32>>(sampled_textures, 4);
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
