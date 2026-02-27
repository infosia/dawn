struct S {
  int before;
  float4x2 m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint w[128];
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

void v_1(uint start_byte_offset, S object) {
  w[(start_byte_offset / 4u)] = asuint(object.before);
  v((8u + start_byte_offset), object.m);
  w[((64u + start_byte_offset) / 4u)] = asuint(object.after);
}

float4x2 v_2(uint start_byte_offset) {
  uint4 v_3 = u[(start_byte_offset / 16u)];
  float2 v_4 = asfloat((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_3.zw) : (v_3.xy)));
  uint4 v_5 = u[((8u + start_byte_offset) / 16u)];
  float2 v_6 = asfloat(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_5.zw) : (v_5.xy)));
  uint4 v_7 = u[((16u + start_byte_offset) / 16u)];
  float2 v_8 = asfloat(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_7.zw) : (v_7.xy)));
  uint4 v_9 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(v_4, v_6, v_8, asfloat(((((((24u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_9.zw) : (v_9.xy))));
}

S v_10(uint start_byte_offset) {
  int v_11 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float4x2 v_12 = v_2((8u + start_byte_offset));
  S v_13 = {v_11, v_12, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_13;
}

void v_14(uint start_byte_offset, S object[4]) {
  {
    uint v_15 = 0u;
    v_15 = 0u;
    while(true) {
      uint v_16 = v_15;
      if ((v_16 >= 4u)) {
        break;
      }
      S v_17 = object[v_16];
      v_1((start_byte_offset + (v_16 * 128u)), v_17);
      {
        v_15 = (v_16 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_18(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_19 = 0u;
    v_19 = 0u;
    while(true) {
      uint v_20 = v_19;
      if ((v_20 >= 4u)) {
        break;
      }
      S v_21 = v_10((start_byte_offset + (v_20 * 128u)));
      a[v_20] = v_21;
      {
        v_19 = (v_20 + 1u);
      }
    }
  }
  S v_22[4] = a;
  return v_22;
}

void f_inner(uint tint_local_index) {
  {
    uint v_23 = 0u;
    v_23 = tint_local_index;
    while(true) {
      uint v_24 = v_23;
      if ((v_24 >= 4u)) {
        break;
      }
      S v_25 = (S)0;
      v_1((v_24 * 128u), v_25);
      {
        v_23 = (v_24 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_26[4] = v_18(0u);
  v_14(0u, v_26);
  S v_27 = v_10(256u);
  v_1(128u, v_27);
  v(392u, v_2(264u));
  float2 v_28 = asfloat(u[1u].xy).yx;
  w[34u] = asuint(v_28.x);
  w[35u] = asuint(v_28.y);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

