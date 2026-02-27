struct S {
  int before;
  float3x2 m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint w[128];
void v(uint start_byte_offset, float3x2 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[((8u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((8u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
}

void v_1(uint start_byte_offset, S object) {
  w[(start_byte_offset / 4u)] = asuint(object.before);
  v((8u + start_byte_offset), object.m);
  w[((64u + start_byte_offset) / 4u)] = asuint(object.after);
}

float3x2 v_2(uint start_byte_offset) {
  uint4 v_3 = u[(start_byte_offset / 16u)];
  float2 v_4 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_3.zw) : (v_3.xy)));
  uint4 v_5 = u[((8u + start_byte_offset) / 16u)];
  float2 v_6 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_5.zw) : (v_5.xy)));
  uint4 v_7 = u[((16u + start_byte_offset) / 16u)];
  return float3x2(v_4, v_6, asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_7.zw) : (v_7.xy))));
}

S v_8(uint start_byte_offset) {
  int v_9 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float3x2 v_10 = v_2((8u + start_byte_offset));
  S v_11 = {v_9, v_10, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_11;
}

void v_12(uint start_byte_offset, S object[4]) {
  {
    uint v_13 = 0u;
    v_13 = 0u;
    while(true) {
      uint v_14 = v_13;
      if ((v_14 >= 4u)) {
        break;
      }
      S v_15 = object[v_14];
      v_1((start_byte_offset + (v_14 * 128u)), v_15);
      {
        v_13 = (v_14 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_16(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_17 = 0u;
    v_17 = 0u;
    while(true) {
      uint v_18 = v_17;
      if ((v_18 >= 4u)) {
        break;
      }
      S v_19 = v_8((start_byte_offset + (v_18 * 128u)));
      a[v_18] = v_19;
      {
        v_17 = (v_18 + 1u);
      }
    }
  }
  S v_20[4] = a;
  return v_20;
}

void f_inner(uint tint_local_index) {
  {
    uint v_21 = 0u;
    v_21 = tint_local_index;
    while(true) {
      uint v_22 = v_21;
      if ((v_22 >= 4u)) {
        break;
      }
      S v_23 = (S)0;
      v_1((v_22 * 128u), v_23);
      {
        v_21 = (v_22 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_24[4] = v_16(0u);
  v_12(0u, v_24);
  S v_25 = v_8(256u);
  v_1(128u, v_25);
  v(392u, v_2(264u));
  float2 v_26 = asfloat(u[1u].xy).yx;
  w[34u] = asuint(v_26.x);
  w[35u] = asuint(v_26.y);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

