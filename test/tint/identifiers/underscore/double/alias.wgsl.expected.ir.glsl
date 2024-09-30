#version 310 es

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  int tint_symbol;
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  int c = 0;
  int d = 0;
  int v_1 = (c + d);
  v.tint_symbol = v_1;
}
