struct main_inputs {
  uint idx : SV_GroupIndex;
};


cbuffer cbuffer_ub : register(b0) {
  uint4 ub[400];
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

matrix<float16_t, 4, 2> v_2(uint start_byte_offset) {
  vector<float16_t, 2> v_3 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_4 = tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  vector<float16_t, 2> v_5 = tint_bitcast_to_f16(ub[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 4, 2>(v_3, v_4, v_5, tint_bitcast_to_f16(ub[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]));
}

typedef matrix<float16_t, 4, 2> ary_ret[2];
ary_ret v_6(uint start_byte_offset) {
  matrix<float16_t, 4, 2> a[2] = (matrix<float16_t, 4, 2>[2])0;
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 2u)) {
        break;
      }
      a[v_8] = v_2((start_byte_offset + (v_8 * 16u)));
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 2> v_9[2] = a;
  return v_9;
}

typedef float3 ary_ret_1[2];
ary_ret_1 v_10(uint start_byte_offset) {
  float3 a[2] = (float3[2])0;
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 2u)) {
        break;
      }
      a[v_12] = asfloat(ub[((start_byte_offset + (v_12 * 16u)) / 16u)].xyz);
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
  float3 v_13[2] = a;
  return v_13;
}

vector<float16_t, 4> tint_bitcast_to_f16_1(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 4, 4> v_14(uint start_byte_offset) {
  uint4 v_15 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_16 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_15.zw, v_15.xy));
  uint4 v_17 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_18 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_17.zw, v_17.xy));
  uint4 v_19 = ub[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_20 = tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_19.zw, v_19.xy));
  uint4 v_21 = ub[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 4>(v_16, v_18, v_20, tint_bitcast_to_f16_1(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)));
}

matrix<float16_t, 4, 3> v_22(uint start_byte_offset) {
  uint4 v_23 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_24 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_23.zw, v_23.xy)).xyz;
  uint4 v_25 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_26 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_25.zw, v_25.xy)).xyz;
  uint4 v_27 = ub[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_28 = tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_27.zw, v_27.xy)).xyz;
  uint4 v_29 = ub[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_24, v_26, v_28, tint_bitcast_to_f16_1(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_29.zw, v_29.xy)).xyz);
}

matrix<float16_t, 3, 4> v_30(uint start_byte_offset) {
  uint4 v_31 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_32 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_31.zw, v_31.xy));
  uint4 v_33 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_34 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_33.zw, v_33.xy));
  uint4 v_35 = ub[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_32, v_34, tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_35.zw, v_35.xy)));
}

matrix<float16_t, 3, 3> v_36(uint start_byte_offset) {
  uint4 v_37 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_38 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_37.zw, v_37.xy)).xyz;
  uint4 v_39 = ub[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_40 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_39.zw, v_39.xy)).xyz;
  uint4 v_41 = ub[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 3>(v_38, v_40, tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_41.zw, v_41.xy)).xyz);
}

matrix<float16_t, 3, 2> v_42(uint start_byte_offset) {
  vector<float16_t, 2> v_43 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_44 = tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 3, 2>(v_43, v_44, tint_bitcast_to_f16(ub[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]));
}

matrix<float16_t, 2, 4> v_45(uint start_byte_offset) {
  uint4 v_46 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_47 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_46.zw, v_46.xy));
  uint4 v_48 = ub[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 4>(v_47, tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_48.zw, v_48.xy)));
}

matrix<float16_t, 2, 3> v_49(uint start_byte_offset) {
  uint4 v_50 = ub[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_51 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_50.zw, v_50.xy)).xyz;
  uint4 v_52 = ub[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 3>(v_51, tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_52.zw, v_52.xy)).xyz);
}

matrix<float16_t, 2, 2> v_53(uint start_byte_offset) {
  vector<float16_t, 2> v_54 = tint_bitcast_to_f16(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  return matrix<float16_t, 2, 2>(v_54, tint_bitcast_to_f16(ub[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]));
}

float4x4 v_55(uint start_byte_offset) {
  return float4x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]), asfloat(ub[((48u + start_byte_offset) / 16u)]));
}

float4x3 v_56(uint start_byte_offset) {
  return float4x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz), asfloat(ub[((48u + start_byte_offset) / 16u)].xyz));
}

float4x2 v_57(uint start_byte_offset) {
  uint4 v_58 = ub[(start_byte_offset / 16u)];
  uint4 v_59 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_60 = ub[((16u + start_byte_offset) / 16u)];
  uint4 v_61 = ub[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_58.zw, v_58.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_59.zw, v_59.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_60.zw, v_60.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_61.zw, v_61.xy)));
}

float3x4 v_62(uint start_byte_offset) {
  return float3x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]));
}

float3x3 v_63(uint start_byte_offset) {
  return float3x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz));
}

