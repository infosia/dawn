#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec3 tint_symbol_col0;
  vec3 tint_symbol_col1;
} v;
mat2x3 p = mat2x3(vec3(0.0f), vec3(0.0f));
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec3 v_1 = v.tint_symbol_col0;
  p = mat2x3(v_1, v.tint_symbol_col1);
  vec3 v_2 = v.tint_symbol_col0;
  p[1] = mat2x3(v_2, v.tint_symbol_col1)[0];
  vec3 v_3 = v.tint_symbol_col0;
  p[1] = mat2x3(v_3, v.tint_symbol_col1)[0].zxy;
  vec3 v_4 = v.tint_symbol_col0;
  p[0][1] = mat2x3(v_4, v.tint_symbol_col1)[1][0];
}
