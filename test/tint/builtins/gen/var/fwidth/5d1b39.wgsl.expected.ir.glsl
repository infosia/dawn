#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec3 tint_symbol;
} v;
vec3 fwidth_5d1b39() {
  vec3 arg_0 = vec3(1.0f);
  vec3 res = fwidth(arg_0);
  return res;
}
void main() {
  vec3 v_1 = fwidth_5d1b39();
  v.tint_symbol = v_1;
}
