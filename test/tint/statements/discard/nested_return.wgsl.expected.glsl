#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer non_uniform_global_block_1_ssbo {
  int inner;
} f_v;
bool continue_execution = true;
void main() {
  if ((f_v.inner < 0)) {
    continue_execution = false;
  }
  if (!(continue_execution)) {
    discard;
  }
}
