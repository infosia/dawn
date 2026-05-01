
cbuffer cbuffer_u : register(b0) {
  uint4 u[8];
};
RWByteAddressBuffer s : register(u1);
vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  uint2 v_1 = uint2(v, v);
  vector<uint16_t, 2> v16 = vector<uint16_t, 2>(((v_1 >> uint2(0u, 16u)) & (65535u).xx));
  return asfloat16(v16);
}

vector<float16_t, 4> tint_bitcast_to_f16_1(uint2 src) {
  uint2 v = src;
  vector<uint16_t, 4> v16 = vector<uint16_t, 4>(((v.xxyy >> uint4(0u, 16u, 0u, 16u)) & (65535u).xxxx));
  return asfloat16(v16);
}

void v_2(uint offset, matrix<float16_t, 4, 3> obj) {
  s.Store<vector<float16_t, 3> >((offset + 0u), obj[0u]);
  s.Store<vector<float16_t, 3> >((offset + 8u), obj[1u]);
  s.Store<vector<float16_t, 3> >((offset + 16u), obj[2u]);
  s.Store<vector<float16_t, 3> >((offset + 24u), obj[3u]);
}

matrix<float16_t, 4, 3> v_3(uint start_byte_offset) {
  uint4 v_4 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_5 = tint_bitcast_to_f16_1(select((((start_byte_offset & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)).xyz;
  uint4 v_6 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_7 = tint_bitcast_to_f16_1(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_6.zw, v_6.xy)).xyz;
  uint4 v_8 = u[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_9 = tint_bitcast_to_f16_1(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_8.zw, v_8.xy)).xyz;
  uint4 v_10 = u[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_5, v_7, v_9, tint_bitcast_to_f16_1(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_10.zw, v_10.xy)).xyz);
}

void v_11(uint offset, matrix<float16_t, 4, 3> obj[4]) {
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      v_2((offset + (v_13 * 32u)), obj[v_13]);
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
}

typedef matrix<float16_t, 4, 3> ary_ret[4];
ary_ret v_14(uint start_byte_offset) {
  matrix<float16_t, 4, 3> a[4] = (matrix<float16_t, 4, 3>[4])0;
  {
    uint v_15 = 0u;
    v_15 = 0u;
    while(true) {
      uint v_16 = v_15;
      if ((v_16 >= 4u)) {
        break;
      }
      a[v_16] = v_3((start_byte_offset + (v_16 * 32u)));
      {
        v_15 = (v_16 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 3> v_17[4] = a;
  return v_17;
}

[numthreads(1, 1, 1)]
void f() {
  matrix<float16_t, 4, 3> v_18[4] = v_14(0u);
  v_11(0u, v_18);
  v_2(32u, v_3(64u));
  s.Store<vector<float16_t, 3> >(32u, tint_bitcast_to_f16_1(u[0u].zw).xyz.zxy);
  s.Store<float16_t>(32u, tint_bitcast_to_f16(u[0u].z).x);
}

