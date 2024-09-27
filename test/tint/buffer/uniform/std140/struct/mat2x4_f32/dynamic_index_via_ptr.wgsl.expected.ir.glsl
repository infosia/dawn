#version 310 es


struct Inner {
  mat2x4 m;
};

struct Outer {
  Inner a[4];
};

layout(binding = 0, std140)
uniform tint_symbol_1_1_ubo {
  Outer tint_symbol[4];
} v;
int counter = 0;
int i() {
  counter = (counter + 1);
  return counter;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  Outer l_a[4] = v.tint_symbol;
  Outer l_a_i = v.tint_symbol[i()];
  Inner l_a_i_a[4] = v.tint_symbol[i()].a;
  Inner l_a_i_a_i = v.tint_symbol[i()].a[i()];
  mat2x4 l_a_i_a_i_m = v.tint_symbol[i()].a[i()].m;
  vec4 l_a_i_a_i_m_i = v.tint_symbol[i()].a[i()].m[i()];
  float l_a_i_a_i_m_i_i = v.tint_symbol[i()].a[i()].m[i()][i()];
}
