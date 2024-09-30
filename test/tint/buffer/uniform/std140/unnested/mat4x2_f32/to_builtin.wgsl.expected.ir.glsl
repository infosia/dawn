#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec2 tint_symbol_col0;
  vec2 tint_symbol_col1;
  vec2 tint_symbol_col2;
  vec2 tint_symbol_col3;
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_1 = v.tint_symbol_col0;
  vec2 v_2 = v.tint_symbol_col1;
  vec2 v_3 = v.tint_symbol_col2;
  mat2x4 t = transpose(mat4x2(v_1, v_2, v_3, v.tint_symbol_col3));
  vec2 v_4 = v.tint_symbol_col0;
  vec2 v_5 = v.tint_symbol_col1;
  vec2 v_6 = v.tint_symbol_col2;
  float l = length(mat4x2(v_4, v_5, v_6, v.tint_symbol_col3)[1]);
  vec2 v_7 = v.tint_symbol_col0;
  vec2 v_8 = v.tint_symbol_col1;
  vec2 v_9 = v.tint_symbol_col2;
  float a = abs(mat4x2(v_7, v_8, v_9, v.tint_symbol_col3)[0].yx[0u]);
}
