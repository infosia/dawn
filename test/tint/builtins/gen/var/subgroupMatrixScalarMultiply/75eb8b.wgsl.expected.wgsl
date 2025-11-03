enable chromium_experimental_subgroup_matrix;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : array<u32, 1024>;

fn subgroupMatrixScalarMultiply_75eb8b() -> subgroup_matrix_result<u8, 8, 8> {
  var arg_0 = subgroup_matrix_result<u8, 8, 8>();
  var arg_1 = 8.0h;
  var res : subgroup_matrix_result<u8, 8, 8> = subgroupMatrixScalarMultiply(arg_0, arg_1);
  return res;
}

@compute @workgroup_size(1)
fn compute_main() {
  subgroupMatrixStore(&(prevent_dce), 0, subgroupMatrixScalarMultiply_75eb8b(), false, 64);
}
