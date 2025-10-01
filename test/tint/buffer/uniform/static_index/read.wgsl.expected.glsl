#version 310 es


struct Inner {
  int scalar_i32;
  float scalar_f32;
};

layout(binding = 0, std140)
uniform ub_block_1_ubo {
  uvec4 inner[44];
} v;
layout(binding = 1, std430)
buffer s_block_1_ssbo {
  int inner;
} v_1;
int tint_f32_to_i32(float value) {
  return int(clamp(value, -2147483648.0f, 2147483520.0f));
}
Inner v_2(uint start_byte_offset) {
  int v_3 = int(v.inner[(start_byte_offset / 16u)][((start_byte_offset % 16u) / 4u)]);
  return Inner(v_3, uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) % 16u) / 4u)]));
}
Inner[4] v_4(uint start_byte_offset) {
  Inner a[4] = Inner[4](Inner(0, 0.0f), Inner(0, 0.0f), Inner(0, 0.0f), Inner(0, 0.0f));
  {
    uint v_5 = 0u;
    v_5 = 0u;
    while(true) {
      uint v_6 = v_5;
      if ((v_6 >= 4u)) {
        break;
      }
      a[v_6] = v_2((start_byte_offset + (v_6 * 32u)));
      {
        v_5 = (v_6 + 1u);
      }
      continue;
    }
  }
  return a;
}
vec3[2] v_7(uint start_byte_offset) {
  vec3 a[2] = vec3[2](vec3(0.0f), vec3(0.0f));
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 2u)) {
        break;
      }
      a[v_9] = uintBitsToFloat(v.inner[((start_byte_offset + (v_9 * 16u)) / 16u)].xyz);
      {
        v_8 = (v_9 + 1u);
      }
      continue;
    }
  }
  return a;
}
mat4 v_10(uint start_byte_offset) {
  return mat4(uintBitsToFloat(v.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)]), uintBitsToFloat(v.inner[((48u + start_byte_offset) / 16u)]));
}
mat4x3 v_11(uint start_byte_offset) {
  return mat4x3(uintBitsToFloat(v.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v.inner[((48u + start_byte_offset) / 16u)].xyz));
}
mat4x2 v_12(uint start_byte_offset) {
  uvec4 v_13 = v.inner[(start_byte_offset / 16u)];
  vec2 v_14 = uintBitsToFloat(mix(v_13.xy, v_13.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_15 = v.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_16 = uintBitsToFloat(mix(v_15.xy, v_15.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_17 = v.inner[((16u + start_byte_offset) / 16u)];
  vec2 v_18 = uintBitsToFloat(mix(v_17.xy, v_17.zw, bvec2(((((16u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_19 = v.inner[((24u + start_byte_offset) / 16u)];
  return mat4x2(v_14, v_16, v_18, uintBitsToFloat(mix(v_19.xy, v_19.zw, bvec2(((((24u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
mat3x4 v_20(uint start_byte_offset) {
  return mat3x4(uintBitsToFloat(v.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)]));
}
mat3 v_21(uint start_byte_offset) {
  return mat3(uintBitsToFloat(v.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)].xyz), uintBitsToFloat(v.inner[((32u + start_byte_offset) / 16u)].xyz));
}
mat3x2 v_22(uint start_byte_offset) {
  uvec4 v_23 = v.inner[(start_byte_offset / 16u)];
  vec2 v_24 = uintBitsToFloat(mix(v_23.xy, v_23.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_25 = v.inner[((8u + start_byte_offset) / 16u)];
  vec2 v_26 = uintBitsToFloat(mix(v_25.xy, v_25.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u))));
  uvec4 v_27 = v.inner[((16u + start_byte_offset) / 16u)];
  return mat3x2(v_24, v_26, uintBitsToFloat(mix(v_27.xy, v_27.zw, bvec2(((((16u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
mat2x4 v_28(uint start_byte_offset) {
  return mat2x4(uintBitsToFloat(v.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)]));
}
mat2x3 v_29(uint start_byte_offset) {
  return mat2x3(uintBitsToFloat(v.inner[(start_byte_offset / 16u)].xyz), uintBitsToFloat(v.inner[((16u + start_byte_offset) / 16u)].xyz));
}
mat2 v_30(uint start_byte_offset) {
  uvec4 v_31 = v.inner[(start_byte_offset / 16u)];
  vec2 v_32 = uintBitsToFloat(mix(v_31.xy, v_31.zw, bvec2((((start_byte_offset % 16u) / 4u) == 2u))));
  uvec4 v_33 = v.inner[((8u + start_byte_offset) / 16u)];
  return mat2(v_32, uintBitsToFloat(mix(v_33.xy, v_33.zw, bvec2(((((8u + start_byte_offset) % 16u) / 4u) == 2u)))));
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  float scalar_f32 = uintBitsToFloat(v.inner[0u].x);
  int scalar_i32 = int(v.inner[0u].y);
  uint scalar_u32 = v.inner[0u].z;
  vec2 vec2_f32 = uintBitsToFloat(v.inner[1u].xy);
  ivec2 vec2_i32 = ivec2(v.inner[1u].zw);
  uvec2 vec2_u32 = v.inner[2u].xy;
  vec3 vec3_f32 = uintBitsToFloat(v.inner[3u].xyz);
  ivec3 vec3_i32 = ivec3(v.inner[4u].xyz);
  uvec3 vec3_u32 = v.inner[5u].xyz;
  vec4 vec4_f32 = uintBitsToFloat(v.inner[6u]);
  ivec4 vec4_i32 = ivec4(v.inner[7u]);
  uvec4 vec4_u32 = v.inner[8u];
  mat2 mat2x2_f32 = v_30(144u);
  mat2x3 mat2x3_f32 = v_29(160u);
  mat2x4 mat2x4_f32 = v_28(192u);
  mat3x2 mat3x2_f32 = v_22(224u);
  mat3 mat3x3_f32 = v_21(256u);
  mat3x4 mat3x4_f32 = v_20(304u);
  mat4x2 mat4x2_f32 = v_12(352u);
  mat4x3 mat4x3_f32 = v_11(384u);
  mat4 mat4x4_f32 = v_10(448u);
  vec3 arr2_vec3_f32[2] = v_7(512u);
  Inner struct_inner = v_2(544u);
  Inner array_struct_inner[4] = v_4(576u);
  uint v_34 = uint(tint_f32_to_i32(scalar_f32));
  int v_35 = int((v_34 + uint(scalar_i32)));
  int v_36 = int(scalar_u32);
  uint v_37 = uint(v_35);
  int v_38 = int((v_37 + uint(v_36)));
  int v_39 = tint_f32_to_i32(vec2_f32.x);
  uint v_40 = uint(v_38);
  uint v_41 = uint(int((v_40 + uint(v_39))));
  int v_42 = int((v_41 + uint(vec2_i32.x)));
  int v_43 = int(vec2_u32.x);
  uint v_44 = uint(v_42);
  int v_45 = int((v_44 + uint(v_43)));
  int v_46 = tint_f32_to_i32(vec3_f32.y);
  uint v_47 = uint(v_45);
  uint v_48 = uint(int((v_47 + uint(v_46))));
  int v_49 = int((v_48 + uint(vec3_i32.y)));
  int v_50 = int(vec3_u32.y);
  uint v_51 = uint(v_49);
  int v_52 = int((v_51 + uint(v_50)));
  int v_53 = tint_f32_to_i32(vec4_f32.z);
  uint v_54 = uint(v_52);
  uint v_55 = uint(int((v_54 + uint(v_53))));
  int v_56 = int((v_55 + uint(vec4_i32.z)));
  int v_57 = int(vec4_u32.z);
  uint v_58 = uint(v_56);
  int v_59 = int((v_58 + uint(v_57)));
  int v_60 = tint_f32_to_i32(mat2x2_f32[0u].x);
  uint v_61 = uint(v_59);
  int v_62 = int((v_61 + uint(v_60)));
  int v_63 = tint_f32_to_i32(mat2x3_f32[0u].x);
  uint v_64 = uint(v_62);
  int v_65 = int((v_64 + uint(v_63)));
  int v_66 = tint_f32_to_i32(mat2x4_f32[0u].x);
  uint v_67 = uint(v_65);
  int v_68 = int((v_67 + uint(v_66)));
  int v_69 = tint_f32_to_i32(mat3x2_f32[0u].x);
  uint v_70 = uint(v_68);
  int v_71 = int((v_70 + uint(v_69)));
  int v_72 = tint_f32_to_i32(mat3x3_f32[0u].x);
  uint v_73 = uint(v_71);
  int v_74 = int((v_73 + uint(v_72)));
  int v_75 = tint_f32_to_i32(mat3x4_f32[0u].x);
  uint v_76 = uint(v_74);
  int v_77 = int((v_76 + uint(v_75)));
  int v_78 = tint_f32_to_i32(mat4x2_f32[0u].x);
  uint v_79 = uint(v_77);
  int v_80 = int((v_79 + uint(v_78)));
  int v_81 = tint_f32_to_i32(mat4x3_f32[0u].x);
  uint v_82 = uint(v_80);
  int v_83 = int((v_82 + uint(v_81)));
  int v_84 = tint_f32_to_i32(mat4x4_f32[0u].x);
  uint v_85 = uint(v_83);
  int v_86 = int((v_85 + uint(v_84)));
  int v_87 = tint_f32_to_i32(arr2_vec3_f32[0u].x);
  uint v_88 = uint(v_86);
  uint v_89 = uint(int((v_88 + uint(v_87))));
  uint v_90 = uint(int((v_89 + uint(struct_inner.scalar_i32))));
  v_1.inner = int((v_90 + uint(array_struct_inner[0u].scalar_i32)));
}
