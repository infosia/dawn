#version 310 es
#extension GL_AMD_gpu_shader_half_float: require
precision highp float;
precision highp int;


struct S_std140 {
  f16vec3 matrix_col0;
  f16vec3 matrix_col1;
  f16vec3 matrix_col2;
  f16vec3 vector;
};

layout(binding = 0, std140)
uniform data_block_std140_1_ubo {
  S_std140 inner;
} f_v;
void main() {
  f16vec3 v_1 = f_v.inner.vector;
  f16vec3 x = (v_1 * f16mat3(f_v.inner.matrix_col0, f_v.inner.matrix_col1, f_v.inner.matrix_col2));
}
