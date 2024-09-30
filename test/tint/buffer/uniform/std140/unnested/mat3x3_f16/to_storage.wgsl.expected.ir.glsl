#version 310 es
#extension GL_AMD_gpu_shader_half_float: require

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  f16vec3 tint_symbol_col0;
  f16vec3 tint_symbol_col1;
  f16vec3 tint_symbol_col2;
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  f16mat3 tint_symbol_2;
} v_1;
void tint_store_and_preserve_padding(inout f16mat3 target, f16mat3 value_param) {
  target[0u] = value_param[0u];
  target[1u] = value_param[1u];
  target[2u] = value_param[2u];
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f16vec3 v_2 = v.tint_symbol_col0;
  f16vec3 v_3 = v.tint_symbol_col1;
  f16mat3 v_4 = f16mat3(v_2, v_3, v.tint_symbol_col2);
  tint_store_and_preserve_padding(v_1.tint_symbol_2, v_4);
  f16vec3 v_5 = v.tint_symbol_col0;
  f16vec3 v_6 = v.tint_symbol_col1;
  v_1.tint_symbol_2[1] = f16mat3(v_5, v_6, v.tint_symbol_col2)[0];
  f16vec3 v_7 = v.tint_symbol_col0;
  f16vec3 v_8 = v.tint_symbol_col1;
  v_1.tint_symbol_2[1] = f16mat3(v_7, v_8, v.tint_symbol_col2)[0].zxy;
  f16vec3 v_9 = v.tint_symbol_col0;
  f16vec3 v_10 = v.tint_symbol_col1;
  v_1.tint_symbol_2[0][1] = f16mat3(v_9, v_10, v.tint_symbol_col2)[1][0];
}
