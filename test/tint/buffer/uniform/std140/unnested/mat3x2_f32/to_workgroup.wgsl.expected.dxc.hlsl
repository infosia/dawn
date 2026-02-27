struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[2];
};
groupshared uint w[6];
void v(uint start_byte_offset, float3x2 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[((8u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((8u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
}

float3x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  float2 v_3 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_2.zw) : (v_2.xy)));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  float2 v_5 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_4.zw) : (v_4.xy)));
  uint4 v_6 = u[((16u + start_byte_offset) / 16u)];
  return float3x2(v_3, v_5, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_6.zw) : (v_6.xy))));
}

void f_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v(0u, float3x2((0.0f).xx, (0.0f).xx, (0.0f).xx));
  }
  GroupMemoryBarrierWithGroupSync();
  v(0u, v_1(0u));
  float2 v_7 = asfloat(u[0u].xy);
  w[2u] = asuint(v_7.x);
  w[3u] = asuint(v_7.y);
  float2 v_8 = asfloat(u[0u].xy).yx;
  w[2u] = asuint(v_8.x);
  w[3u] = asuint(v_8.y);
  w[1u] = asuint(asfloat(u[0u].z));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

