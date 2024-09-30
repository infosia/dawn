#version 310 es

layout(binding = 0, std430)
buffer tint_symbol_3_1_ssbo {
  ivec4 tint_symbol_2;
} v;
void deref() {
  ivec4 v_1 = v.tint_symbol_2.wzyx;
  v.tint_symbol_2 = v_1;
}
void no_deref() {
  ivec4 v_2 = v.tint_symbol_2.wzyx;
  v.tint_symbol_2 = v_2;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  deref();
  no_deref();
}
