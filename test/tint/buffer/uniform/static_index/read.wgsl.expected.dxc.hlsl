struct Inner {
  int scalar_i32;
  float scalar_f32;
};


cbuffer cbuffer_ub : register(b0) {
  uint4 ub[44];
};
RWByteAddressBuffer s : register(u1);
int tint_f32_to_i32(float value) {
  return int(clamp(value, -2147483648.0f, 2147483520.0f));
}

Inner v(uint start_byte_offset) {
  Inner v_1 = {asint(ub[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]), asfloat(ub[((16u + start_byte_offset) / 16u)][(((16u + start_byte_offset) & 15u) >> 2u)])};
  return v_1;
}

typedef Inner ary_ret[4];
ary_ret v_2(uint start_byte_offset) {
  Inner a[4] = (Inner[4])0;
  {
    uint v_3 = 0u;
    v_3 = 0u;
    while(true) {
      uint v_4 = v_3;
      if ((v_4 >= 4u)) {
        break;
      }
      Inner v_5 = v((start_byte_offset + (v_4 * 32u)));
      a[v_4] = v_5;
      {
        v_3 = (v_4 + 1u);
      }
    }
  }
  Inner v_6[4] = a;
  return v_6;
}

typedef float3 ary_ret_1[2];
ary_ret_1 v_7(uint start_byte_offset) {
  float3 a[2] = (float3[2])0;
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 2u)) {
        break;
      }
      a[v_9] = asfloat(ub[((start_byte_offset + (v_9 * 16u)) / 16u)].xyz);
      {
        v_8 = (v_9 + 1u);
      }
    }
  }
  float3 v_10[2] = a;
  return v_10;
}

float4x4 v_11(uint start_byte_offset) {
  return float4x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]), asfloat(ub[((48u + start_byte_offset) / 16u)]));
}

float4x3 v_12(uint start_byte_offset) {
  return float4x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz), asfloat(ub[((48u + start_byte_offset) / 16u)].xyz));
}

float4x2 v_13(uint start_byte_offset) {
  uint4 v_14 = ub[(start_byte_offset / 16u)];
  uint4 v_15 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_16 = ub[((16u + start_byte_offset) / 16u)];
  uint4 v_17 = ub[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_14.zw, v_14.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_15.zw, v_15.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_16.zw, v_16.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_17.zw, v_17.xy)));
}

float3x4 v_18(uint start_byte_offset) {
  return float3x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]), asfloat(ub[((32u + start_byte_offset) / 16u)]));
}

float3x3 v_19(uint start_byte_offset) {
  return float3x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz), asfloat(ub[((32u + start_byte_offset) / 16u)].xyz));
}

float3x2 v_20(uint start_byte_offset) {
  uint4 v_21 = ub[(start_byte_offset / 16u)];
  uint4 v_22 = ub[((8u + start_byte_offset) / 16u)];
  uint4 v_23 = ub[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_21.zw, v_21.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_22.zw, v_22.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_23.zw, v_23.xy)));
}

float2x4 v_24(uint start_byte_offset) {
  return float2x4(asfloat(ub[(start_byte_offset / 16u)]), asfloat(ub[((16u + start_byte_offset) / 16u)]));
}

float2x3 v_25(uint start_byte_offset) {
  return float2x3(asfloat(ub[(start_byte_offset / 16u)].xyz), asfloat(ub[((16u + start_byte_offset) / 16u)].xyz));
}

float2x2 v_26(uint start_byte_offset) {
  uint4 v_27 = ub[(start_byte_offset / 16u)];
  uint4 v_28 = ub[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_27.zw, v_27.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_28.zw, v_28.xy)));
}

[numthreads(1, 1, 1)]
void main() {
  float scalar_f32 = asfloat(ub[0u].x);
  int scalar_i32 = asint(ub[0u].y);
  uint scalar_u32 = ub[0u].z;
  float2 vec2_f32 = asfloat(ub[1u].xy);
  int2 vec2_i32 = asint(ub[1u].zw);
  uint2 vec2_u32 = ub[2u].xy;
  float3 vec3_f32 = asfloat(ub[3u].xyz);
  int3 vec3_i32 = asint(ub[4u].xyz);
  uint3 vec3_u32 = ub[5u].xyz;
  float4 vec4_f32 = asfloat(ub[6u]);
  int4 vec4_i32 = asint(ub[7u]);
  uint4 vec4_u32 = ub[8u];
  float2x2 mat2x2_f32 = v_26(144u);
  float2x3 mat2x3_f32 = v_25(160u);
  float2x4 mat2x4_f32 = v_24(192u);
  float3x2 mat3x2_f32 = v_20(224u);
  float3x3 mat3x3_f32 = v_19(256u);
  float3x4 mat3x4_f32 = v_18(304u);
  float4x2 mat4x2_f32 = v_13(352u);
  float4x3 mat4x3_f32 = v_12(384u);
  float4x4 mat4x4_f32 = v_11(448u);
  float3 arr2_vec3_f32[2] = v_7(512u);
  Inner struct_inner = v(544u);
  Inner array_struct_inner[4] = v_2(576u);
  int v_29 = asint((asuint(tint_f32_to_i32(scalar_f32)) + asuint(scalar_i32)));
  int v_30 = asint((asuint(v_29) + asuint(int(scalar_u32))));
  int v_31 = asint((asuint(asint((asuint(v_30) + asuint(tint_f32_to_i32(vec2_f32.x))))) + asuint(vec2_i32.x)));
  int v_32 = asint((asuint(v_31) + asuint(int(vec2_u32.x))));
  int v_33 = asint((asuint(asint((asuint(v_32) + asuint(tint_f32_to_i32(vec3_f32.y))))) + asuint(vec3_i32.y)));
  int v_34 = asint((asuint(v_33) + asuint(int(vec3_u32.y))));
  int v_35 = asint((asuint(asint((asuint(v_34) + asuint(tint_f32_to_i32(vec4_f32.z))))) + asuint(vec4_i32.z)));
  int v_36 = asint((asuint(v_35) + asuint(int(vec4_u32.z))));
  int v_37 = asint((asuint(v_36) + asuint(tint_f32_to_i32(mat2x2_f32[0u].x))));
  int v_38 = asint((asuint(v_37) + asuint(tint_f32_to_i32(mat2x3_f32[0u].x))));
  int v_39 = asint((asuint(v_38) + asuint(tint_f32_to_i32(mat2x4_f32[0u].x))));
  int v_40 = asint((asuint(v_39) + asuint(tint_f32_to_i32(mat3x2_f32[0u].x))));
  int v_41 = asint((asuint(v_40) + asuint(tint_f32_to_i32(mat3x3_f32[0u].x))));
  int v_42 = asint((asuint(v_41) + asuint(tint_f32_to_i32(mat3x4_f32[0u].x))));
  int v_43 = asint((asuint(v_42) + asuint(tint_f32_to_i32(mat4x2_f32[0u].x))));
  int v_44 = asint((asuint(v_43) + asuint(tint_f32_to_i32(mat4x3_f32[0u].x))));
  int v_45 = asint((asuint(v_44) + asuint(tint_f32_to_i32(mat4x4_f32[0u].x))));
  s.Store(0u, asuint(asint((asuint(asint((asuint(asint((asuint(v_45) + asuint(tint_f32_to_i32(arr2_vec3_f32[0u].x))))) + asuint(struct_inner.scalar_i32)))) + asuint(array_struct_inner[0u].scalar_i32)))));
}

