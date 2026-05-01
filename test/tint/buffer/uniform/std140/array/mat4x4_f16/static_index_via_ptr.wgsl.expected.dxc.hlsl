
cbuffer cbuffer_a : register(b0) {
  uint4 a[8];
};
RWByteAddressBuffer s : register(u1);
vector<float16_t, 2> tint_bitcast_to_f16_1(uint src) {
  uint v = src;
  uint2 v_1 = uint2(v, v);
  vector<uint16_t, 2> v16 = vector<uint16_t, 2>(((v_1 >> uint2(0u, 16u)) & (65535u).xx));
  return asfloat16(v16);
}

vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 4, 4> v_2(uint start_byte_offset) {
  uint4 v_3 = a[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_4 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_3.zw, v_3.xy));
  uint4 v_5 = a[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_6 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy));
  uint4 v_7 = a[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_8 = tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_7.zw, v_7.xy));
  uint4 v_9 = a[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 4>(v_4, v_6, v_8, tint_bitcast_to_f16(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_9.zw, v_9.xy)));
}

typedef matrix<float16_t, 4, 4> ary_ret[4];
ary_ret v_10(uint start_byte_offset) {
  matrix<float16_t, 4, 4> a_1[4] = (matrix<float16_t, 4, 4>[4])0;
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      a_1[v_12] = v_2((start_byte_offset + (v_12 * 32u)));
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 4> v_13[4] = a_1;
  return v_13;
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 4, 4> l_a[4] = v_10(0u);
  matrix<float16_t, 4, 4> l_a_i = v_2(64u);
  vector<float16_t, 4> l_a_i_i = tint_bitcast_to_f16(a[4u].zw);
  s.Store<float16_t>(0u, (((tint_bitcast_to_f16_1(a[4u].z).x + l_a[0u][0u].x) + l_a_i[0u].x) + l_a_i_i.x));
}

