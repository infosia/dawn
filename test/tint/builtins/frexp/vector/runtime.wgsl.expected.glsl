#version 310 es


struct frexp_result_vec2_f32 {
  vec2 tint_member;
  ivec2 tint_member_1;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  vec2 v = vec2(1.25f, 3.75f);
  frexp_result_vec2_f32 v_1 = frexp_result_vec2_f32(vec2(0.0f), ivec2(0));
  v_1.tint_member = frexp(v, v_1.tint_member_1);
  frexp_result_vec2_f32 res = v_1;
  vec2 v_2 = res.tint_member;
  ivec2 v_3 = res.tint_member_1;
}
