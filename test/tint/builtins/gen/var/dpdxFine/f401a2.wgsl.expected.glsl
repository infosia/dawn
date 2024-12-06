#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  float inner;
} f_v;
float dpdxFine_f401a2() {
  float arg_0 = 1.0f;
  float res = dFdx(arg_0);
  return res;
}
void main() {
  f_v.inner = dpdxFine_f401a2();
}
