#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec2 tint_symbol_col0;
  vec2 tint_symbol_col1;
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_1 = v.tint_symbol_col0;
  mat2 t = transpose(mat2(v_1, v.tint_symbol_col1));
  vec2 v_2 = v.tint_symbol_col0;
  float l = length(mat2(v_2, v.tint_symbol_col1)[1]);
  vec2 v_3 = v.tint_symbol_col0;
  float a = abs(mat2(v_3, v.tint_symbol_col1)[0].yx[0u]);
}
