SKIP: FAILED

<dawn>/test/tint/builtins/gen/var/subgroupMatrixLoad/1e5747.wgsl:54:46 error: no matching call to 'subgroupMatrixLoad<subgroup_matrix_right<i8, 8, 8>>(ptr<storage, array<u32>, read_write>, u32, bool, u32)'

6 candidate functions:
 • 'subgroupMatrixLoad<T  ✗ >(ptr<storage, array<u32>, read' or 'read_write>  ✓ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
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
 • 'subgroupMatrixLoad<T  ✗ >(ptr<workgroup' or 'storage, array<u32, AC>, read' or 'read_write>  ✗ , u32  ✓ , bool  ✓ , u32  ✓ ) -> T' where:
      ✗  'T' is 'subgroup_matrix<K, S, C, R>'
      ✗  'S' is 'u8'

  var res: subgroup_matrix_right<i8, 8, 8> = subgroupMatrixLoad<subgroup_matrix_right<i8, 8, 8>>(&sb_rw.arg_0, arg_1, arg_2, arg_3);
                                             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
