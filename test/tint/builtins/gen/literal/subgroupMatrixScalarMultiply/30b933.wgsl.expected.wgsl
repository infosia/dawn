enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<u32, 1024>;

fn subgroupMatrixScalarMultiply_30b933() -> subgroup_matrix_right<u8, 8, 8> {
  var res : subgroup_matrix_right<u8, 8, 8> = subgroupMatrixScalarMultiply(subgroup_matrix_right<u8, 8, 8>(), 8i);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarMultiply_30b933(), false, 64);
}
