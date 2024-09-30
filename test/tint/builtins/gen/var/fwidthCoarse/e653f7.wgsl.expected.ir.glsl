#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec2 tint_symbol;
} v;
vec2 fwidthCoarse_e653f7() {
  vec2 arg_0 = vec2(1.0f);
  vec2 res = fwidth(arg_0);
  return res;
}
void main() {
  vec2 v_1 = fwidthCoarse_e653f7();
  v.tint_symbol = v_1;
}
