#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec3 tint_symbol;
} v;
vec3 dpdyCoarse_ae1873() {
  vec3 arg_0 = vec3(1.0f);
  vec3 res = dFdy(arg_0);
  return res;
}
void main() {
  vec3 v_1 = dpdyCoarse_ae1873();
  v.tint_symbol = v_1;
}
