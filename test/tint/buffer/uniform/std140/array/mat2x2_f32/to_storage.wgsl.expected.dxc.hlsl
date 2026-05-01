
cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
RWByteAddressBuffer s : register(u1);
void v(uint offset, float2x2 obj) {
  s.Store2((offset + 0u), asuint(obj[0u]));
  s.Store2((offset + 8u), asuint(obj[1u]));
}

float2x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  uint4 v_3 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

void v_4(uint offset, float2x2 obj[4]) {
  {
    uint v_5 = 0u;
    v_5 = 0u;
    while(true) {
      uint v_6 = v_5;
      if ((v_6 >= 4u)) {
        break;
      }
      v((offset + (v_6 * 16u)), obj[v_6]);
      {
        v_5 = (v_6 + 1u);
      }
    }
  }
}

typedef float2x2 ary_ret[4];
ary_ret v_7(uint start_byte_offset) {
  float2x2 a[4] = (float2x2[4])0;
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      a[v_9] = v_1((start_byte_offset + (v_9 * 16u)));
      {
        v_8 = (v_9 + 1u);
      }
    }
  }
  float2x2 v_10[4] = a;
  return v_10;
}

[numthreads(1, 1, 1)]
void f() {
  float2x2 v_11[4] = v_7(0u);
  v_4(0u, v_11);
  v(16u, v_1(32u));
  s.Store2(16u, asuint(asfloat(u[0u].zw).yx));
  s.Store(16u, asuint(asfloat(u[0u].z)));
}

