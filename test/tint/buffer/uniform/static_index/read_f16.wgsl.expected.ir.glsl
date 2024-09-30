#version 310 es
#extension GL_AMD_gpu_shader_half_float: require


struct mat4x2_f16_std140 {
  f16vec2 col0;
  f16vec2 col1;
  f16vec2 col2;
  f16vec2 col3;
};

struct Inner {
  int scalar_i32;
  float scalar_f32;
  float16_t scalar_f16;
};

struct S_std140 {
  float scalar_f32;
  int scalar_i32;
  uint scalar_u32;
  float16_t scalar_f16;
  vec2 vec2_f32;
  ivec2 vec2_i32;
  uvec2 vec2_u32;
  f16vec2 vec2_f16;
  vec3 vec3_f32;
  ivec3 vec3_i32;
  uvec3 vec3_u32;
  f16vec3 vec3_f16;
  vec4 vec4_f32;
  ivec4 vec4_i32;
  uvec4 vec4_u32;
  f16vec4 vec4_f16;
  vec2 mat2x2_f32_col0;
  vec2 mat2x2_f32_col1;
  vec3 mat2x3_f32_col0;
  vec3 mat2x3_f32_col1;
  mat2x4 mat2x4_f32;
  vec2 mat3x2_f32_col0;
  vec2 mat3x2_f32_col1;
  vec2 mat3x2_f32_col2;
  vec3 mat3x3_f32_col0;
  vec3 mat3x3_f32_col1;
  vec3 mat3x3_f32_col2;
  mat3x4 mat3x4_f32;
  vec2 mat4x2_f32_col0;
  vec2 mat4x2_f32_col1;
  vec2 mat4x2_f32_col2;
  vec2 mat4x2_f32_col3;
  vec3 mat4x3_f32_col0;
  vec3 mat4x3_f32_col1;
  vec3 mat4x3_f32_col2;
  vec3 mat4x3_f32_col3;
  mat4 mat4x4_f32;
  f16vec2 mat2x2_f16_col0;
  f16vec2 mat2x2_f16_col1;
  f16vec3 mat2x3_f16_col0;
  f16vec3 mat2x3_f16_col1;
  f16vec4 mat2x4_f16_col0;
  f16vec4 mat2x4_f16_col1;
  f16vec2 mat3x2_f16_col0;
  f16vec2 mat3x2_f16_col1;
  f16vec2 mat3x2_f16_col2;
  f16vec3 mat3x3_f16_col0;
  f16vec3 mat3x3_f16_col1;
  f16vec3 mat3x3_f16_col2;
  f16vec4 mat3x4_f16_col0;
  f16vec4 mat3x4_f16_col1;
  f16vec4 mat3x4_f16_col2;
  f16vec2 mat4x2_f16_col0;
  f16vec2 mat4x2_f16_col1;
  f16vec2 mat4x2_f16_col2;
  f16vec2 mat4x2_f16_col3;
  f16vec3 mat4x3_f16_col0;
  f16vec3 mat4x3_f16_col1;
  f16vec3 mat4x3_f16_col2;
  f16vec3 mat4x3_f16_col3;
  f16vec4 mat4x4_f16_col0;
  f16vec4 mat4x4_f16_col1;
  f16vec4 mat4x4_f16_col2;
  f16vec4 mat4x4_f16_col3;
  vec3 arr2_vec3_f32[2];
  mat4x2_f16_std140 arr2_mat4x2_f16[2];
  Inner struct_inner;
  Inner array_struct_inner[4];
};

