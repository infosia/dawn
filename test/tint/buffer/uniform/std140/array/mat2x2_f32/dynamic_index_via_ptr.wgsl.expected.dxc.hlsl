
cbuffer cbuffer_a : register(b0) {
  uint4 a[4];
};
RWByteAddressBuffer s : register(u1);
static int counter = int(0);
int i() {
  counter = asint((asuint(counter) + asuint(int(1))));
  return counter;
}

float2x2 v(uint start_byte_offset) {
  uint4 v_1 = a[(start_byte_offset / 16u)];
  uint4 v_2 = a[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

typedef float2x2 ary_ret[4];
ary_ret v_3(uint start_byte_offset) {
  float2x2 a_1[4] = (float2x2[4])0;
  {
    uint v_4 = 0u;
    v_4 = 0u;
    while(true) {
      uint v_5 = v_4;
      if ((v_5 >= 4u)) {
        break;
      }
      a_1[v_5] = v((start_byte_offset + (v_5 * 16u)));
      {
        v_4 = (v_5 + 1u);
      }
    }
  }
  float2x2 v_6[4] = a_1;
  return v_6;
}

[numthreads(1, 1, 1)]
void f() {
  uint v_7 = (min(uint(i()), 3u) * 16u);
  uint v_8 = (min(uint(i()), 1u) * 8u);
  float2x2 l_a[4] = v_3(0u);
  float2x2 l_a_i = v(v_7);
  uint4 v_9 = a[((v_7 + v_8) / 16u)];
  float2 l_a_i_i = asfloat(select(((((v_7 + v_8) & 15u) >> 2u) == 2u), v_9.zw, v_9.xy));
  s.Store(0u, asuint((((asfloat(a[((v_7 + v_8) / 16u)][(((v_7 + v_8) & 15u) >> 2u)]) + l_a[0u][0u].x) + l_a_i[0u].x) + l_a_i_i.x)));
}

