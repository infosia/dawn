#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec3 tint_symbol;
} v;
vec3 dpdx_0763f7() {
  vec3 res = dFdx(vec3(1.0f));
  return res;
}
void main() {
  vec3 v_1 = dpdx_0763f7();
  v.tint_symbol = v_1;
}
