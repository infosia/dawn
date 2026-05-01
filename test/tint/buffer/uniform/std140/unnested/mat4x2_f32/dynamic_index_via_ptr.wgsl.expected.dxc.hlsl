
cbuffer cbuffer_m : register(b0) {
  uint4 m[2];
};
static int counter = int(0);
int i() {
  counter = asint((asuint(counter) + asuint(int(1))));
  return counter;
}

float4x2 v(uint start_byte_offset) {
  uint4 v_1 = m[(start_byte_offset / 16u)];
  uint4 v_2 = m[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = m[((16u + start_byte_offset) / 16u)];
  uint4 v_4 = m[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)));
}

[numthreads(1, 1, 1)]
void f() {
  uint v_5 = (min(uint(i()), 3u) * 8u);
  float4x2 l_m = v(0u);
  uint4 v_6 = m[(v_5 / 16u)];
  float2 l_m_i = asfloat(select((((v_5 & 15u) >> 2u) == 2u), v_6.zw, v_6.xy));
}

