struct Inner {
  int scalar_i32;
  float scalar_f32;
  float16_t scalar_f16;
};


cbuffer cbuffer_ub : register(b0) {
  uint4 ub[55];
};
RWByteAddressBuffer s : register(u1);
int tint_f16_to_i32(float16_t value) {
  return int(clamp(value, float16_t(-65504.0h), float16_t(65504.0h)));
}

int tint_f32_to_i32(float value) {
  return int(clamp(value, -2147483648.0f, 2147483520.0f));
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  uint2 v_1 = uint2(v, v);
  vector<uint16_t, 2> v16 = vector<uint16_t, 2>(((v_1 >> uint2(0u, 16u)) & (65535u).xx));
  return asfloat16(v16);
}

Inner v_2(uint start_byte_offset) {
  int v_3 = asint(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float v_4 = asfloat(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  Inner v_5 = {v_3, v_4, tint_bitcast_to_f16(ub[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)])[select((((8u + start_byte_offset) % 4u) == 0u), 0u, 1u)]};
  return v_5;
}

typedef Inner ary_ret[4];
ary_ret v_6(uint start_byte_offset) {
  Inner a[4] = (Inner[4])0;
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      Inner v_9 = v_2((start_byte_offset + (v_8 * 16u)));
      a[v_8] = v_9;
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
  Inner v_10[4] = a;
  return v_10;
}

matrix<float16_t, 4, 2> v_11(uint start_byte_offset) {
  vector<float16_t, 2> v_12 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_13 = tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  vector<float16_t, 2> v_14 = tint_bitcast_to_f16(ub[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 4, 2>(v_12, v_13, v_14, tint_bitcast_to_f16(ub[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]));
}

typedef matrix<float16_t, 4, 2> ary_ret_1[2];
ary_ret_1 v_15(uint start_byte_offset) {
  matrix<float16_t, 4, 2> a[2] = (matrix<float16_t, 4, 2>[2])0;
  {
    uint v_16 = 0u;
    v_16 = 0u;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 2u)) {
        break;
      }
      a[v_17] = v_11((start_byte_offset + (v_17 * 16u)));
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 2> v_18[2] = a;
  return v_18;
}

typedef float3 ary_ret_2[2];
ary_ret_2 v_19(uint start_byte_offset) {
  float3 a[2] = (float3[2])0;
  {
    uint v_20 = 0u;
    v_20 = 0u;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 2u)) {
        break;
      }
      a[v_21] = asfloat(ub[((start_byte_offset + (v_21 * 16u)) / 16u)].xyz);
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
  float3 v_22[2] = a;
  return v_22;
}

vector<float16_t, 4> tint_bitcast_to_f16_1(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 4, 4> v_23(uint start_byte_offset) {
  uint4 v_24 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_25 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_24.zw, v_24.xy));
  uint4 v_26 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_27 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_26.zw, v_26.xy));
  uint4 v_28 = ub[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_29 = tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_28.zw, v_28.xy));
  uint4 v_30 = ub[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 4>(v_25, v_27, v_29, tint_bitcast_to_f16_1(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_30.zw, v_30.xy)));
}

matrix<float16_t, 4, 3> v_31(uint start_byte_offset) {
  uint4 v_32 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_33 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_32.zw, v_32.xy)).xyz;
  uint4 v_34 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_35 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_34.zw, v_34.xy)).xyz;
  uint4 v_36 = ub[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_37 = tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_36.zw, v_36.xy)).xyz;
  uint4 v_38 = ub[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_33, v_35, v_37, tint_bitcast_to_f16_1(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_38.zw, v_38.xy)).xyz);
}

matrix<float16_t, 3, 4> v_39(uint start_byte_offset) {
  uint4 v_40 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_41 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_40.zw, v_40.xy));
  uint4 v_42 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_43 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_42.zw, v_42.xy));
  uint4 v_44 = ub[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_41, v_43, tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_44.zw, v_44.xy)));
}

matrix<float16_t, 3, 3> v_45(uint start_byte_offset) {
  uint4 v_46 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_47 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_46.zw, v_46.xy)).xyz;
  uint4 v_48 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_49 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_48.zw, v_48.xy)).xyz;
  uint4 v_50 = ub[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 3>(v_47, v_49, tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_50.zw, v_50.xy)).xyz);
}

matrix<float16_t, 3, 2> v_51(uint start_byte_offset) {
  vector<float16_t, 2> v_52 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_53 = tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 3, 2>(v_52, v_53, tint_bitcast_to_f16(ub[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]));
}

