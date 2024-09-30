#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  float tint_symbol;
} v;
float dpdy_7f8d84() {
  float res = dFdy(1.0f);
  return res;
}
void main() {
  float v_1 = dpdy_7f8d84();
  v.tint_symbol = v_1;
}
