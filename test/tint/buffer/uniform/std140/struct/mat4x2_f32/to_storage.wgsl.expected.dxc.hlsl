struct S {
  int before;
  float4x2 m;
  int after;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
RWByteAddressBuffer s : register(u1);
void v(uint offset, float4x2 obj) {
  s.Store2((offset + 0u), asuint(obj[0u]));
  s.Store2((offset + 8u), asuint(obj[1u]));
  s.Store2((offset + 16u), asuint(obj[2u]));
  s.Store2((offset + 24u), asuint(obj[3u]));
}

float4x2 v_1(uint start_byte_offset) {
  uint4 v_2 = u[(start_byte_offset / 16u)];
  uint4 v_3 = u[((8u + start_byte_offset) / 16u)];
  uint4 v_4 = u[((16u + start_byte_offset) / 16u)];
  uint4 v_5 = u[((24u + start_byte_offset) / 16u)];
  return float4x2(asfloat(select((((start_byte_offset & 15u) >> 2u) == 2u), v_2.zw, v_2.xy)), asfloat(select(((((8u + start_byte_offset) & 15u) >> 2u) == 2u), v_3.zw, v_3.xy)), asfloat(select(((((16u + start_byte_offset) & 15u) >> 2u) == 2u), v_4.zw, v_4.xy)), asfloat(select(((((24u + start_byte_offset) & 15u) >> 2u) == 2u), v_5.zw, v_5.xy)));
}

void v_6(uint offset, S obj) {
  s.Store((offset + 0u), asuint(obj.before));
  v((offset + 8u), obj.m);
  s.Store((offset + 64u), asuint(obj.after));
}

S v_7(uint start_byte_offset) {
  int v_8 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float4x2 v_9 = v_1((8u + start_byte_offset));
  S v_10 = {v_8, v_9, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_10;
}

void v_11(uint offset, S obj[4]) {
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      S v_14 = obj[v_13];
      v_6((offset + (v_13 * 128u)), v_14);
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_15(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_16 = 0u;
    v_16 = 0u;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      S v_18 = v_7((start_byte_offset + (v_17 * 128u)));
      a[v_17] = v_18;
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
  S v_19[4] = a;
  return v_19;
}

[numthreads(1, 1, 1)]
void f() {
  S v_20[4] = v_15(0u);
  v_11(0u, v_20);
  S v_21 = v_7(256u);
  v_6(128u, v_21);
  v(392u, v_1(264u));
  s.Store2(136u, asuint(asfloat(u[1u].xy).yx));
}

