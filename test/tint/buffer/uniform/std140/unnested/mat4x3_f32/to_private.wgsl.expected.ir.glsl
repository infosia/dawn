#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec3 tint_symbol_col0;
  vec3 tint_symbol_col1;
  vec3 tint_symbol_col2;
  vec3 tint_symbol_col3;
} v;
mat4x3 p = mat4x3(vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f));
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec3 v_1 = v.tint_symbol_col0;
  vec3 v_2 = v.tint_symbol_col1;
  vec3 v_3 = v.tint_symbol_col2;
  p = mat4x3(v_1, v_2, v_3, v.tint_symbol_col3);
  vec3 v_4 = v.tint_symbol_col0;
  vec3 v_5 = v.tint_symbol_col1;
  vec3 v_6 = v.tint_symbol_col2;
  p[1] = mat4x3(v_4, v_5, v_6, v.tint_symbol_col3)[0];
  vec3 v_7 = v.tint_symbol_col0;
  vec3 v_8 = v.tint_symbol_col1;
  vec3 v_9 = v.tint_symbol_col2;
  p[1] = mat4x3(v_7, v_8, v_9, v.tint_symbol_col3)[0].zxy;
  vec3 v_10 = v.tint_symbol_col0;
  vec3 v_11 = v.tint_symbol_col1;
  vec3 v_12 = v.tint_symbol_col2;
  p[0][1] = mat4x3(v_10, v_11, v_12, v.tint_symbol_col3)[1][0];
}
