#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec2 tint_symbol;
} v;
vec2 dpdyCoarse_3e1ab4() {
  vec2 res = dFdy(vec2(1.0f));
  return res;
}
void main() {
  vec2 v_1 = dpdyCoarse_3e1ab4();
  v.tint_symbol = v_1;
}
