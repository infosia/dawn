#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct S {
  int before;
  uint tint_pad_0;
  f16mat2x3 m;
  uint tint_pad_1;
  uint tint_pad_2;
  uint tint_pad_3;
  uint tint_pad_4;
  uint tint_pad_5;
  uint tint_pad_6;
  uint tint_pad_7;
  uint tint_pad_8;
  uint tint_pad_9;
  uint tint_pad_10;
  int after;
  uint tint_pad_11;
  uint tint_pad_12;
  uint tint_pad_13;
  uint tint_pad_14;
  uint tint_pad_15;
  uint tint_pad_16;
  uint tint_pad_17;
  uint tint_pad_18;
  uint tint_pad_19;
  uint tint_pad_20;
  uint tint_pad_21;
  uint tint_pad_22;
  uint tint_pad_23;
  uint tint_pad_24;
  uint tint_pad_25;
};

layout(binding = 0, std140)
uniform u_block_1_ubo {
  uvec4 inner[32];
} v;
layout(binding = 1, std430)
buffer s_block_1_ssbo {
  S inner[4];
} v_1;
f16vec4 tint_bitcast_to_f16(uvec2 src) {
  return f16vec4(unpackFloat2x16(src.x), unpackFloat2x16(src.y));
}
void tint_store_and_preserve_padding_2(uint target_indices[1], f16mat2x3 value_param) {
  v_1.inner[target_indices[0u]].m[0u] = value_param[0u];
  v_1.inner[target_indices[0u]].m[1u] = value_param[1u];
}
f16mat2x3 v_2(uint start_byte_offset) {
  uvec4 v_3 = v.inner[(start_byte_offset / 16u)];
  f16vec3 v_4 = tint_bitcast_to_f16(mix(v_3.xy, v_3.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u)))).xyz;
  uvec4 v_5 = v.inner[((8u + start_byte_offset) / 16u)];
  return f16mat2x3(v_4, tint_bitcast_to_f16(mix(v_5.xy, v_5.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u)))).xyz);
}
void tint_store_and_preserve_padding_1(uint target_indices[1], S value_param) {
  v_1.inner[target_indices[0u]].before = value_param.before;
  tint_store_and_preserve_padding_2(uint[1](target_indices[0u]), value_param.m);
  v_1.inner[target_indices[0u]].after = value_param.after;
}
S v_6(uint start_byte_offset) {
  int v_7 = int(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  f16mat2x3 v_8 = v_2((8u + start_byte_offset));
  return S(v_7, 0u, v_8, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, int(v.inner[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) % 16u) / 4u)]), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
}
void tint_store_and_preserve_padding(S value_param[4]) {
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      tint_store_and_preserve_padding_1(uint[1](v_10), value_param[v_10]);
      {
        v_9 = (v_10 + 1u);
      }
      continue;
    }
  }
}
S[4] v_11(uint start_byte_offset) {
  S a[4] = S[4](S(0, 0u, f16mat2x3(f16vec3(0.0hf), f16vec3(0.0hf)), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u), S(0, 0u, f16mat2x3(f16vec3(0.0hf), f16vec3(0.0hf)), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u), S(0, 0u, f16mat2x3(f16vec3(0.0hf), f16vec3(0.0hf)), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u), S(0, 0u, f16mat2x3(f16vec3(0.0hf), f16vec3(0.0hf)), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u));
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      a[v_13] = v_6((start_byte_offset + (v_13 * 128u)));
      {
        v_12 = (v_13 + 1u);
      }
      continue;
    }
  }
  return a;
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_store_and_preserve_padding(v_11(0u));
  S v_14 = v_6(256u);
  tint_store_and_preserve_padding_1(uint[1](1u), v_14);
  f16mat2x3 v_15 = v_2(264u);
  tint_store_and_preserve_padding_2(uint[1](3u), v_15);
  v_1.inner[1u].m[0u] = tint_bitcast_to_f16(v.inner[1u].xy).xyz.zxy;
}