float3x2 v_64(uint start_byte_offset) {
  uint4 v_65 = ub[(start_byte_offset / 16u)];
  uint4 v_66 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_67 = ub[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_65.zw, v_65.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_66.zw, v_66.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_67.zw, v_67.xy)));
}

float2x4 v_68(uint start_byte_offset) {
  return float2x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]));
}

float2x3 v_69(uint start_byte_offset) {
  return float2x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz));
}

float2x2 v_70(uint start_byte_offset) {
  uint4 v_71 = ub[(start_byte_offset / 16u)];
  uint4 v_72 = ub[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_71.zw, v_71.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_72.zw, v_72.xy)));
}

void main_inner(uint idx) {
  float scalar_f32 = asfloat(ub[((idx * 800u) / 16u)][(((idx * 800u) & 15u) >> 2u)]);
  int scalar_i32 = asint(ub[((4u + (idx * 800u)) / 16u)][(((4u + (idx * 800u)) & 15u) >> 2u)]);
  uint scalar_u32 = ub[((8u + (idx * 800u)) / 16u)][(((8u + (idx * 800u)) & 15u) >> 2u)];
  float16_t scalar_f16 = tint_bitcast_to_f16(ub[((12u + (idx * 800u)) / 16u)][(((12u + (idx * 800u)) & 15u) >> 2u)])[select((((12u + (idx * 800u)) % 4u) == 0u), 0u, 1u)];
  uint4 v_73 = ub[((16u + (idx * 800u)) / 16u)];
  float2 vec2_f32 = asfloat(select(((((16u + (idx * 800u)) & 15u) >> 2u) == 2u), v_73.zw, v_73.xy));
  uint4 v_74 = ub[((24u + (idx * 800u)) / 16u)];
  int2 vec2_i32 = asint(select(((((24u + (idx * 800u)) & 15u) >> 2u) == 2u), v_74.zw, v_74.xy));
  uint4 v_75 = ub[((32u + (idx * 800u)) / 16u)];
  uint2 vec2_u32 = select(((((32u + (idx * 800u)) & 15u) >> 2u) == 2u), v_75.zw, v_75.xy);
  vector<float16_t, 2> vec2_f16 = tint_bitcast_to_f16(ub[((40u + (idx * 800u)) / 16u)][(((40u + (idx * 800u)) & 15u) >> 2u)]);
  float3 vec3_f32 = asfloat(ub[((48u + (idx * 800u)) / 16u)].xyz);
  int3 vec3_i32 = asint(ub[((64u + (idx * 800u)) / 16u)].xyz);
  uint3 vec3_u32 = ub[((80u + (idx * 800u)) / 16u)].xyz;
  uint4 v_76 = ub[((96u + (idx * 800u)) / 16u)];
  vector<float16_t, 3> vec3_f16 = tint_bitcast_to_f16_1(select(((((96u + (idx * 800u)) & 15u) >> 2u) == 2u), v_76.zw, v_76.xy)).xyz;
  float4 vec4_f32 = asfloat(ub[((112u + (idx * 800u)) / 16u)]);
  int4 vec4_i32 = asint(ub[((128u + (idx * 800u)) / 16u)]);
  uint4 vec4_u32 = ub[((144u + (idx * 800u)) / 16u)];
  uint4 v_77 = ub[((160u + (idx * 800u)) / 16u)];
  vector<float16_t, 4> vec4_f16 = tint_bitcast_to_f16_1(select(((((160u + (idx * 800u)) & 15u) >> 2u) == 2u), v_77.zw, v_77.xy));
  float2x2 mat2x2_f32 = v_70((168u + (idx * 800u)));
  float2x3 mat2x3_f32 = v_69((192u + (idx * 800u)));
  float2x4 mat2x4_f32 = v_68((224u + (idx * 800u)));
  float3x2 mat3x2_f32 = v_64((256u + (idx * 800u)));
  float3x3 mat3x3_f32 = v_63((288u + (idx * 800u)));
  float3x4 mat3x4_f32 = v_62((336u + (idx * 800u)));
  float4x2 mat4x2_f32 = v_57((384u + (idx * 800u)));
  float4x3 mat4x3_f32 = v_56((416u + (idx * 800u)));
  float4x4 mat4x4_f32 = v_55((480u + (idx * 800u)));
  matrix<float16_t, 2, 2> mat2x2_f16 = v_53((544u + (idx * 800u)));
  matrix<float16_t, 2, 3> mat2x3_f16 = v_49((552u + (idx * 800u)));
  matrix<float16_t, 2, 4> mat2x4_f16 = v_45((568u + (idx * 800u)));
  matrix<float16_t, 3, 2> mat3x2_f16 = v_42((584u + (idx * 800u)));
  matrix<float16_t, 3, 3> mat3x3_f16 = v_36((600u + (idx * 800u)));
  matrix<float16_t, 3, 4> mat3x4_f16 = v_30((624u + (idx * 800u)));
  matrix<float16_t, 4, 2> mat4x2_f16 = v_2((648u + (idx * 800u)));
  matrix<float16_t, 4, 3> mat4x3_f16 = v_22((664u + (idx * 800u)));
  matrix<float16_t, 4, 4> mat4x4_f16 = v_14((696u + (idx * 800u)));
  float3 arr2_vec3_f32[2] = v_10((736u + (idx * 800u)));
  matrix<float16_t, 4, 2> arr2_mat4x2_f16[2] = v_6((768u + (idx * 800u)));
  int v_78 = asint((asuint(tint_f32_to_i32(scalar_f32)) + asuint(scalar_i32)));
  int v_79 = asint((asuint(v_78) + asuint(int(scalar_u32))));
  int v_80 = asint((asuint(v_79) + asuint(tint_f16_to_i32(scalar_f16))));
  int v_81 = asint((asuint(asint((asuint(v_80) + asuint(tint_f32_to_i32(vec2_f32.x))))) + asuint(vec2_i32.x)));
  int v_82 = asint((asuint(v_81) + asuint(int(vec2_u32.x))));
  int v_83 = asint((asuint(v_82) + asuint(tint_f16_to_i32(vec2_f16.x))));
  int v_84 = asint((asuint(asint((asuint(v_83) + asuint(tint_f32_to_i32(vec3_f32.y))))) + asuint(vec3_i32.y)));
  int v_85 = asint((asuint(v_84) + asuint(int(vec3_u32.y))));
  int v_86 = asint((asuint(v_85) + asuint(tint_f16_to_i32(vec3_f16.y))));
  int v_87 = asint((asuint(asint((asuint(v_86) + asuint(tint_f32_to_i32(vec4_f32.z))))) + asuint(vec4_i32.z)));
  int v_88 = asint((asuint(v_87) + asuint(int(vec4_u32.z))));
  int v_89 = asint((asuint(v_88) + asuint(tint_f16_to_i32(vec4_f16.z))));
  int v_90 = asint((asuint(v_89) + asuint(tint_f32_to_i32(mat2x2_f32[0u].x))));
  int v_91 = asint((asuint(v_90) + asuint(tint_f32_to_i32(mat2x3_f32[0u].x))));
  int v_92 = asint((asuint(v_91) + asuint(tint_f32_to_i32(mat2x4_f32[0u].x))));
  int v_93 = asint((asuint(v_92) + asuint(tint_f32_to_i32(mat3x2_f32[0u].x))));
  int v_94 = asint((asuint(v_93) + asuint(tint_f32_to_i32(mat3x3_f32[0u].x))));
  int v_95 = asint((asuint(v_94) + asuint(tint_f32_to_i32(mat3x4_f32[0u].x))));
  int v_96 = asint((asuint(v_95) + asuint(tint_f32_to_i32(mat4x2_f32[0u].x))));
  int v_97 = asint((asuint(v_96) + asuint(tint_f32_to_i32(mat4x3_f32[0u].x))));
  int v_98 = asint((asuint(v_97) + asuint(tint_f32_to_i32(mat4x4_f32[0u].x))));
  int v_99 = asint((asuint(v_98) + asuint(tint_f16_to_i32(mat2x2_f16[0u].x))));
  int v_100 = asint((asuint(v_99) + asuint(tint_f16_to_i32(mat2x3_f16[0u].x))));
  int v_101 = asint((asuint(v_100) + asuint(tint_f16_to_i32(mat2x4_f16[0u].x))));
  int v_102 = asint((asuint(v_101) + asuint(tint_f16_to_i32(mat3x2_f16[0u].x))));
  int v_103 = asint((asuint(v_102) + asuint(tint_f16_to_i32(mat3x3_f16[0u].x))));
  int v_104 = asint((asuint(v_103) + asuint(tint_f16_to_i32(mat3x4_f16[0u].x))));
  int v_105 = asint((asuint(v_104) + asuint(tint_f16_to_i32(mat4x2_f16[0u].x))));
  int v_106 = asint((asuint(v_105) + asuint(tint_f16_to_i32(mat4x3_f16[0u].x))));
  int v_107 = asint((asuint(v_106) + asuint(tint_f16_to_i32(mat4x4_f16[0u].x))));
  int v_108 = asint((asuint(v_107) + asuint(tint_f32_to_i32(arr2_vec3_f32[0u].x))));
  s.Store(0u, asuint(asint((asuint(v_108) + asuint(tint_f16_to_i32(arr2_mat4x2_f16[0u][0u].x))))));
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.idx);
}

