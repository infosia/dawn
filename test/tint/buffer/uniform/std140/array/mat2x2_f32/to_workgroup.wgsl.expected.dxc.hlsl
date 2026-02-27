struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
groupshared uint w[16];
void v(uint start_byte_offset, float2x2 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[((8u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((8u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
}

float2x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  float2 v_3 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_2.zw) : (v_2.xy)));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(v_3, asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_4.zw) : (v_4.xy))));
}

void v_5(uint start_byte_offset, float2x2 object[4]) {
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      v((start_byte_offset + (v_7 * 16u)), object[v_7]);
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
}

typedef float2x2 ary_ret[4];
ary_ret v_8(uint start_byte_offset) {
  float2x2 a[4] = (float2x2[4])0;
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      a[v_10] = v_1((start_byte_offset + (v_10 * 16u)));
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
  float2x2 v_11[4] = a;
  return v_11;
}

void f_inner(uint tint_local_index) {
  {
    uint v_12 = 0u;
    v_12 = tint_local_index;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      v((v_13 * 16u), float2x2((0.0f).xx, (0.0f).xx));
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  float2x2 v_14[4] = v_8(0u);
  v_5(0u, v_14);
  v(16u, v_1(32u));
  float2 v_15 = asfloat(u[0u].zw).yx;
  w[4u] = asuint(v_15.x);
  w[5u] = asuint(v_15.y);
  w[4u] = asuint(asfloat(u[0u].z));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

