#version 310 es


struct S_std140 {
  int before;
  vec2 m_col0;
  vec2 m_col1;
  vec2 m_col2;
  vec2 m_col3;
  int after;
};

layout(binding = 0, std140)
uniform tint_symbol_1_std140_1_ubo {
  S_std140 tint_symbol[4];
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v_1 = v.tint_symbol[2].m_col0;
  vec2 v_2 = v.tint_symbol[2].m_col1;
  vec2 v_3 = v.tint_symbol[2].m_col2;
  mat2x4 t = transpose(mat4x2(v_1, v_2, v_3, v.tint_symbol[2].m_col3));
  float l = length(v.tint_symbol[0].m_col1.yx);
  float a = abs(v.tint_symbol[0].m_col1.yx[0u]);
}
