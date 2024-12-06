#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer prevent_dce_block_1_ssbo {
  float inner;
} f_v;
float dpdyCoarse_870a7e() {
  float res = dFdy(1.0f);
  return res;
}
void main() {
  f_v.inner = dpdyCoarse_870a7e();
}
