SKIP: FAILED

<dawn>/test/tint/builtins/gen/literal/subgroupMatrixLoad/14bbb7.wgsl:51:45 error: no matching call to 'subgroupMatrixLoad<subgroup_matrix_left<i8, 8, 8>>(ptr<storage, array<u32, 1024>, read>, u32, bool, u32)'

6 candidate functions:
 • 'subgroupMatrixLoad<T  ✗ >(ptr<workgroup' or 'storage, array<u32, AC>, read' or 'read_write>  ✓ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✗  'T' is 'subgroup_matrix<K, S, C, R>'
      ✗  'S' is 'u8'
 • 'subgroupMatrixLoad<T  ✓ >(ptr<storage, array<i32>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✓  'T' is 'subgroup_matrix<K, S, C, R>'
      ✓  'S' is 'i8'
 • 'subgroupMatrixLoad<T  ✓ >(ptr<workgroup' or 'storage, array<i32, AC>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✓  'T' is 'subgroup_matrix<K, S, C, R>'
      ✓  'S' is 'i8'
 • 'subgroupMatrixLoad<T  ✗ >(ptr<storage, array<S>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✗  'T' is 'subgroup_matrix<K, S, C, R>'
      ✗  'S' is 'f32', 'i32', 'u32' or 'f16'
 • 'subgroupMatrixLoad<T  ✗ >(ptr<workgroup' or 'storage, array<S, AC>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✗  'T' is 'subgroup_matrix<K, S, C, R>'
      ✗  'S' is 'f32', 'i32', 'u32' or 'f16'
 • 'subgroupMatrixLoad<T  ✗ >(ptr<storage, array<u32>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✗  'T' is 'subgroup_matrix<K, S, C, R>'
      ✗  'S' is 'u8'

  var res: subgroup_matrix_left<i8, 8, 8> = subgroupMatrixLoad<subgroup_matrix_left<i8, 8, 8>>(&sb_ro.arg_0, 1u, true, 8u);
                                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
