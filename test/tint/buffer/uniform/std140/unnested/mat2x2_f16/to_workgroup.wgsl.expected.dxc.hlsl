struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[1];
};
groupshared uint16_t w[4];
void v_1(uint start_byte_offset, matrix<float16_t, 2, 2> object) {
  w[(start_byte_offset / 2u)] = asuint16(object[0u].x);
  w[((start_byte_offset / 2u) + 1u)] = asuint16(object[0u].y);
  w[((4u + start_byte_offset) / 2u)] = asuint16(object[1u].x);
  w[(((4u + start_byte_offset) / 2u) + 1u)] = asuint16(object[1u].y);
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_2 = float16_t(t_low);
  return vector<float16_t, 2>(v_2, float16_t(t_high));
}

matrix<float16_t, 2, 2> v_3(uint start_byte_offset) {
  vector<float16_t, 2> v_4 = tint_bitcast_to_f16(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  return matrix<float16_t, 2, 2>(v_4, tint_bitcast_to_f16(u[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]));
}

void f_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v_1(0u, matrix<float16_t, 2, 2>((float16_t(0.0h)).xx, (float16_t(0.0h)).xx));
  }
  GroupMemoryBarrierWithGroupSync();
  v_1(0u, v_3(0u));
  vector<float16_t, 2> v_5 = tint_bitcast_to_f16(u[0u].x);
  w[2u] = asuint16(v_5.x);
  w[3u] = asuint16(v_5.y);
  vector<float16_t, 2> v_6 = tint_bitcast_to_f16(u[0u].x).yx;
  w[2u] = asuint16(v_6.x);
  w[3u] = asuint16(v_6.y);
  w[1u] = asuint16(tint_bitcast_to_f16(u[0u].y).x);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

