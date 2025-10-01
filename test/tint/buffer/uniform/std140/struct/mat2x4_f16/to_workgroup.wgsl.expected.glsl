#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct S {
  int before;
  f16mat2x4 m;
  int after;
};

layout(binding = 0, std140)
uniform u_block_1_ubo {
  uvec4 inner[32];
} v;
shared S w[4];
f16vec4 tint_bitcast_to_f16(uvec2 src) {
  return f16vec4(unpackFloat2x16(src.x), unpackFloat2x16(src.y));
}
f16mat2x4 v_1(uint start_byte_offset) {
  uvec4 v_2 = v.inner[(start_byte_offset / 16u)];
  f16vec4 v_3 = tint_bitcast_to_f16(mix(v_2.xy, v_2.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_4 = v.inner[((8u + start_byte_offset) / 16u)];
  return f16mat2x4(v_3, tint_bitcast_to_f16(mix(v_4.xy, v_4.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
S v_5(uint start_byte_offset) {
  int v_6 = int(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  f16mat2x4 v_7 = v_1((8u + start_byte_offset));
  return S(v_6, v_7, int(v.inner[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) % 16u) / 4u)]));
}
S[4] v_8(uint start_byte_offset) {
  S a[4] = S[4](S(0, f16mat2x4(f16vec4(0.0hf), f16vec4(0.0hf)), 0), S(0, f16mat2x4(f16vec4(0.0hf), f16vec4(0.0hf)), 0), S(0, f16mat2x4(f16vec4(0.0hf), f16vec4(0.0hf)), 0), S(0, f16mat2x4(f16vec4(0.0hf), f16vec4(0.0hf)), 0));
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
void f_inner(uint tint_local_index) {
  {
    uint v_11 = 0u;
    v_11 = tint_local_index;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      w[v_12] = S(0, f16mat2x4(f16vec4(0.0hf), f16vec4(0.0hf)), 0);
      {
        v_11 = (v_12 + 1u);
      }
      continue;
    }
  }
  barrier();
  w = v_8(0u);
  w[1u] = v_5(256u);
  w[3u].m = v_1(264u);
  w[1u].m[0u] = tint_bitcast_to_f16(v.inner[1u].xy).ywxz;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  f_inner(gl_LocalInvocationIndex);
}
