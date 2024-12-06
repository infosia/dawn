#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  float inner;
} f_v;
float dpdx_e263de() {
  float res = dFdx(1.0f);
  return res;
}
void main() {
  f_v.inner = dpdx_e263de();
}
