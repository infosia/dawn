#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  float tint_symbol;
} v;
float dpdx_e263de() {
  float arg_0 = 1.0f;
  float res = dFdx(arg_0);
  return res;
}
void main() {
  float v_1 = dpdx_e263de();
  v.tint_symbol = v_1;
}
