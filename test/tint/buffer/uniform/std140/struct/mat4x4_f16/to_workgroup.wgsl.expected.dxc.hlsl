struct S {
  int before;
  matrix<float16_t, 4, 4> m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint w[128];
uint2 tint_bitcast_from_f16(vector<float16_t, 4> src) {
  uint4 r = f32tof16(float4(src));
  return uint2(((r.x & 65535u) | ((r.y & 65535u) << 16u)), ((r.z & 65535u) | ((r.w & 65535u) << 16u)));
}

void v_1(uint start_byte_offset, matrix<float16_t, 4, 4> object) {
  uint2 v_2 = tint_bitcast_from_f16(object[0u]);
  w[(start_byte_offset / 4u)] = v_2.x;
  w[((start_byte_offset / 4u) + 1u)] = v_2.y;
  uint2 v_3 = tint_bitcast_from_f16(object[1u]);
  w[((8u + start_byte_offset) / 4u)] = v_3.x;
  w[(((8u + start_byte_offset) / 4u) + 1u)] = v_3.y;
  uint2 v_4 = tint_bitcast_from_f16(object[2u]);
  w[((16u + start_byte_offset) / 4u)] = v_4.x;
  w[(((16u + start_byte_offset) / 4u) + 1u)] = v_4.y;
  uint2 v_5 = tint_bitcast_from_f16(object[3u]);
  w[((24u + start_byte_offset) / 4u)] = v_5.x;
  w[(((24u + start_byte_offset) / 4u) + 1u)] = v_5.y;
}

void v_6(uint start_byte_offset, S object) {
  w[(start_byte_offset / 4u)] = asuint(object.before);
  v_1((8u + start_byte_offset), object.m);
  w[((64u + start_byte_offset) / 4u)] = asuint(object.after);
}

vector<float16_t, 4> tint_bitcast_to_f16(uint2 src) {
  uint2 v = src;
  uint2 mask = (65535u).xx;
  uint2 shift = (16u).xx;
  float2 t_low = f16tof32((v & mask));
  float2 t_high = f16tof32(((v >> shift) & mask));
  float16_t v_7 = float16_t(t_low.x);
  float16_t v_8 = float16_t(t_high.x);
  float16_t v_9 = float16_t(t_low.y);
  return vector<float16_t, 4>(v_7, v_8, v_9, float16_t(t_high.y));
}

matrix<float16_t, 4, 4> v_10(uint start_byte_offset) {
  uint4 v_11 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_12 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_11.zw) : (v_11.xy)));
  uint4 v_13 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_14 = tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_13.zw) : (v_13.xy)));
  uint4 v_15 = u[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_16 = tint_bitcast_to_f16(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_15.zw) : (v_15.xy)));
  uint4 v_17 = u[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 4>(v_12, v_14, v_16, tint_bitcast_to_f16(((((((24u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_17.zw) : (v_17.xy))));
}

S v_18(uint start_byte_offset) {
  int v_19 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 4, 4> v_20 = v_10((8u + start_byte_offset));
  S v_21 = {v_19, v_20, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_21;
}

void v_22(uint start_byte_offset, S object[4]) {
  {
    uint v_23 = 0u;
    v_23 = 0u;
    while(true) {
      uint v_24 = v_23;
      if ((v_24 >= 4u)) {
        break;
      }
      S v_25 = object[v_24];
      v_6((start_byte_offset + (v_24 * 128u)), v_25);
      {
        v_23 = (v_24 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_26(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_27 = 0u;
    v_27 = 0u;
    while(true) {
      uint v_28 = v_27;
      if ((v_28 >= 4u)) {
        break;
      }
      S v_29 = v_18((start_byte_offset + (v_28 * 128u)));
      a[v_28] = v_29;
      {
        v_27 = (v_28 + 1u);
      }
    }
  }
  S v_30[4] = a;
  return v_30;
}

void f_inner(uint tint_local_index) {
  {
    uint v_31 = 0u;
    v_31 = tint_local_index;
    while(true) {
      uint v_32 = v_31;
      if ((v_32 >= 4u)) {
        break;
      }
      S v_33 = (S)0;
      v_6((v_32 * 128u), v_33);
      {
        v_31 = (v_32 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_34[4] = v_26(0u);
  v_22(0u, v_34);
  S v_35 = v_18(256u);
  v_6(128u, v_35);
  v_1(392u, v_10(264u));
  uint2 v_36 = tint_bitcast_from_f16(tint_bitcast_to_f16(u[1u].xy).ywxz);
  w[34u] = v_36.x;
  w[35u] = v_36.y;
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

