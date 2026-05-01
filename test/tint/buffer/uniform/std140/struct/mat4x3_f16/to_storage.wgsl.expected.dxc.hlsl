struct S {
  int before;
  matrix<float16_t, 4, 3> m;
  int after;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
RWByteAddressBuffer s : register(u1);
vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

void v_1(uint offset, matrix<float16_t, 4, 3> obj) {
  s.Store<vector<float16_t, 3> >((offset + 0u), obj[0u]);
  s.Store<vector<float16_t, 3> >((offset + 8u), obj[1u]);
  s.Store<vector<float16_t, 3> >((offset + 16u), obj[2u]);
  s.Store<vector<float16_t, 3> >((offset + 24u), obj[3u]);
}

matrix<float16_t, 4, 3> v_2(uint start_byte_offset) {
  uint4 v_3 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_4 = tint_bitcast_to_f16(select((((start_byte_offset & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)).xyz;
  uint4 v_5 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_6 = tint_bitcast_to_f16(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy)).xyz;
  uint4 v_7 = u[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_8 = tint_bitcast_to_f16(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_7.zw, v_7.xy)).xyz;
  uint4 v_9 = u[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_4, v_6, v_8, tint_bitcast_to_f16(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_9.zw, v_9.xy)).xyz);
}

void v_10(uint offset, S obj) {
  s.Store((offset + 0u), asuint(obj.before));
  v_1((offset + 8u), obj.m);
  s.Store((offset + 64u), asuint(obj.after));
}

S v_11(uint start_byte_offset) {
  int v_12 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 4, 3> v_13 = v_2((8u + start_byte_offset));
  S v_14 = {v_12, v_13, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_14;
}

void v_15(uint offset, S obj[4]) {
  {
    uint v_16 = 0u;
    v_16 = 0u;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      S v_18 = obj[v_17];
      v_10((offset + (v_17 * 128u)), v_18);
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_19(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_20 = 0u;
    v_20 = 0u;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 4u)) {
        break;
      }
      S v_22 = v_11((start_byte_offset + (v_21 * 128u)));
      a[v_21] = v_22;
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
  S v_23[4] = a;
  return v_23;
}

[numthreads(1, 1, 1)]
void f() {
  S v_24[4] = v_19(0u);
  v_15(0u, v_24);
  S v_25 = v_11(256u);
  v_10(128u, v_25);
  v_1(392u, v_2(264u));
  s.Store<vector<float16_t, 3> >(136u, tint_bitcast_to_f16(u[1u].xy).xyz.zxy);
}

