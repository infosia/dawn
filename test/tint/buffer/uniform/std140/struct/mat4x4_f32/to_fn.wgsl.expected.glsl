#version 310 es


struct S {
  int before;
  mat4 m;
  int after;
};

layout(binding = 0, std140)
uniform u_block_1_ubo {
  uvec4 inner[48];
} v_1;
void a(S a_1[4]) {
}
void b(S s) {
}
void c(mat4 m) {
}
void d(vec4 v) {
}
void e(float f_1) {
}
mat4 v_2(uint start_byte_offset) {
  return mat4(uintBitsToFloat(v_1.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_1.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((32u + start_byte_offset) / 16u)]), uintBitsToFloat(v_1.inner[((48u + start_byte_offset) / 16u)]));
}
S v_3(uint start_byte_offset) {
  int v_4 = int(v_1.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  mat4 v_5 = v_2((16u + start_byte_offset));
  return S(v_4, v_5, int(v_1.inner[((128u + start_byte_offset) / 16u)][(((128u + start_byte_offset) % 16u) / 4u)]));
}
S[4] v_6(uint start_byte_offset) {
  S a_2[4] = S[4](S(0, mat4(vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f)), 0), S(0, mat4(vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f)), 0), S(0, mat4(vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f)), 0), S(0, mat4(vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f)), 0));
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      a_2[v_8] = v_3((start_byte_offset + (v_8 * 192u)));
      {
        v_7 = (v_8 + 1u);
      }
      continue;
    }
  }
  return a_2;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  a(v_6(0u));
  b(v_3(384u));
  c(v_2(400u));
  d(uintBitsToFloat(v_1.inner[2u]).ywxz);
  e(uintBitsToFloat(v_1.inner[2u]).ywxz.x);
}
