#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  vec2 inner;
} f_v;
vec2 dpdxCoarse_9581cf() {
  vec2 arg_0 = vec2(1.0f);
  vec2 res = dFdx(arg_0);
  return res;
}
void main() {
  f_v.inner = dpdxCoarse_9581cf();
}
