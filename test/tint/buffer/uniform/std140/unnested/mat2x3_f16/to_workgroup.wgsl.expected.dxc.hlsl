struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[1];
};
groupshared uint16_t w[8];
void v_1(uint start_byte_offset, matrix<float16_t, 2, 3> object) {
  w[(start_byte_offset / 2u)] = asuint16(object[0u].x);
  w[((start_byte_offset / 2u) + 1u)] = asuint16(object[0u].y);
  w[(((start_byte_offset / 2u) + 1u) + 1u)] = asuint16(object[0u].z);
  w[((8u + start_byte_offset) / 2u)] = asuint16(object[1u].x);
  w[(((8u + start_byte_offset) / 2u) + 1u)] = asuint16(object[1u].y);
  w[((((8u + start_byte_offset) / 2u) + 1u) + 1u)] = asuint16(object[1u].z);
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_2 = float16_t(t_low);
  return vector<float16_t, 2>(v_2, float16_t(t_high));
}

vector<float16_t, 4> tint_bitcast_to_f16_1(uint2 src) {
  uint2 v = src;
  uint2 mask = (65535u).xx;
  uint2 shift = (16u).xx;
  float2 t_low = f16tof32((v & mask));
  float2 t_high = f16tof32(((v >> shift) & mask));
  float16_t v_3 = float16_t(t_low.x);
  float16_t v_4 = float16_t(t_high.x);
  float16_t v_5 = float16_t(t_low.y);
  return vector<float16_t, 4>(v_3, v_4, v_5, float16_t(t_high.y));
}

matrix<float16_t, 2, 3> v_6(uint start_byte_offset) {
  uint4 v_7 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_8 = tint_bitcast_to_f16_1((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_7.zw) : (v_7.xy))).xyz;
  uint4 v_9 = u[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 3>(v_8, tint_bitcast_to_f16_1(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_9.zw) : (v_9.xy))).xyz);
}

void f_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v_1(0u, matrix<float16_t, 2, 3>((float16_t(0.0h)).xxx, (float16_t(0.0h)).xxx));
  }
  GroupMemoryBarrierWithGroupSync();
  v_1(0u, v_6(0u));
  vector<float16_t, 3> v_10 = tint_bitcast_to_f16_1(u[0u].xy).xyz;
  w[4u] = asuint16(v_10.x);
  w[5u] = asuint16(v_10.y);
  w[6u] = asuint16(v_10.z);
  vector<float16_t, 3> v_11 = tint_bitcast_to_f16_1(u[0u].xy).xyz.zxy;
  w[4u] = asuint16(v_11.x);
  w[5u] = asuint16(v_11.y);
  w[6u] = asuint16(v_11.z);
  w[1u] = asuint16(tint_bitcast_to_f16(u[0u].z).x);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

