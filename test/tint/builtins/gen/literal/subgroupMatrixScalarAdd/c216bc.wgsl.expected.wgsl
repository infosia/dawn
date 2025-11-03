enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<u32, 1024>;

fn subgroupMatrixScalarAdd_c216bc() -> subgroup_matrix_right<u8, 8, 8> {
  var res : subgroup_matrix_right<u8, 8, 8> = subgroupMatrixScalarAdd(subgroup_matrix_right<u8, 8, 8>(), 8.0f);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarAdd_c216bc(), false, 64);
}
