#version 310 es


struct frexp_result_f32 {
  float tint_member;
  int tint_member_1;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  float v = 1.25f;
  frexp_result_f32 v_1 = frexp_result_f32(0.0f, 0);
  v_1.tint_member = frexp(v, v_1.tint_member_1);
  frexp_result_f32 res = v_1;
  float v_2 = res.tint_member;
  int v_3 = res.tint_member_1;
}
