#version 310 es


struct frexp_result_f32 {
  float tint_member;
  int tint_member_1;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  float runtime_in = 1.25f;
  frexp_result_f32 res = frexp_result_f32(0.625f, 1);
  frexp_result_f32 v = frexp_result_f32(0.0f, 0);
  v.tint_member = frexp(runtime_in, v.tint_member_1);
  res = v;
  res = frexp_result_f32(0.625f, 1);
  float v_1 = res.tint_member;
  int v_2 = res.tint_member_1;
}
