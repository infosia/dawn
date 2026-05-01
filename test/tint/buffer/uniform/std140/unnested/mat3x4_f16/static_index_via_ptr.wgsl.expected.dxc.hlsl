
cbuffer cbuffer_m : register(b0) {
  uint4 m[2];
};
vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 3, 4> v_1(uint start_byte_offset) {
  uint4 v_2 = m[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_3 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy));
  uint4 v_4 = m[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_5 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy));
  uint4 v_6 = m[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_3, v_5, tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_6.zw, v_6.xy)));
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 3, 4> l_m = v_1(0u);
  vector<float16_t, 4> l_m_1 = tint_bitcast_to_f16(m[0u].zw);
}

