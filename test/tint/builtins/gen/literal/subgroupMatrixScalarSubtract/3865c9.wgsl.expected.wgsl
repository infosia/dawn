enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<i32, 1024>;

fn subgroupMatrixScalarSubtract_3865c9() -> subgroup_matrix_right<i8, 8, 8> {
  var res : subgroup_matrix_right<i8, 8, 8> = subgroupMatrixScalarSubtract(subgroup_matrix_right<i8, 8, 8>(), 8u);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarSubtract_3865c9(), false, 64);
}
