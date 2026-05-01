struct main_inputs {
  uint idx : SV_GroupIndex;
};


cbuffer cbuffer_ub : register(b0) {
  uint4 ub[272];
};
RWByteAddressBuffer s : register(u1);
int tint_f32_to_i32(float value) {
  return int(clamp(value, -2147483648.0f, 2147483520.0f));
}

typedef float3 ary_ret[2];
ary_ret v(uint start_byte_offset) {
  float3 a[2] = (float3[2])0;
  {
    uint v_1 = 0u;
    v_1 = 0u;
    while(true) {
      uint v_2 = v_1;
      if ((v_2 >= 2u)) {
        break;
      }
      a[v_2] = asfloat(ub[((start_byte_offset + (v_2 * 16u)) / 16u)].xyz);
      {
        v_1 = (v_2 + 1u);
      }
    }
  }
  float3 v_3[2] = a;
  return v_3;
}

float4x4 v_4(uint start_byte_offset) {
  return float4x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]), asfloat(ub[((48u + start_byte_offset) / 16u)]));
}

float4x3 v_5(uint start_byte_offset) {
  return float4x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz), asfloat(ub[((48u + start_byte_offset) / 16u)].xyz));
}

float4x2 v_6(uint start_byte_offset) {
  uint4 v_7 = ub[(start_byte_offset / 16u)];
  uint4 v_8 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_9 = ub[((16u + start_byte_offset) / 16u)];
  uint4 v_10 = ub[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_7.zw, v_7.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_8.zw, v_8.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_9.zw, v_9.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_10.zw, v_10.xy)));
}

float3x4 v_11(uint start_byte_offset) {
  return float3x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]));
}

float3x3 v_12(uint start_byte_offset) {
  return float3x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz));
}

float3x2 v_13(uint start_byte_offset) {
  uint4 v_14 = ub[(start_byte_offset / 16u)];
  uint4 v_15 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_16 = ub[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_14.zw, v_14.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_15.zw, v_15.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_16.zw, v_16.xy)));
}

float2x4 v_17(uint start_byte_offset) {
  return float2x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]));
}

float2x3 v_18(uint start_byte_offset) {
  return float2x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz));
}

float2x2 v_19(uint start_byte_offset) {
  uint4 v_20 = ub[(start_byte_offset / 16u)];
  uint4 v_21 = ub[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_20.zw, v_20.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)));
}

void main_inner(uint idx) {
  float scalar_f32 = asfloat(ub[((idx * 544u) / 16u)][(((idx * 544u) & 15u) >> 2u)]);
  int scalar_i32 = asint(ub[((4u + (idx * 544u)) / 16u)][(((4u + (idx * 544u)) & 15u) >> 2u)]);
  uint scalar_u32 = ub[((8u + (idx * 544u)) / 16u)][(((8u + (idx * 544u)) & 15u) >> 2u)];
  uint4 v_22 = ub[((16u + (idx * 544u)) / 16u)];
  float2 vec2_f32 = asfloat(select(((((16u + (idx * 544u)) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy));
  uint4 v_23 = ub[((24u + (idx * 544u)) / 16u)];
  int2 vec2_i32 = asint(select(((((24u + (idx * 544u)) & 15u) >> 2u) == 2u), v_23.zw, v_23.xy));
  uint4 v_24 = ub[((32u + (idx * 544u)) / 16u)];
  uint2 vec2_u32 = select(((((32u + (idx * 544u)) & 15u) >> 2u) == 2u), v_24.zw, v_24.xy);
  float3 vec3_f32 = asfloat(ub[((48u + (idx * 544u)) / 16u)].xyz);
  int3 vec3_i32 = asint(ub[((64u + (idx * 544u)) / 16u)].xyz);
  uint3 vec3_u32 = ub[((80u + (idx * 544u)) / 16u)].xyz;
  float4 vec4_f32 = asfloat(ub[((96u + (idx * 544u)) / 16u)]);
  int4 vec4_i32 = asint(ub[((112u + (idx * 544u)) / 16u)]);
  uint4 vec4_u32 = ub[((128u + (idx * 544u)) / 16u)];
  float2x2 mat2x2_f32 = v_19((144u + (idx * 544u)));
  float2x3 mat2x3_f32 = v_18((160u + (idx * 544u)));
  float2x4 mat2x4_f32 = v_17((192u + (idx * 544u)));
  float3x2 mat3x2_f32 = v_13((224u + (idx * 544u)));
  float3x3 mat3x3_f32 = v_12((256u + (idx * 544u)));
  float3x4 mat3x4_f32 = v_11((304u + (idx * 544u)));
  float4x2 mat4x2_f32 = v_6((352u + (idx * 544u)));
  float4x3 mat4x3_f32 = v_5((384u + (idx * 544u)));
  float4x4 mat4x4_f32 = v_4((448u + (idx * 544u)));
  float3 arr2_vec3_f32[2] = v((512u + (idx * 544u)));
  int v_25 = asint((asuint(tint_f32_to_i32(scalar_f32)) + asuint(scalar_i32)));
  int v_26 = asint((asuint(v_25) + asuint(int(scalar_u32))));
  int v_27 = asint((asuint(asint((asuint(v_26) + asuint(tint_f32_to_i32(vec2_f32.x))))) + asuint(vec2_i32.x)));
  int v_28 = asint((asuint(v_27) + asuint(int(vec2_u32.x))));
  int v_29 = asint((asuint(asint((asuint(v_28) + asuint(tint_f32_to_i32(vec3_f32.y))))) + asuint(vec3_i32.y)));
  int v_30 = asint((asuint(v_29) + asuint(int(vec3_u32.y))));
  int v_31 = asint((asuint(asint((asuint(v_30) + asuint(tint_f32_to_i32(vec4_f32.z))))) + asuint(vec4_i32.z)));
  int v_32 = asint((asuint(v_31) + asuint(int(vec4_u32.z))));
  int v_33 = asint((asuint(v_32) + asuint(tint_f32_to_i32(mat2x2_f32[0u].x))));
  int v_34 = asint((asuint(v_33) + asuint(tint_f32_to_i32(mat2x3_f32[0u].x))));
  int v_35 = asint((asuint(v_34) + asuint(tint_f32_to_i32(mat2x4_f32[0u].x))));
  int v_36 = asint((asuint(v_35) + asuint(tint_f32_to_i32(mat3x2_f32[0u].x))));
  int v_37 = asint((asuint(v_36) + asuint(tint_f32_to_i32(mat3x3_f32[0u].x))));
  int v_38 = asint((asuint(v_37) + asuint(tint_f32_to_i32(mat3x4_f32[0u].x))));
  int v_39 = asint((asuint(v_38) + asuint(tint_f32_to_i32(mat4x2_f32[0u].x))));
  int v_40 = asint((asuint(v_39) + asuint(tint_f32_to_i32(mat4x3_f32[0u].x))));
  int v_41 = asint((asuint(v_40) + asuint(tint_f32_to_i32(mat4x4_f32[0u].x))));
  s.Store(0u, asuint(asint((asuint(v_41) + asuint(tint_f32_to_i32(arr2_vec3_f32[0u].x))))));
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.idx);
}

