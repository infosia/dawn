
cbuffer cbuffer_data : register(b0) {
  uint4 data[3];
};
float3x2 v(uint start_byte_offset) {
  uint4 v_1 = data[(start_byte_offset / 16u)];
  uint4 v_2 = data[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = data[((16u + start_byte_offset) / 16u)];
  return float3x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)));
}

void main() {
  float3x2 v_4 = v(0u);
  float2 x = mul(asfloat(data[2u].xyz), v_4);
}

