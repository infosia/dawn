struct Inner {
  float2x2 m;
};

struct Outer {
  Inner a[4];
};


cbuffer cbuffer_a : register(b0) {
  uint4 a[64];
};
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = a[(start_byte_offset / 16u)];
  uint4 v_2 = a[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

Inner v_3(uint start_byte_offset) {
  Inner v_4 = {v(start_byte_offset)};
  return v_4;
}

typedef Inner ary_ret[4];
ary_ret v_5(uint start_byte_offset) {
  Inner a_2[4] = (Inner[4])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      Inner v_8 = v_3((start_byte_offset + (v_7 * 64u)));
      a_2[v_7] = v_8;
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  Inner v_9[4] = a_2;
  return v_9;
}

Outer v_10(uint start_byte_offset) {
  Inner v_11[4] = v_5(start_byte_offset);
  Outer v_12 = {v_11};
  return v_12;
}

typedef Outer ary_ret_1[4];
ary_ret_1 v_13(uint start_byte_offset) {
  Outer a_1[4] = (Outer[4])0;
  {
    uint v_14 = 0u;
    v_14 = 0u;
    while(true) {
      uint v_15 = v_14;
      if ((v_15 >= 4u)) {
        break;
      }
      Outer v_16 = v_10((start_byte_offset + (v_15 * 256u)));
      a_1[v_15] = v_16;
      {
        v_14 = (v_15 + 1u);
      }
    }
  }
  Outer v_17[4] = a_1;
  return v_17;
}

[numthreads(1, 1, 1)]
void f() {
  Outer l_a[4] = v_13(0u);
  Outer l_a_3 = v_10(768u);
  Inner l_a_3_a[4] = v_5(768u);
  Inner l_a_3_a_2 = v_3(896u);
  float2x2 l_a_3_a_2_m = v(896u);
  float2 l_a_3_a_2_m_1 = asfloat(a[56u].zw);
  float l_a_3_a_2_m_1_0 = asfloat(a[56u].z);
}

