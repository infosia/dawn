#version 310 es
#extension GL_AMD_gpu_shader_half_float: require

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  f16vec3 tint_symbol_col0;
  f16vec3 tint_symbol_col1;
  f16vec3 tint_symbol_col2;
} v;
shared f16mat3 w;
void f_inner(uint tint_local_index) {
  if ((tint_local_index == 0u)) {
    w = f16mat3(f16vec3(0.0hf), f16vec3(0.0hf), f16vec3(0.0hf));
  }
  barrier();
  f16vec3 v_1 = v.tint_symbol_col0;
  f16vec3 v_2 = v.tint_symbol_col1;
  w = f16mat3(v_1, v_2, v.tint_symbol_col2);
  f16vec3 v_3 = v.tint_symbol_col0;
  f16vec3 v_4 = v.tint_symbol_col1;
  w[1] = f16mat3(v_3, v_4, v.tint_symbol_col2)[0];
  f16vec3 v_5 = v.tint_symbol_col0;
  f16vec3 v_6 = v.tint_symbol_col1;
  w[1] = f16mat3(v_5, v_6, v.tint_symbol_col2)[0].zxy;
  f16vec3 v_7 = v.tint_symbol_col0;
  f16vec3 v_8 = v.tint_symbol_col1;
  w[0][1] = f16mat3(v_7, v_8, v.tint_symbol_col2)[1][0];
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f_inner(gl_LocalInvocationIndex);
}
