#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec3 tint_symbol_col0;
  vec3 tint_symbol_col1;
  vec3 tint_symbol_col2;
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec3 v_1 = v.tint_symbol_col0;
  vec3 v_2 = v.tint_symbol_col1;
  mat3 t = transpose(mat3(v_1, v_2, v.tint_symbol_col2));
  vec3 v_3 = v.tint_symbol_col0;
  vec3 v_4 = v.tint_symbol_col1;
  float l = length(mat3(v_3, v_4, v.tint_symbol_col2)[1]);
  vec3 v_5 = v.tint_symbol_col0;
  vec3 v_6 = v.tint_symbol_col1;
  float a = abs(mat3(v_5, v_6, v.tint_symbol_col2)[0].zxy[0u]);
}
