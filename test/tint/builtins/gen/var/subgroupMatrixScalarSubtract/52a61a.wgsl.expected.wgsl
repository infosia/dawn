enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<i32, 1024>;

fn subgroupMatrixScalarSubtract_52a61a() -> subgroup_matrix_right<i8, 8, 8> {
  var arg_0 = subgroup_matrix_right<i8, 8, 8>();
  var arg_1 = 8.0f;
  var res : subgroup_matrix_right<i8, 8, 8> = subgroupMatrixScalarSubtract(arg_0, arg_1);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarSubtract_52a61a(), false, 64);
}