matrix<float16_t, 2, 4> v_54(uint start_byte_offset) {
  uint4 v_55 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_56 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_55.zw, v_55.xy));
  uint4 v_57 = ub[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 4>(v_56, tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_57.zw, v_57.xy)));
}

matrix<float16_t, 2, 3> v_58(uint start_byte_offset) {
  uint4 v_59 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_60 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_59.zw, v_59.xy)).xyz;
  uint4 v_61 = ub[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 3>(v_60, tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_61.zw, v_61.xy)).xyz);
}

matrix<float16_t, 2, 2> v_62(uint start_byte_offset) {
  vector<float16_t, 2> v_63 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  return matrix<float16_t, 2, 2>(v_63, tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]));
}

float4x4 v_64(uint start_byte_offset) {
  return float4x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]), asfloat(ub[((48u + start_byte_offset) / 16u)]));
}

float4x3 v_65(uint start_byte_offset) {
  return float4x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz), asfloat(ub[((48u + start_byte_offset) / 16u)].xyz));
}

float4x2 v_66(uint start_byte_offset) {
  uint4 v_67 = ub[(start_byte_offset / 16u)];
  uint4 v_68 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_69 = ub[((16u + start_byte_offset) / 16u)];
  uint4 v_70 = ub[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_67.zw, v_67.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_68.zw, v_68.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_69.zw, v_69.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_70.zw, v_70.xy)));
}

float3x4 v_71(uint start_byte_offset) {
  return float3x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]));
}

float3x3 v_72(uint start_byte_offset) {
  return float3x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz));
}

float3x2 v_73(uint start_byte_offset) {
  uint4 v_74 = ub[(start_byte_offset / 16u)];
  uint4 v_75 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_76 = ub[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_74.zw, v_74.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_75.zw, v_75.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_76.zw, v_76.xy)));
}

float2x4 v_77(uint start_byte_offset) {
  return float2x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]));
}

float2x3 v_78(uint start_byte_offset) {
  return float2x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz));
}

float2x2 v_79(uint start_byte_offset) {
  uint4 v_80 = ub[(start_byte_offset / 16u)];
  uint4 v_81 = ub[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_80.zw, v_80.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_81.zw, v_81.xy)));
}

