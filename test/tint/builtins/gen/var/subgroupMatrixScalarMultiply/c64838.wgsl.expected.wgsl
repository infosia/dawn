enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<i32, 1024>;

fn subgroupMatrixScalarMultiply_c64838() -> subgroup_matrix_left<i8, 8, 8> {
  var arg_0 = subgroup_matrix_left<i8, 8, 8>();
  var arg_1 = 8u;
  var res : subgroup_matrix_left<i8, 8, 8> = subgroupMatrixScalarMultiply(arg_0, arg_1);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarMultiply_c64838(), false, 64);
}
