
cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
RWByteAddressBuffer s : register(u1);
void v(uint offset, float4x2 obj) {
  s.Store2((offset + 0u), asuint(obj[0u]));
  s.Store2((offset + 8u), asuint(obj[1u]));
  s.Store2((offset + 16u), asuint(obj[2u]));
  s.Store2((offset + 24u), asuint(obj[3u]));
}

float4x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  uint4 v_3 = u[((8u + start_byte_offset) / 16u)];
  uint4 v_4 = u[((16u + start_byte_offset) / 16u)];
  uint4 v_5 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy)));
}

void v_6(uint offset, float4x2 obj[4]) {
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      v((offset + (v_8 * 32u)), obj[v_8]);
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
}

typedef float4x2 ary_ret[4];
ary_ret v_9(uint start_byte_offset) {
  float4x2 a[4] = (float4x2[4])0;
  {
    uint v_10 = 0u;
    v_10 = 0u;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 4u)) {
        break;
      }
      a[v_11] = v_1((start_byte_offset + (v_11 * 32u)));
      {
        v_10 = (v_11 + 1u);
      }
    }
  }
  float4x2 v_12[4] = a;
  return v_12;
}

[numthreads(1, 1, 1)]
void f() {
  float4x2 v_13[4] = v_9(0u);
  v_6(0u, v_13);
  v(32u, v_1(64u));
  s.Store2(32u, asuint(asfloat(u[0u].zw).yx));
  s.Store(32u, asuint(asfloat(u[0u].z)));
}

