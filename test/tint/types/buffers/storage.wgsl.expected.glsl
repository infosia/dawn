#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer weights_block_1_ssbo {
  float inner[];
} f_v;
void main() {
  uint v_1 = (uint(f_v.inner.length()) - 1u);
  uint v_2 = min(uint(0), v_1);
  float a = f_v.inner[v_2];
}
