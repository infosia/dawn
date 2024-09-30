#version 310 es
#extension GL_AMD_gpu_shader_half_float: require

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  f16vec4 tint_symbol_col0;
  f16vec4 tint_symbol_col1;
  f16vec4 tint_symbol_col2;
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  f16mat3x4 tint_symbol_2;
} v_1;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f16vec4 v_2 = v.tint_symbol_col0;
  f16vec4 v_3 = v.tint_symbol_col1;
  f16mat3x4 v_4 = f16mat3x4(v_2, v_3, v.tint_symbol_col2);
  v_1.tint_symbol_2 = v_4;
  f16vec4 v_5 = v.tint_symbol_col0;
  f16vec4 v_6 = v.tint_symbol_col1;
  v_1.tint_symbol_2[1] = f16mat3x4(v_5, v_6, v.tint_symbol_col2)[0];
  f16vec4 v_7 = v.tint_symbol_col0;
  f16vec4 v_8 = v.tint_symbol_col1;
  v_1.tint_symbol_2[1] = f16mat3x4(v_7, v_8, v.tint_symbol_col2)[0].ywxz;
  f16vec4 v_9 = v.tint_symbol_col0;
  f16vec4 v_10 = v.tint_symbol_col1;
  v_1.tint_symbol_2[0][1] = f16mat3x4(v_9, v_10, v.tint_symbol_col2)[1][0];
}
