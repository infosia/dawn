#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec2 tint_symbol_col0;
  vec2 tint_symbol_col1;
} v;
int counter = 0;
int i() {
  counter = (counter + 1);
  return counter;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_1 = v.tint_symbol_col0;
  mat2 v_2 = mat2(v_1, v.tint_symbol_col1);
  mat2 l_m = v_2;
  vec2 l_m_i = v_2[i()];
}
