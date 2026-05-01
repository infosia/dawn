
cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
RWByteAddressBuffer s : register(u1);
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

[numthreads(1, 1, 1)]
void f() {
  float2x2 t = transpose(v(32u));
  float l = length(asfloat(u[0u].zw).yx);
  float a = abs(asfloat(u[0u].zw).yx.x);
  float v_3 = (t[0u].x + float(l));
  s.Store(0u, asuint((v_3 + float(a))));
}

