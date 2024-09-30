#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  vec4 tint_symbol;
} v;
vec4 fwidthFine_68f4ef() {
  vec4 res = fwidth(vec4(1.0f));
  return res;
}
void main() {
  vec4 v_1 = fwidthFine_68f4ef();
  v.tint_symbol = v_1;
}
