struct S {
  int before;
  matrix<float16_t, 3, 4> m;
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

void v_1(uint offset, matrix<float16_t, 3, 4> obj) {
  s.Store<vector<float16_t, 4> >((offset + 0u), obj[0u]);
  s.Store<vector<float16_t, 4> >((offset + 8u), obj[1u]);
  s.Store<vector<float16_t, 4> >((offset + 16u), obj[2u]);
}

matrix<float16_t, 3, 4> v_2(uint start_byte_offset) {
  uint4 v_3 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_4 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_3.zw) : (v_3.xy)));
  uint4 v_5 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_6 = tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_5.zw) : (v_5.xy)));
  uint4 v_7 = u[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_4, v_6, tint_bitcast_to_f16(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_7.zw) : (v_7.xy))));
}

void v_8(uint offset, S obj) {
  s.Store((offset + 0u), asuint(obj.before));
  v_1((offset + 8u), obj.m);
  s.Store((offset + 64u), asuint(obj.after));
}

S v_9(uint start_byte_offset) {
  int v_10 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 3, 4> v_11 = v_2((8u + start_byte_offset));
  S v_12 = {v_10, v_11, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_12;
}

void v_13(uint offset, S obj[4]) {
  {
    uint v_14 = 0u;
    v_14 = 0u;
    while(true) {
      uint v_15 = v_14;
      if ((v_15 >= 4u)) {
        break;
      }
      S v_16 = obj[v_15];
      v_8((offset + (v_15 * 128u)), v_16);
      {
        v_14 = (v_15 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_17(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_18 = 0u;
    v_18 = 0u;
    while(true) {
      uint v_19 = v_18;
      if ((v_19 >= 4u)) {
        break;
      }
      S v_20 = v_9((start_byte_offset + (v_19 * 128u)));
      a[v_19] = v_20;
      {
        v_18 = (v_19 + 1u);
      }
    }
  }
  S v_21[4] = a;
  return v_21;
}

[numthreads(1, 1, 1)]
void f() {
  S v_22[4] = v_17(0u);
  v_13(0u, v_22);
  S v_23 = v_9(256u);
  v_8(128u, v_23);
  v_1(392u, v_2(264u));
  s.Store<vector<float16_t, 4> >(136u, tint_bitcast_to_f16(u[1u].xy).ywxz);
}

