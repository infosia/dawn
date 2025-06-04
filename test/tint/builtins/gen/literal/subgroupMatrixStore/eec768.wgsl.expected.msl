SKIP: FAILED

<dawn>/test/tint/builtins/gen/literal/subgroupMatrixStore/eec768.wgsl:49:3 error: no matching call to 'subgroupMatrixStore(ptr<storage, array<u32>, read_write>, u32, subgroup_matrix_right<i8, 8, 8>, bool, u32)'

6 candidate functions:
 • 'subgroupMatrixStore(ptr<storage, array<u32>, write' or 'read_write>  ✓ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✗ , bool  ✓ , u32  ✓ )' where:
      ✗  'S' is 'u8'
 • 'subgroupMatrixStore(ptr<storage, array<S>, write' or 'read_write>  ✓ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✗ , bool  ✓ , u32  ✓ )' where:
      ✓  'S' is 'f32', 'i32', 'u32' or 'f16'
 • 'subgroupMatrixStore(ptr<storage, array<i32>, write' or 'read_write>  ✗ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✓ , bool  ✓ , u32  ✓ )' where:
      ✓  'S' is 'i8'
 • 'subgroupMatrixStore(ptr<workgroup' or 'storage, array<i32, AC>, write' or 'read_write>  ✗ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✓ , bool  ✓ , u32  ✓ )' where:
      ✓  'S' is 'i8'
 • 'subgroupMatrixStore(ptr<workgroup' or 'storage, array<S, AC>, write' or 'read_write>  ✗ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✗ , bool  ✓ , u32  ✓ )' where:
      ✗  'S' is 'f32', 'i32', 'u32' or 'f16'
 • 'subgroupMatrixStore(ptr<workgroup' or 'storage, array<u32, AC>, write' or 'read_write>  ✗ , u32  ✓ , subgroup_matrix<K, S, C, R>  ✗ , bool  ✓ , u32  ✓ )' where:
      ✗  'S' is 'u8'

  subgroupMatrixStore(&sb_rw.arg_0, 1u, subgroup_matrix_right<i8, 8, 8>(), true, 8u);
  ^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
