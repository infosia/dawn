#version 310 es


struct frexp_result_f32 {
  float fract;
  int exp;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  float tint_symbol_1 = 1.25f;
  frexp_result_f32 v = frexp_result_f32(0.0f, 0);
  float v_1 = frexp(tint_symbol_1, v.exp);
  v.fract = v_1;
  frexp_result_f32 res = v;
  float tint_symbol_2 = res.fract;
  int tint_symbol_3 = res.exp;
}
