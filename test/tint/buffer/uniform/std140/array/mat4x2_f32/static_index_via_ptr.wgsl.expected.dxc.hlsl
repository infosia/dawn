
cbuffer cbuffer_a : register(b0) {
  uint4 a[8];
};
RWByteAddressBuffer s : register(u1);
float4x2 v(uint start_byte_offset) {
  uint4 v_1 = a[(start_byte_offset / 16u)];
  uint4 v_2 = a[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = a[((16u + start_byte_offset) / 16u)];
  uint4 v_4 = a[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)));
}

typedef float4x2 ary_ret[4];
ary_ret v_5(uint start_byte_offset) {
  float4x2 a_1[4] = (float4x2[4])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      a_1[v_7] = v((start_byte_offset + (v_7 * 32u)));
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  float4x2 v_8[4] = a_1;
  return v_8;
}

[numthreads(1, 1, 1)]
void f() {
  float4x2 l_a[4] = v_5(0u);
  float4x2 l_a_i = v(64u);
  float2 l_a_i_i = asfloat(a[4u].zw);
  s.Store(0u, asuint((((asfloat(a[4u].z) + l_a[0u][0u].x) + l_a_i[0u].x) + l_a_i_i.x)));
}

