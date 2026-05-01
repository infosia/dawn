struct S {
  int before;
  float2x2 m;
  int after;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
void a(S a_1[4]) {
}

void b(S s) {
}

void c(float2x2 m) {
}

void d(float2 v) {
}

void e(float f_1) {
}

float2x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  uint4 v_3 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

S v_4(uint start_byte_offset) {
  int v_5 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float2x2 v_6 = v_1((8u + start_byte_offset));
  S v_7 = {v_5, v_6, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_7;
}

typedef S ary_ret[4];
ary_ret v_8(uint start_byte_offset) {
  S a_2[4] = (S[4])0;
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      S v_11 = v_4((start_byte_offset + (v_10 * 128u)));
      a_2[v_10] = v_11;
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
  S v_12[4] = a_2;
  return v_12;
}

[numthreads(1, 1, 1)]
void f() {
  S v_13[4] = v_8(0u);
  a(v_13);
  S v_14 = v_4(256u);
  b(v_14);
  c(v_1(264u));
  d(asfloat(u[1u].xy).yx);
  e(asfloat(u[1u].xy).yx.x);
}

