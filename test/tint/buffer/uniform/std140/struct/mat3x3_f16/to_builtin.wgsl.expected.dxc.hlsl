
cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 3, 3> v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_3 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)).xyz;
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_5 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)).xyz;
  uint4 v_6 = u[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 3>(v_3, v_5, tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_6.zw, v_6.xy)).xyz);
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 3, 3> t = transpose(v_1(264u));
  float16_t l = length(tint_bitcast_to_f16(u[1u].xy).xyz.zxy);
  float16_t a = abs(tint_bitcast_to_f16(u[1u].xy).xyz.zxy.x);
}

