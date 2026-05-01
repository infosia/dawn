
cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
RWByteAddressBuffer s : register(u1);
float a(float4x2 a_1[4]) {
  return a_1[0u][0u].x;
}

float b(float4x2 m) {
  return m[0u].x;
}

float c(float2 v) {
  return v.x;
}

float d(float f_1) {
  return f_1;
}

float4x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  uint4 v_3 = u[((8u + start_byte_offset) / 16u)];
  uint4 v_4 = u[((16u + start_byte_offset) / 16u)];
  uint4 v_5 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy)));
}

typedef float4x2 ary_ret[4];
ary_ret v_6(uint start_byte_offset) {
  float4x2 a_2[4] = (float4x2[4])0;
  {
    uint v_7 = 0u;
    v_7 = 0u;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      a_2[v_8] = v_1((start_byte_offset + (v_8 * 32u)));
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
  float4x2 v_9[4] = a_2;
  return v_9;
}

[numthreads(1, 1, 1)]
void f() {
  float4x2 v_10[4] = v_6(0u);
  float v_11 = a(v_10);
  float v_12 = (v_11 + b(v_1(32u)));
  float v_13 = (v_12 + c(asfloat(u[2u].xy).yx));
  s.Store(0u, asuint((v_13 + d(asfloat(u[2u].xy).yx.x))));
}

