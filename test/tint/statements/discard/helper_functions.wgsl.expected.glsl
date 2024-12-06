#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer non_uniform_global_block_1_ssbo {
  int inner;
} f_v;
layout(binding = 1, std430)
buffer tint_symbol_block_1_ssbo {
  float inner;
} f_v_1;
bool continue_execution = true;
void foo() {
  if ((f_v.inner < 0)) {
    continue_execution = false;
  }
}
void bar() {
  float v_2 = dFdx(1.0f);
  if (continue_execution) {
    f_v_1.inner = v_2;
  }
}
void main() {
  foo();
  bar();
  if (!(continue_execution)) {
    discard;
  }
}