layout(binding = 0, std140)
uniform tint_symbol_2_std140_1_ubo {
  S_std140 tint_symbol_1;
} v;
layout(binding = 1, std430)
buffer tint_symbol_4_1_ssbo {
  int tint_symbol_3;
} v_1;
int tint_f16_to_i32(float16_t value) {
  return mix(2147483647, mix((-2147483647 - 1), int(value), (value >= -65504.0hf)), (value <= 65504.0hf));
}
int tint_f32_to_i32(float value) {
  return mix(2147483647, mix((-2147483647 - 1), int(value), (value >= -2147483648.0f)), (value <= 2147483520.0f));
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  float scalar_f32 = v.tint_symbol_1.scalar_f32;
  int scalar_i32 = v.tint_symbol_1.scalar_i32;
  uint scalar_u32 = v.tint_symbol_1.scalar_u32;
  float16_t scalar_f16 = v.tint_symbol_1.scalar_f16;
  vec2 vec2_f32 = v.tint_symbol_1.vec2_f32;
  ivec2 vec2_i32 = v.tint_symbol_1.vec2_i32;
  uvec2 vec2_u32 = v.tint_symbol_1.vec2_u32;
  f16vec2 vec2_f16 = v.tint_symbol_1.vec2_f16;
  vec3 vec3_f32 = v.tint_symbol_1.vec3_f32;
  ivec3 vec3_i32 = v.tint_symbol_1.vec3_i32;
  uvec3 vec3_u32 = v.tint_symbol_1.vec3_u32;
  f16vec3 vec3_f16 = v.tint_symbol_1.vec3_f16;
  vec4 vec4_f32 = v.tint_symbol_1.vec4_f32;
  ivec4 vec4_i32 = v.tint_symbol_1.vec4_i32;
  uvec4 vec4_u32 = v.tint_symbol_1.vec4_u32;
  f16vec4 vec4_f16 = v.tint_symbol_1.vec4_f16;
  vec2 v_2 = v.tint_symbol_1.mat2x2_f32_col0;
  mat2 mat2x2_f32 = mat2(v_2, v.tint_symbol_1.mat2x2_f32_col1);
  vec3 v_3 = v.tint_symbol_1.mat2x3_f32_col0;
  mat2x3 mat2x3_f32 = mat2x3(v_3, v.tint_symbol_1.mat2x3_f32_col1);
  mat2x4 mat2x4_f32 = v.tint_symbol_1.mat2x4_f32;
  vec2 v_4 = v.tint_symbol_1.mat3x2_f32_col0;
  vec2 v_5 = v.tint_symbol_1.mat3x2_f32_col1;
  mat3x2 mat3x2_f32 = mat3x2(v_4, v_5, v.tint_symbol_1.mat3x2_f32_col2);
  vec3 v_6 = v.tint_symbol_1.mat3x3_f32_col0;
  vec3 v_7 = v.tint_symbol_1.mat3x3_f32_col1;
  mat3 mat3x3_f32 = mat3(v_6, v_7, v.tint_symbol_1.mat3x3_f32_col2);
  mat3x4 mat3x4_f32 = v.tint_symbol_1.mat3x4_f32;
  vec2 v_8 = v.tint_symbol_1.mat4x2_f32_col0;
  vec2 v_9 = v.tint_symbol_1.mat4x2_f32_col1;
  vec2 v_10 = v.tint_symbol_1.mat4x2_f32_col2;
  mat4x2 mat4x2_f32 = mat4x2(v_8, v_9, v_10, v.tint_symbol_1.mat4x2_f32_col3);
  vec3 v_11 = v.tint_symbol_1.mat4x3_f32_col0;
  vec3 v_12 = v.tint_symbol_1.mat4x3_f32_col1;
  vec3 v_13 = v.tint_symbol_1.mat4x3_f32_col2;
  mat4x3 mat4x3_f32 = mat4x3(v_11, v_12, v_13, v.tint_symbol_1.mat4x3_f32_col3);
  mat4 mat4x4_f32 = v.tint_symbol_1.mat4x4_f32;
  f16vec2 v_14 = v.tint_symbol_1.mat2x2_f16_col0;
  f16mat2 mat2x2_f16 = f16mat2(v_14, v.tint_symbol_1.mat2x2_f16_col1);
  f16vec3 v_15 = v.tint_symbol_1.mat2x3_f16_col0;
  f16mat2x3 mat2x3_f16 = f16mat2x3(v_15, v.tint_symbol_1.mat2x3_f16_col1);
  f16vec4 v_16 = v.tint_symbol_1.mat2x4_f16_col0;
  f16mat2x4 mat2x4_f16 = f16mat2x4(v_16, v.tint_symbol_1.mat2x4_f16_col1);
  f16vec2 v_17 = v.tint_symbol_1.mat3x2_f16_col0;
  f16vec2 v_18 = v.tint_symbol_1.mat3x2_f16_col1;
  f16mat3x2 mat3x2_f16 = f16mat3x2(v_17, v_18, v.tint_symbol_1.mat3x2_f16_col2);
  f16vec3 v_19 = v.tint_symbol_1.mat3x3_f16_col0;
  f16vec3 v_20 = v.tint_symbol_1.mat3x3_f16_col1;
  f16mat3 mat3x3_f16 = f16mat3(v_19, v_20, v.tint_symbol_1.mat3x3_f16_col2);
  f16vec4 v_21 = v.tint_symbol_1.mat3x4_f16_col0;
  f16vec4 v_22 = v.tint_symbol_1.mat3x4_f16_col1;
  f16mat3x4 mat3x4_f16 = f16mat3x4(v_21, v_22, v.tint_symbol_1.mat3x4_f16_col2);
  f16vec2 v_23 = v.tint_symbol_1.mat4x2_f16_col0;
  f16vec2 v_24 = v.tint_symbol_1.mat4x2_f16_col1;
  f16vec2 v_25 = v.tint_symbol_1.mat4x2_f16_col2;
  f16mat4x2 mat4x2_f16 = f16mat4x2(v_23, v_24, v_25, v.tint_symbol_1.mat4x2_f16_col3);
  f16vec3 v_26 = v.tint_symbol_1.mat4x3_f16_col0;
  f16vec3 v_27 = v.tint_symbol_1.mat4x3_f16_col1;
  f16vec3 v_28 = v.tint_symbol_1.mat4x3_f16_col2;
  f16mat4x3 mat4x3_f16 = f16mat4x3(v_26, v_27, v_28, v.tint_symbol_1.mat4x3_f16_col3);
  f16vec4 v_29 = v.tint_symbol_1.mat4x4_f16_col0;
  f16vec4 v_30 = v.tint_symbol_1.mat4x4_f16_col1;
  f16vec4 v_31 = v.tint_symbol_1.mat4x4_f16_col2;
  f16mat4 mat4x4_f16 = f16mat4(v_29, v_30, v_31, v.tint_symbol_1.mat4x4_f16_col3);
  vec3 arr2_vec3_f32[2] = v.tint_symbol_1.arr2_vec3_f32;
  mat4x2_f16_std140 v_32[2] = v.tint_symbol_1.arr2_mat4x2_f16;
  f16mat4x2 v_33[2] = f16mat4x2[2](f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)), f16mat4x2(f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf), f16vec2(0.0hf)));
  {
    uint v_34 = 0u;
    v_34 = 0u;
    while(true) {
      uint v_35 = v_34;
      if ((v_35 >= 2u)) {
        break;
      }
      v_33[v_35] = f16mat4x2(v_32[v_35].col0, v_32[v_35].col1, v_32[v_35].col2, v_32[v_35].col3);
      {
        v_34 = (v_35 + 1u);
      }
      continue;
    }
  }
  f16mat4x2 arr2_mat4x2_f16[2] = v_33;
  Inner struct_inner = v.tint_symbol_1.struct_inner;
  Inner array_struct_inner[4] = v.tint_symbol_1.array_struct_inner;
  int v_36 = (tint_f32_to_i32(scalar_f32) + scalar_i32);
  int v_37 = (v_36 + int(scalar_u32));
  int v_38 = (v_37 + tint_f16_to_i32(scalar_f16));
  int v_39 = ((v_38 + tint_f32_to_i32(vec2_f32[0u])) + vec2_i32[0u]);
  int v_40 = (v_39 + int(vec2_u32[0u]));
  int v_41 = (v_40 + tint_f16_to_i32(vec2_f16[0u]));
  int v_42 = ((v_41 + tint_f32_to_i32(vec3_f32[1u])) + vec3_i32[1u]);
  int v_43 = (v_42 + int(vec3_u32[1u]));
  int v_44 = (v_43 + tint_f16_to_i32(vec3_f16[1u]));
  int v_45 = ((v_44 + tint_f32_to_i32(vec4_f32[2u])) + vec4_i32[2u]);
  int v_46 = (v_45 + int(vec4_u32[2u]));
  int v_47 = (v_46 + tint_f16_to_i32(vec4_f16[2u]));
  int v_48 = (v_47 + tint_f32_to_i32(mat2x2_f32[0][0u]));
  int v_49 = (v_48 + tint_f32_to_i32(mat2x3_f32[0][0u]));
  int v_50 = (v_49 + tint_f32_to_i32(mat2x4_f32[0][0u]));
  int v_51 = (v_50 + tint_f32_to_i32(mat3x2_f32[0][0u]));
  int v_52 = (v_51 + tint_f32_to_i32(mat3x3_f32[0][0u]));
  int v_53 = (v_52 + tint_f32_to_i32(mat3x4_f32[0][0u]));
  int v_54 = (v_53 + tint_f32_to_i32(mat4x2_f32[0][0u]));
  int v_55 = (v_54 + tint_f32_to_i32(mat4x3_f32[0][0u]));
  int v_56 = (v_55 + tint_f32_to_i32(mat4x4_f32[0][0u]));
  int v_57 = (v_56 + tint_f16_to_i32(mat2x2_f16[0][0u]));
  int v_58 = (v_57 + tint_f16_to_i32(mat2x3_f16[0][0u]));
  int v_59 = (v_58 + tint_f16_to_i32(mat2x4_f16[0][0u]));
  int v_60 = (v_59 + tint_f16_to_i32(mat3x2_f16[0][0u]));
  int v_61 = (v_60 + tint_f16_to_i32(mat3x3_f16[0][0u]));
  int v_62 = (v_61 + tint_f16_to_i32(mat3x4_f16[0][0u]));
  int v_63 = (v_62 + tint_f16_to_i32(mat4x2_f16[0][0u]));
  int v_64 = (v_63 + tint_f16_to_i32(mat4x3_f16[0][0u]));
  int v_65 = (v_64 + tint_f16_to_i32(mat4x4_f16[0][0u]));
  int v_66 = (v_65 + tint_f32_to_i32(arr2_vec3_f32[0][0u]));
  int v_67 = (((v_66 + tint_f16_to_i32(arr2_mat4x2_f16[0][0][0u])) + struct_inner.scalar_i32) + array_struct_inner[0].scalar_i32);
  v_1.tint_symbol_3 = v_67;
}
