#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec2 tint_symbol_col0;
  vec2 tint_symbol_col1;
  vec2 tint_symbol_col2;
  vec2 tint_symbol_col3;
} v_1;
void a(mat4x2 m) {
}
void b(vec2 v) {
}
void c(float f) {
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_2 = v_1.tint_symbol_col0;
  vec2 v_3 = v_1.tint_symbol_col1;
  vec2 v_4 = v_1.tint_symbol_col2;
  a(mat4x2(v_2, v_3, v_4, v_1.tint_symbol_col3));
  vec2 v_5 = v_1.tint_symbol_col0;
  vec2 v_6 = v_1.tint_symbol_col1;
  vec2 v_7 = v_1.tint_symbol_col2;
  b(mat4x2(v_5, v_6, v_7, v_1.tint_symbol_col3)[1]);
  vec2 v_8 = v_1.tint_symbol_col0;
  vec2 v_9 = v_1.tint_symbol_col1;
  vec2 v_10 = v_1.tint_symbol_col2;
  b(mat4x2(v_8, v_9, v_10, v_1.tint_symbol_col3)[1].yx);
  vec2 v_11 = v_1.tint_symbol_col0;
  vec2 v_12 = v_1.tint_symbol_col1;
  vec2 v_13 = v_1.tint_symbol_col2;
  c(mat4x2(v_11, v_12, v_13, v_1.tint_symbol_col3)[1][0u]);
  vec2 v_14 = v_1.tint_symbol_col0;
  vec2 v_15 = v_1.tint_symbol_col1;
  vec2 v_16 = v_1.tint_symbol_col2;
  c(mat4x2(v_14, v_15, v_16, v_1.tint_symbol_col3)[1].yx[0u]);
}
