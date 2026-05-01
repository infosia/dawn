struct main_outputs {
  float4 tint_symbol : SV_Position;
};

struct main_inputs {
  uint gl_VertexIndex : SV_VertexID;
};


cbuffer cbuffer_x_20 : register(b0) {
  uint4 x_20[1];
};
cbuffer cbuffer_x_26 : register(b0, space1) {
  uint4 x_26[1];
};
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = x_26[(start_byte_offset / 16u)];
  uint4 v_2 = x_26[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

float2x2 v_3(uint start_byte_offset) {
  uint4 v_4 = x_20[(start_byte_offset / 16u)];
  uint4 v_5 = x_20[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy)));
}

float4 main_inner(uint gl_VertexIndex) {
  float2 indexable[3] = (float2[3])0;
  float2x2 x_23 = v_3(0u);
  float2x2 x_28 = v(0u);
  uint x_46 = gl_VertexIndex;
  float2 v_6[3] = {float2(-1.0f, 1.0f), (1.0f).xx, (-1.0f).xx};
  indexable = v_6;
  float2 x_51 = indexable[min(x_46, 2u)];
  float2 x_52 = mul(x_51, float2x2((x_23[0u] + x_28[0u]), (x_23[1u] + x_28[1u])));
  return float4(x_52.x, x_52.y, 0.0f, 1.0f);
}

main_outputs main(main_inputs inputs) {
  main_outputs v_7 = {main_inner(inputs.gl_VertexIndex)};
  return v_7;
}

