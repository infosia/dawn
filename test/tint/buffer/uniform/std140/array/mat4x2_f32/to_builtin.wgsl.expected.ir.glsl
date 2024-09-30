#version 310 es


struct mat4x2_f32_std140 {
  vec2 col0;
  vec2 col1;
  vec2 col2;
  vec2 col3;
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  mat4x2_f32_std140 tint_symbol[4];
} v;
layout(binding = 1, std430)
buffer tint_symbol_3_1_ssbo {
  float tint_symbol_2;
} v_1;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_2 = v.tint_symbol[2].col0;
  vec2 v_3 = v.tint_symbol[2].col1;
  vec2 v_4 = v.tint_symbol[2].col2;
  mat2x4 t = transpose(mat4x2(v_2, v_3, v_4, v.tint_symbol[2].col3));
  float l = length(v.tint_symbol[0].col1.yx);
  float a = abs(v.tint_symbol[0].col1.yx[0u]);
  float v_5 = (t[0][0u] + float(l));
  float v_6 = (v_5 + float(a));
  v_1.tint_symbol_2 = v_6;
}
