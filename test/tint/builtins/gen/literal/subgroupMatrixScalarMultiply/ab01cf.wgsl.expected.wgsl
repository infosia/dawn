enable chromium_experimental_subgroup_matrix;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<u32, 1024>;

fn subgroupMatrixScalarMultiply_ab01cf() -> subgroup_matrix_left<u8, 8, 8> {
  var res : subgroup_matrix_left<u8, 8, 8> = subgroupMatrixScalarMultiply(subgroup_matrix_left<u8, 8, 8>(), 8.0h);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarMultiply_ab01cf(), false, 64);
}
