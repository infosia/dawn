enable chromium_experimental_subgroup_matrix;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<u32, 1024>;

fn subgroupMatrixScalarSubtract_a6a10f() -> subgroup_matrix_right<u8, 8, 8> {
  var res : subgroup_matrix_right<u8, 8, 8> = subgroupMatrixScalarSubtract(subgroup_matrix_right<u8, 8, 8>(), 8.0h);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarSubtract_a6a10f(), false, 64);
}
