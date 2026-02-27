struct S {
  int before;
  matrix<float16_t, 3, 4> m;
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

void v_1(uint start_byte_offset, matrix<float16_t, 3, 4> object) {
  uint2 v_2 = tint_bitcast_from_f16(object[0u]);
  w[(start_byte_offset / 4u)] = v_2.x;
  w[((start_byte_offset / 4u) + 1u)] = v_2.y;
  uint2 v_3 = tint_bitcast_from_f16(object[1u]);
  w[((8u + start_byte_offset) / 4u)] = v_3.x;
  w[(((8u + start_byte_offset) / 4u) + 1u)] = v_3.y;
  uint2 v_4 = tint_bitcast_from_f16(object[2u]);
  w[((16u + start_byte_offset) / 4u)] = v_4.x;
  w[(((16u + start_byte_offset) / 4u) + 1u)] = v_4.y;
}

void v_5(uint start_byte_offset, S object) {
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
  float16_t v_6 = float16_t(t_low.x);
  float16_t v_7 = float16_t(t_high.x);
  float16_t v_8 = float16_t(t_low.y);
  return vector<float16_t, 4>(v_6, v_7, v_8, float16_t(t_high.y));
}

matrix<float16_t, 3, 4> v_9(uint start_byte_offset) {
  uint4 v_10 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_11 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_10.zw) : (v_10.xy)));
  uint4 v_12 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 4> v_13 = tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_12.zw) : (v_12.xy)));
  uint4 v_14 = u[((16u + start_byte_offset) / 16u)];
  return matrix<float16_t, 3, 4>(v_11, v_13, tint_bitcast_to_f16(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_14.zw) : (v_14.xy))));
}

S v_15(uint start_byte_offset) {
  int v_16 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 3, 4> v_17 = v_9((8u + start_byte_offset));
  S v_18 = {v_16, v_17, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_18;
}

void v_19(uint start_byte_offset, S object[4]) {
  {
    uint v_20 = 0u;
    v_20 = 0u;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 4u)) {
        break;
      }
      S v_22 = object[v_21];
      v_5((start_byte_offset + (v_21 * 128u)), v_22);
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_23(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_24 = 0u;
    v_24 = 0u;
    while(true) {
      uint v_25 = v_24;
      if ((v_25 >= 4u)) {
        break;
      }
      S v_26 = v_15((start_byte_offset + (v_25 * 128u)));
      a[v_25] = v_26;
      {
        v_24 = (v_25 + 1u);
      }
    }
  }
  S v_27[4] = a;
  return v_27;
}

void f_inner(uint tint_local_index) {
  {
    uint v_28 = 0u;
    v_28 = tint_local_index;
    while(true) {
      uint v_29 = v_28;
      if ((v_29 >= 4u)) {
        break;
      }
      S v_30 = (S)0;
      v_5((v_29 * 128u), v_30);
      {
        v_28 = (v_29 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_31[4] = v_23(0u);
  v_19(0u, v_31);
  S v_32 = v_15(256u);
  v_5(128u, v_32);
  v_1(392u, v_9(264u));
  uint2 v_33 = tint_bitcast_from_f16(tint_bitcast_to_f16(u[1u].xy).ywxz);
  w[34u] = v_33.x;
  w[35u] = v_33.y;
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

