#version 310 es
#extension GL_AMD_gpu_shader_half_float: require

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  f16vec3 tint_symbol_col0;
  f16vec3 tint_symbol_col1;
  f16vec3 tint_symbol_col2;
  f16vec3 tint_symbol_col3;
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  f16mat4x3 tint_symbol_2;
} v_1;
void tint_store_and_preserve_padding(inout f16mat4x3 target, f16mat4x3 value_param) {
  target[0u] = value_param[0u];
  target[1u] = value_param[1u];
  target[2u] = value_param[2u];
  target[3u] = value_param[3u];
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f16vec3 v_2 = v.tint_symbol_col0;
  f16vec3 v_3 = v.tint_symbol_col1;
  f16vec3 v_4 = v.tint_symbol_col2;
  f16mat4x3 v_5 = f16mat4x3(v_2, v_3, v_4, v.tint_symbol_col3);
  tint_store_and_preserve_padding(v_1.tint_symbol_2, v_5);
  f16vec3 v_6 = v.tint_symbol_col0;
  f16vec3 v_7 = v.tint_symbol_col1;
  f16vec3 v_8 = v.tint_symbol_col2;
  v_1.tint_symbol_2[1] = f16mat4x3(v_6, v_7, v_8, v.tint_symbol_col3)[0];
  f16vec3 v_9 = v.tint_symbol_col0;
  f16vec3 v_10 = v.tint_symbol_col1;
  f16vec3 v_11 = v.tint_symbol_col2;
  v_1.tint_symbol_2[1] = f16mat4x3(v_9, v_10, v_11, v.tint_symbol_col3)[0].zxy;
  f16vec3 v_12 = v.tint_symbol_col0;
  f16vec3 v_13 = v.tint_symbol_col1;
  f16vec3 v_14 = v.tint_symbol_col2;
  v_1.tint_symbol_2[0][1] = f16mat4x3(v_12, v_13, v_14, v.tint_symbol_col3)[1][0];
}
