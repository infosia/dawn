
cbuffer cbuffer_a : register(b0) {
  uint4 a[4];
};
RWByteAddressBuffer s : register(u1);
static int counter = int(0);
int i() {
  counter = asint((asuint(counter) + asuint(int(1))));
  return counter;
}

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

matrix<float16_t, 2, 4> v_2(uint start_byte_offset) {
  uint4 v_3 = a[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_4 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_3.zw) : (v_3.xy)));
  uint4 v_5 = a[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 4>(v_4, tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_5.zw) : (v_5.xy))));
}

typedef matrix<float16_t, 2, 4> ary_ret[4];
ary_ret v_6(uint start_byte_offset) {
  matrix<float16_t, 2, 4> a_1[4] = (matrix<float16_t, 2, 4>[4])0;
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      a_1[v_8] = v_2((start_byte_offset + (v_8 * 16u)));
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
  matrix<float16_t, 2, 4> v_9[4] = a_1;
  return v_9;
}

[numthreads(1, 1, 1)]
void f() {
  uint v_10 = (min(uint(i()), 3u) * 16u);
  uint v_11 = (min(uint(i()), 1u) * 8u);
  matrix<float16_t, 2, 4> l_a[4] = v_6(0u);
  matrix<float16_t, 2, 4> l_a_i = v_2(v_10);
  uint4 v_12 = a[((v_10 + v_11) / 16u)];
  vector<float16_t, 4> l_a_i_i = tint_bitcast_to_f16(((((((v_10 + v_11) & 15u) >> 2u) == 2u)) ? (v_12.zw) : (v_12.xy)));
  uint v_13 = a[((v_10 + v_11) / 16u)][(((v_10 + v_11) & 15u) >> 2u)];
  uint v_14 = (((((v_10 + v_11) % 4u) == 0u)) ? (0u) : (1u));
  s.Store<float16_t>(0u, (((tint_bitcast_to_f16_1(v_13)[v_14] + l_a[0u][0u].x) + l_a_i[0u].x) + l_a_i_i.x));
}

