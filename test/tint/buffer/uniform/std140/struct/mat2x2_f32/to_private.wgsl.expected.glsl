#version 310 es


struct S {
  int before;
  mat2 m;
  int after;
};

layout(binding = 0, std140)
uniform u_block_1_ubo {
  uvec4 inner[32];
} v;
S p[4] = S[4](S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0));
mat2 v_1(uint start_byte_offset) {
  uvec4 v_2 = v.inner[(start_byte_offset / 16u)];
  vec2 v_3 = uintBitsToFloat(mix(v_2.xy, v_2.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_4 = v.inner[((8u + start_byte_offset) / 16u)];
  return mat2(v_3, uintBitsToFloat(mix(v_4.xy, v_4.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
S v_5(uint start_byte_offset) {
  int v_6 = int(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  mat2 v_7 = v_1((8u + start_byte_offset));
  return S(v_6, v_7, int(v.inner[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) % 16u) / 4u)]));
}
S[4] v_8(uint start_byte_offset) {
  S a[4] = S[4](S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0), S(0, mat2(vec2(0.0f), vec2(0.0f)), 0));
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      a[v_10] = v_5((start_byte_offset + (v_10 * 128u)));
      {
        v_9 = (v_10 + 1u);
      }
      continue;
    }
  }
  return a;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  p = v_8(0u);
  p[1u] = v_5(256u);
  p[3u].m = v_1(264u);
  p[1u].m[0u] = uintBitsToFloat(v.inner[1u].xy).yx;
}
