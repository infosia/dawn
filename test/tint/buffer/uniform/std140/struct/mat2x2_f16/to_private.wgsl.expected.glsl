#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct S {
  int before;
  f16mat2 m;
  int after;
};

layout(binding = 0, std140)
uniform u_block_1_ubo {
  uvec4 inner[32];
} v;
S p[4] = S[4](S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0));
f16vec2 tint_bitcast_to_f16(uint src) {
  return unpackFloat2x16(src);
}
f16mat2 v_1(uint start_byte_offset) {
  f16vec2 v_2 = tint_bitcast_to_f16(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  return f16mat2(v_2, tint_bitcast_to_f16(v.inner[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) % 16u) / 4u)]));
}
S v_3(uint start_byte_offset) {
  int v_4 = int(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  f16mat2 v_5 = v_1((4u + start_byte_offset));
  return S(v_4, v_5, int(v.inner[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) % 16u) / 4u)]));
}
S[4] v_6(uint start_byte_offset) {
  S a[4] = S[4](S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0), S(0, f16mat2(f16vec2(0.0hf), f16vec2(0.0hf)), 0));
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      a[v_8] = v_3((start_byte_offset + (v_8 * 128u)));
      {
        v_7 = (v_8 + 1u);
      }
      continue;
    }
  }
  return a;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  p = v_6(0u);
  p[1u] = v_3(256u);
  p[3u].m = v_1(260u);
  p[1u].m[0u] = tint_bitcast_to_f16(v.inner[0u].z).yx;
}
