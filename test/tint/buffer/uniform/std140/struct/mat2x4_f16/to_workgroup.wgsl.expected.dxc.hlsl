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
groupshared uint w[128];
uint2 tint_bitcast_from_f16(vector<float16_t, 4> src) {
  uint4 r = f32tof16(float4(src));
  return uint2(((r.x & 65535u) | ((r.y & 65535u) << 16u)), ((r.z & 65535u) | ((r.w & 65535u) << 16u)));
}

void v_1(uint start_byte_offset, matrix<float16_t, 2, 4> object) {
  uint2 v_2 = tint_bitcast_from_f16(object[0u]);
  w[(start_byte_offset / 4u)] = v_2.x;
  w[((start_byte_offset / 4u) + 1u)] = v_2.y;
  uint2 v_3 = tint_bitcast_from_f16(object[1u]);
  w[((8u + start_byte_offset) / 4u)] = v_3.x;
  w[(((8u + start_byte_offset) / 4u) + 1u)] = v_3.y;
}

void v_4(uint start_byte_offset, S object) {
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
  float16_t v_5 = float16_t(t_low.x);
  float16_t v_6 = float16_t(t_high.x);
  float16_t v_7 = float16_t(t_low.y);
  return vector<float16_t, 4>(v_5, v_6, v_7, float16_t(t_high.y));
}

matrix<float16_t, 2, 4> v_8(uint start_byte_offset) {
  uint4 v_9 = u[(start_byte_offset / 16u)];
  vector<float16_t, 4> v_10 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_9.zw) : (v_9.xy)));
  uint4 v_11 = u[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 4>(v_10, tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_11.zw) : (v_11.xy))));
}

S v_12(uint start_byte_offset) {
  int v_13 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 2, 4> v_14 = v_8((8u + start_byte_offset));
  S v_15 = {v_13, v_14, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_15;
}

void v_16(uint start_byte_offset, S object[4]) {
  {
    uint v_17 = 0u;
    v_17 = 0u;
    while(true) {
      uint v_18 = v_17;
      if ((v_18 >= 4u)) {
        break;
      }
      S v_19 = object[v_18];
      v_4((start_byte_offset + (v_18 * 128u)), v_19);
      {
        v_17 = (v_18 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_20(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_21 = 0u;
    v_21 = 0u;
    while(true) {
      uint v_22 = v_21;
      if ((v_22 >= 4u)) {
        break;
      }
      S v_23 = v_12((start_byte_offset + (v_22 * 128u)));
      a[v_22] = v_23;
      {
        v_21 = (v_22 + 1u);
      }
    }
  }
  S v_24[4] = a;
  return v_24;
}

void f_inner(uint tint_local_index) {
  {
    uint v_25 = 0u;
    v_25 = tint_local_index;
    while(true) {
      uint v_26 = v_25;
      if ((v_26 >= 4u)) {
        break;
      }
      S v_27 = (S)0;
      v_4((v_26 * 128u), v_27);
      {
        v_25 = (v_26 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_28[4] = v_20(0u);
  v_16(0u, v_28);
  S v_29 = v_12(256u);
  v_4(128u, v_29);
  v_1(392u, v_8(264u));
  uint2 v_30 = tint_bitcast_from_f16(tint_bitcast_to_f16(u[1u].xy).ywxz);
  w[34u] = v_30.x;
  w[35u] = v_30.y;
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

