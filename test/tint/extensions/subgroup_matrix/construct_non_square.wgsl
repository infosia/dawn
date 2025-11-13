enable chromium_experimental_subgroup_matrix;

@group(0) @binding(0)
var<storage, read_write> ibuffer: array<f32>;
@group(0) @binding(1)
var<storage, read_write> ubuffer: array<f32>;

@compute @workgroup_size(64)
fn main() {
  // Zero init.
  subgroupMatrixStore(&ibuffer, 0, subgroup_matrix_left<f32, 4, 2>(), false, 64);
  subgroupMatrixStore(&ubuffer, 0, subgroup_matrix_right<f32, 2, 4>(), false, 64);

  // Non-zero init.
  subgroupMatrixStore(&ibuffer, 0, subgroup_matrix_left<f32, 4, 2>(-42), false, 64);
  subgroupMatrixStore(&ubuffer, 0, subgroup_matrix_right<f32, 2, 4>(42), false, 64);
}
