
cbuffer cbuffer_data : register(b0) {
  uint4 data[3];
};
vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 4, 3> v_1(uint start_byte_offset) {
  uint4 v_2 = data[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_3 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_2.zw) : (v_2.xy))).xyz;
  uint4 v_4 = data[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_5 = tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_4.zw) : (v_4.xy))).xyz;
  uint4 v_6 = data[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_7 = tint_bitcast_to_f16(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_6.zw) : (v_6.xy))).xyz;
  uint4 v_8 = data[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_3, v_5, v_7, tint_bitcast_to_f16(((((((24u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_8.zw) : (v_8.xy))).xyz);
}

void main() {
  vector<float16_t, 3> v_9 = tint_bitcast_to_f16(data[2u].xy).xyz;
  vector<float16_t, 4> x = mul(v_1(0u), v_9);
}

