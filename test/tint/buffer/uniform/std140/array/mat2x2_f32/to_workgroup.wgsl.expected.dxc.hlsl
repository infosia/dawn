struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
groupshared float2x2 w[4];
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

typedef float2x2 ary_ret[4];
ary_ret v_3(uint start_byte_offset) {
  float2x2 a[4] = (float2x2[4])0;
  {
    uint v_4 = 0u;
    v_4 = 0u;
    while(true) {
      uint v_5 = v_4;
      if ((v_5 >= 4u)) {
        break;
      }
      a[v_5] = v((start_byte_offset + (v_5 * 16u)));
      {
        v_4 = (v_5 + 1u);
      }
    }
  }
  float2x2 v_6[4] = a;
  return v_6;
}

void f_inner(uint tint_local_index) {
  {
    uint v_7 = 0u;
    v_7 = tint_local_index;
    while(true) {
      uint v_8 = v_7;
      if ((v_8 >= 4u)) {
        break;
      }
      w[v_8] = float2x2((0.0f).xx, (0.0f).xx);
      {
        v_7 = (v_8 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  float2x2 v_9[4] = v_3(0u);
  w = v_9;
  w[1u] = v(32u);
  w[1u][0u] = asfloat(u[0u].zw).yx;
  w[1u][0u].x = asfloat(u[0u].z);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

