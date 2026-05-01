struct S {
  int before;
  matrix<float16_t, 3, 4> m;
  int after;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
void a(S a_1[4]) {
}

void b(S s) {
}

void c(matrix<float16_t, 3, 4> m) {
}

void d(vector<float16_t, 4> v) {
}

void e(float16_t f_1) {
}

vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 3, 4> v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_3 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_5 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy));
  uint4 v_6 = u[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_3, v_5, tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_6.zw, v_6.xy)));
}

S v_7(uint start_byte_offset) {
  int v_8 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 3, 4> v_9 = v_1((8u + start_byte_offset));
  S v_10 = {v_8, v_9, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_10;
}

typedef S ary_ret[4];
ary_ret v_11(uint start_byte_offset) {
  S a_2[4] = (S[4])0;
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      S v_14 = v_7((start_byte_offset + (v_13 * 128u)));
      a_2[v_13] = v_14;
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
  S v_15[4] = a_2;
  return v_15;
}

[numthreads(1, 1, 1)]
void f() {
  S v_16[4] = v_11(0u);
  a(v_16);
  S v_17 = v_7(256u);
  b(v_17);
  c(v_1(264u));
  d(tint_bitcast_to_f16(u[1u].xy).ywxz);
  e(tint_bitcast_to_f16(u[1u].xy).ywxz.x);
}

