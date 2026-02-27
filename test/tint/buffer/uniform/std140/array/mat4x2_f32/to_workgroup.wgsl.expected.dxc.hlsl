struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
groupshared uint w[32];
void v(uint start_byte_offset, float4x2 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[((8u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((8u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  w[((24u + start_byte_offset) / 4u)] = asuint(object[3u].x);
  w[(((24u + start_byte_offset) / 4u) + 1u)] = asuint(object[3u].y);
}

float4x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  float2 v_3 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_2.zw) : (v_2.xy)));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  float2 v_5 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_4.zw) : (v_4.xy)));
  uint4 v_6 = u[((16u + start_byte_offset) / 16u)];
  float2 v_7 = asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_6.zw) : (v_6.xy)));
  uint4 v_8 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(v_3, v_5, v_7, asfloat(((((((24u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_8.zw) : (v_8.xy))));
}

void v_9(uint start_byte_offset, float4x2 object[4]) {
  {
    uint v_10 = 0u;
    v_10 = 0u;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 4u)) {
        break;
      }
      v((start_byte_offset + (v_11 * 32u)), object[v_11]);
      {
        v_10 = (v_11 + 1u);
      }
    }
  }
}

typedef float4x2 ary_ret[4];
ary_ret v_12(uint start_byte_offset) {
  float4x2 a[4] = (float4x2[4])0;
  {
    uint v_13 = 0u;
    v_13 = 0u;
    while(true) {
      uint v_14 = v_13;
      if ((v_14 >= 4u)) {
        break;
      }
      a[v_14] = v_1((start_byte_offset + (v_14 * 32u)));
      {
        v_13 = (v_14 + 1u);
      }
    }
  }
  float4x2 v_15[4] = a;
  return v_15;
}

void f_inner(uint tint_local_index) {
  {
    uint v_16 = 0u;
    v_16 = tint_local_index;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      v((v_17 * 32u), float4x2((0.0f).xx, (0.0f).xx, (0.0f).xx, (0.0f).xx));
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  float4x2 v_18[4] = v_12(0u);
  v_9(0u, v_18);
  v(32u, v_1(64u));
  float2 v_19 = asfloat(u[0u].zw).yx;
  w[8u] = asuint(v_19.x);
  w[9u] = asuint(v_19.y);
  w[8u] = asuint(asfloat(u[0u].z));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

