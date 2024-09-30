#version 310 es

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  vec3 tint_symbol_col0;
  vec3 tint_symbol_col1;
  vec3 tint_symbol_col2;
} v;
shared mat3 w;
void f_inner(uint tint_local_index) {
  if ((tint_local_index == 0u)) {
    w = mat3(vec3(0.0f), vec3(0.0f), vec3(0.0f));
  }
  barrier();
  vec3 v_1 = v.tint_symbol_col0;
  vec3 v_2 = v.tint_symbol_col1;
  w = mat3(v_1, v_2, v.tint_symbol_col2);
  vec3 v_3 = v.tint_symbol_col0;
  vec3 v_4 = v.tint_symbol_col1;
  w[1] = mat3(v_3, v_4, v.tint_symbol_col2)[0];
  vec3 v_5 = v.tint_symbol_col0;
  vec3 v_6 = v.tint_symbol_col1;
  w[1] = mat3(v_5, v_6, v.tint_symbol_col2)[0].zxy;
  vec3 v_7 = v.tint_symbol_col0;
  vec3 v_8 = v.tint_symbol_col1;
  w[0][1] = mat3(v_7, v_8, v.tint_symbol_col2)[1][0];
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f_inner(gl_LocalInvocationIndex);
}
