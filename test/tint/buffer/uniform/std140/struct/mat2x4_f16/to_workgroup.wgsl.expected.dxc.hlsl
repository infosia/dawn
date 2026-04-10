struct S {
  int before;
  matrix<float16_t, 2, 4> m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared S w[4];
vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

matrix<float16_t, 2, 4> v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_3 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_2.zw) : (v_2.xy)));
  uint4 v_4 = u[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 4>(v_3, tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_4.zw) : (v_4.xy))));
}

S v_5(uint start_byte_offset) {
  int v_6 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 2, 4> v_7 = v_1((8u + start_byte_offset));
  S v_8 = {v_6, v_7, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_8;
}

typedef S ary_ret[4];
ary_ret v_9(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_10 = 0u;
    v_10 = 0u;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 4u)) {
        break;
      }
      S v_12 = v_5((start_byte_offset + (v_11 * 128u)));
      a[v_11] = v_12;
      {
        v_10 = (v_11 + 1u);
      }
    }
  }
  S v_13[4] = a;
  return v_13;
}

void f_inner(uint tint_local_index) {
  {
    uint v_14 = 0u;
    v_14 = tint_local_index;
    while(true) {
      uint v_15 = v_14;
      if ((v_15 >= 4u)) {
        break;
      }
      S v_16 = (S)0;
      w[v_15] = v_16;
      {
        v_14 = (v_15 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_17[4] = v_9(0u);
  w = v_17;
  S v_18 = v_5(256u);
  w[1u] = v_18;
  w[3u].m = v_1(264u);
  w[1u].m[0u] = tint_bitcast_to_f16(u[1u].xy).ywxz;
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

