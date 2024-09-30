#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  uint tint_symbol;
} v;
layout(binding = 1, std430)
buffer SB_RW_1_ssbo {
  uint arg_0[];
} sb_rw;
uint arrayLength_eb510f() {
  uint res = uint(sb_rw.arg_0.length());
  return res;
}
void main() {
  uint v_1 = arrayLength_eb510f();
  v.tint_symbol = v_1;
}
#version 310 es

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  uint tint_symbol;
} v;
layout(binding = 1, std430)
buffer SB_RW_1_ssbo {
  uint arg_0[];
} sb_rw;
uint arrayLength_eb510f() {
  uint res = uint(sb_rw.arg_0.length());
  return res;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint v_1 = arrayLength_eb510f();
  v.tint_symbol = v_1;
}
