#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec4 tint_symbol;
} v;
vec4 fwidth_d2ab9a() {
  vec4 res = fwidth(vec4(1.0f));
  return res;
}
void main() {
  vec4 v_1 = fwidth_d2ab9a();
  v.tint_symbol = v_1;
}
