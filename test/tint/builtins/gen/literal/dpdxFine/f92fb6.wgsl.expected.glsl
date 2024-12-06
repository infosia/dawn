#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  vec3 inner;
} f_v;
vec3 dpdxFine_f92fb6() {
  vec3 res = dFdx(vec3(1.0f));
  return res;
}
void main() {
  f_v.inner = dpdxFine_f92fb6();
}
