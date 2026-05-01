
cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
RWByteAddressBuffer s : register(u1);
float16_t a(matrix<float16_t, 4, 4> a_1[4]) {
  return a_1[0u][0u].x;
}

float16_t b(matrix<float16_t, 4, 4> m) {
  return m[0u].x;
}

float16_t c(vector<float16_t, 4> v) {
  return v.x;
}

float16_t d(float16_t f_1) {
  return f_1;
}

vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 4, 4> v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_3 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_5 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy));
  uint4 v_6 = u[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_7 = tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_6.zw, v_6.xy));
  uint4 v_8 = u[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 4>(v_3, v_5, v_7, tint_bitcast_to_f16(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_8.zw, v_8.xy)));
}

typedef matrix<float16_t, 4, 4> ary_ret[4];
ary_ret v_9(uint start_byte_offset) {
  matrix<float16_t, 4, 4> a_2[4] = (matrix<float16_t, 4, 4>[4])0;
  {
    uint v_10 = 0u;
    v_10 = 0u;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 4u)) {
        break;
      }
      a_2[v_11] = v_1((start_byte_offset + (v_11 * 32u)));
      {
        v_10 = (v_11 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 4> v_12[4] = a_2;
  return v_12;
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 4, 4> v_13[4] = v_9(0u);
  float16_t v_14 = a(v_13);
  float16_t v_15 = (v_14 + b(v_1(32u)));
  float16_t v_16 = (v_15 + c(tint_bitcast_to_f16(u[2u].xy).ywxz));
  s.Store<float16_t>(0u, (v_16 + d(tint_bitcast_to_f16(u[2u].xy).ywxz.x)));
}