[numthreads(1, 1, 1)]
void main() {
  float scalar_f32 = asfloat(ub[0u].x);
  int scalar_i32 = asint(ub[0u].y);
  uint scalar_u32 = ub[0u].z;
  float16_t scalar_f16 = tint_bitcast_to_f16(ub[0u].w).x;
  float2 vec2_f32 = asfloat(ub[1u].xy);
  int2 vec2_i32 = asint(ub[1u].zw);
  uint2 vec2_u32 = ub[2u].xy;
  vector<float16_t, 2> vec2_f16 = tint_bitcast_to_f16(ub[2u].z);
  float3 vec3_f32 = asfloat(ub[3u].xyz);
  int3 vec3_i32 = asint(ub[4u].xyz);
  uint3 vec3_u32 = ub[5u].xyz;
  vector<float16_t, 3> vec3_f16 = tint_bitcast_to_f16_1(ub[6u].xy).xyz;
  float4 vec4_f32 = asfloat(ub[7u]);
  int4 vec4_i32 = asint(ub[8u]);
  uint4 vec4_u32 = ub[9u];
  vector<float16_t, 4> vec4_f16 = tint_bitcast_to_f16_1(ub[10u].xy);
  float2x2 mat2x2_f32 = v_79(168u);
  float2x3 mat2x3_f32 = v_78(192u);
  float2x4 mat2x4_f32 = v_77(224u);
  float3x2 mat3x2_f32 = v_73(256u);
  float3x3 mat3x3_f32 = v_72(288u);
  float3x4 mat3x4_f32 = v_71(336u);
  float4x2 mat4x2_f32 = v_66(384u);
  float4x3 mat4x3_f32 = v_65(416u);
  float4x4 mat4x4_f32 = v_64(480u);
  matrix<float16_t, 2, 2> mat2x2_f16 = v_62(544u);
  matrix<float16_t, 2, 3> mat2x3_f16 = v_58(552u);
  matrix<float16_t, 2, 4> mat2x4_f16 = v_54(568u);
  matrix<float16_t, 3, 2> mat3x2_f16 = v_51(584u);
  matrix<float16_t, 3, 3> mat3x3_f16 = v_45(600u);
  matrix<float16_t, 3, 4> mat3x4_f16 = v_39(624u);
  matrix<float16_t, 4, 2> mat4x2_f16 = v_11(648u);
  matrix<float16_t, 4, 3> mat4x3_f16 = v_31(664u);
  matrix<float16_t, 4, 4> mat4x4_f16 = v_23(696u);
  float3 arr2_vec3_f32[2] = v_19(736u);
  matrix<float16_t, 4, 2> arr2_mat4x2_f16[2] = v_15(768u);
  Inner struct_inner = v_2(800u);
  Inner array_struct_inner[4] = v_6(816u);
  int v_82 = asint((asuint(tint_f32_to_i32(scalar_f32)) + asuint(scalar_i32)));
  int v_83 = asint((asuint(v_82) + asuint(int(scalar_u32))));
  int v_84 = asint((asuint(v_83) + asuint(tint_f16_to_i32(scalar_f16))));
  int v_85 = asint((asuint(asint((asuint(v_84) + asuint(tint_f32_to_i32(vec2_f32.x))))) + asuint(vec2_i32.x)));
  int v_86 = asint((asuint(v_85) + asuint(int(vec2_u32.x))));
  int v_87 = asint((asuint(v_86) + asuint(tint_f16_to_i32(vec2_f16.x))));
  int v_88 = asint((asuint(asint((asuint(v_87) + asuint(tint_f32_to_i32(vec3_f32.y))))) + asuint(vec3_i32.y)));
  int v_89 = asint((asuint(v_88) + asuint(int(vec3_u32.y))));
  int v_90 = asint((asuint(v_89) + asuint(tint_f16_to_i32(vec3_f16.y))));
  int v_91 = asint((asuint(asint((asuint(v_90) + asuint(tint_f32_to_i32(vec4_f32.z))))) + asuint(vec4_i32.z)));
  int v_92 = asint((asuint(v_91) + asuint(int(vec4_u32.z))));
  int v_93 = asint((asuint(v_92) + asuint(tint_f16_to_i32(vec4_f16.z))));
  int v_94 = asint((asuint(v_93) + asuint(tint_f32_to_i32(mat2x2_f32[0u].x))));
  int v_95 = asint((asuint(v_94) + asuint(tint_f32_to_i32(mat2x3_f32[0u].x))));
  int v_96 = asint((asuint(v_95) + asuint(tint_f32_to_i32(mat2x4_f32[0u].x))));
  int v_97 = asint((asuint(v_96) + asuint(tint_f32_to_i32(mat3x2_f32[0u].x))));
  int v_98 = asint((asuint(v_97) + asuint(tint_f32_to_i32(mat3x3_f32[0u].x))));
  int v_99 = asint((asuint(v_98) + asuint(tint_f32_to_i32(mat3x4_f32[0u].x))));
  int v_100 = asint((asuint(v_99) + asuint(tint_f32_to_i32(mat4x2_f32[0u].x))));
  int v_101 = asint((asuint(v_100) + asuint(tint_f32_to_i32(mat4x3_f32[0u].x))));
  int v_102 = asint((asuint(v_101) + asuint(tint_f32_to_i32(mat4x4_f32[0u].x))));
  int v_103 = asint((asuint(v_102) + asuint(tint_f16_to_i32(mat2x2_f16[0u].x))));
  int v_104 = asint((asuint(v_103) + asuint(tint_f16_to_i32(mat2x3_f16[0u].x))));
  int v_105 = asint((asuint(v_104) + asuint(tint_f16_to_i32(mat2x4_f16[0u].x))));
  int v_106 = asint((asuint(v_105) + asuint(tint_f16_to_i32(mat3x2_f16[0u].x))));
  int v_107 = asint((asuint(v_106) + asuint(tint_f16_to_i32(mat3x3_f16[0u].x))));
  int v_108 = asint((asuint(v_107) + asuint(tint_f16_to_i32(mat3x4_f16[0u].x))));
  int v_109 = asint((asuint(v_108) + asuint(tint_f16_to_i32(mat4x2_f16[0u].x))));
  int v_110 = asint((asuint(v_109) + asuint(tint_f16_to_i32(mat4x3_f16[0u].x))));
  int v_111 = asint((asuint(v_110) + asuint(tint_f16_to_i32(mat4x4_f16[0u].x))));
  int v_112 = asint((asuint(v_111) + asuint(tint_f32_to_i32(arr2_vec3_f32[0u].x))));
  s.Store(0u, asuint(asint((asuint(asint((asuint(asint((asuint(v_112) + asuint(tint_f16_to_i32(arr2_mat4x2_f16[0u][0u].x))))) + asuint(struct_inner.scalar_i32)))) + asuint(array_struct_inner[0u].scalar_i32)))));
}

