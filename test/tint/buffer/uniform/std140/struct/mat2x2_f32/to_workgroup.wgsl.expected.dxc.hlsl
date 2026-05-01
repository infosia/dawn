struct S {
  int before;
  float2x2 m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared S w[4];
float2x2 v(uint start_byte_offset) {
  uint4 v_1 = u[(start_byte_offset / 16u)];
  uint4 v_2 = u[((8u + start_byte_offset) / 16u)];
  return float2x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_1.zw, v_1.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)));
}

S v_3(uint start_byte_offset) {
  int v_4 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float2x2 v_5 = v((8u + start_byte_offset));
  S v_6 = {v_4, v_5, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_6;
}

typedef S ary_ret[4];
ary_ret v_7(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      S v_10 = v_3((start_byte_offset + (v_9 * 128u)));
      a[v_9] = v_10;
      {
        v_8 = (v_9 + 1u);
      }
    }
  }
  S v_11[4] = a;
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
      S v_14 = (S)0;
      w[v_13] = v_14;
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_15[4] = v_7(0u);
  w = v_15;
  S v_16 = v_3(256u);
  w[1u] = v_16;
  w[3u].m = v(264u);
  w[1u].m[0u] = asfloat(u[1u].xy).yx;
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

