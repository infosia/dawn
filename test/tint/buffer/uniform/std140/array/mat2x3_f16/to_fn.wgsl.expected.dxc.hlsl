
cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
RWByteAddressBuffer s : register(u1);
float16_t a(matrix<float16_t, 2, 3> a_1[4]) {
  return a_1[0u][0u].x;
}

float16_t b(matrix<float16_t, 2, 3> m) {
  return m[0u].x;
}

float16_t c(vector<float16_t, 3> v) {
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

matrix<float16_t, 2, 3> v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_3 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)).xyz;
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 3>(v_3, tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)).xyz);
}

typedef matrix<float16_t, 2, 3> ary_ret[4];
ary_ret v_5(uint start_byte_offset) {
  matrix<float16_t, 2, 3> a_2[4] = (matrix<float16_t, 2, 3>[4])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      a_2[v_7] = v_1((start_byte_offset + (v_7 * 16u)));
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  matrix<float16_t, 2, 3> v_8[4] = a_2;
  return v_8;
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 2, 3> v_9[4] = v_5(0u);
  float16_t v_10 = a(v_9);
  float16_t v_11 = (v_10 + b(v_1(16u)));
  float16_t v_12 = (v_11 + c(tint_bitcast_to_f16(u[1u].xy).xyz.zxy));
  s.Store<float16_t>(0u, (v_12 + d(tint_bitcast_to_f16(u[1u].xy).xyz.zxy.x)));
}

