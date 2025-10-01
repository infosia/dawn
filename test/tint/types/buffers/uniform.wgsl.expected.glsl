#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std140)
uniform f_weights_block_ubo {
  uvec4 inner[1];
} v;
void main() {
  float a = uintBitsToFloat(v.inner[0u].x);
}
