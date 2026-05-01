struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
groupshared float4x2 w[4];
float4x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  uint4 v_3 = u[((16u + start_byte_offset) / 16u)];
  uint4 v_4 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)));
}

typedef float4x2 ary_ret[4];
ary_ret v_5(uint start_byte_offset) {
  float4x2 a[4] = (float4x2[4])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      a[v_7] = v((start_byte_offset + (v_7 * 32u)));
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  float4x2 v_8[4] = a;
  return v_8;
}

void f_inner(uint tint_local_index) {
  {
    uint v_9 = 0u;
    v_9 = tint_local_index;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      w[v_10] = float4x2((0.0f).xx, (0.0f).xx, (0.0f).xx, (0.0f).xx);
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  float4x2 v_11[4] = v_5(0u);
  w = v_11;
  w[1u] = v(64u);
  w[1u][0u] = asfloat(u[0u].zw).yx;
  w[1u][0u].x = asfloat(u[0u].z);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

