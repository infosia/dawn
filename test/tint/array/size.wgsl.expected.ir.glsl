#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_2_1_ssbo {
  float tint_symbol_1;
} v;
void main() {
  float signed_literal[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  float unsigned_literal[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  float signed_constant[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  float unsigned_constant[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  float shr_const_expr[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  unsigned_literal = signed_literal;
  signed_constant = signed_literal;
  unsigned_constant = signed_literal;
  shr_const_expr = signed_literal;
  float v_1 = signed_literal[0];
  float v_2 = (v_1 + unsigned_literal[0]);
  float v_3 = (v_2 + signed_constant[0]);
  float v_4 = (v_3 + unsigned_constant[0]);
  float v_5 = (v_4 + shr_const_expr[0]);
  v.tint_symbol_1 = v_5;
}
