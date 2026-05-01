
cbuffer cbuffer_u : register(b0) {
  uint4 u[1];
};
static float2x2 p = float2x2((0.0f).xx, (0.0f).xx);
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

[numthreads(1, 1, 1)]
void f() {
  p = v(0u);
  p[1u] = asfloat(u[0u].xy);
  p[1u] = asfloat(u[0u].xy).yx;
  p[0u].y = asfloat(u[0u].z);
}

