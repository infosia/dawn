#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec4 tint_symbol;
} v;
vec4 dpdyFine_d0a648() {
  vec4 res = dFdy(vec4(1.0f));
  return res;
}
void main() {
  vec4 v_1 = dpdyFine_d0a648();
  v.tint_symbol = v_1;
}
