
cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
RWByteAddressBuffer s : register(u1);
float4x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = u[((16u + start_byte_offset) / 16u)];
  uint4 v_4 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)));
}

[numthreads(1, 1, 1)]
void f() {
  float2x4 t = transpose(v(64u));
  float l = length(asfloat(u[0u].zw).yx);
  float a = abs(asfloat(u[0u].zw).yx.x);
  float v_5 = (t[0u].x + float(l));
  s.Store(0u, asuint((v_5 + float(a))));
}

