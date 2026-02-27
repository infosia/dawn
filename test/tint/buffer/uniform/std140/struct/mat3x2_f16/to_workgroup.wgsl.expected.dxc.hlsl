struct S {
  int before;
  matrix<float16_t, 3, 2> m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint w[128];
uint tint_bitcast_from_f16(vector<float16_t, 2> src) {
  uint2 r = f32tof16(float2(src));
  return ((r.x & 65535u) | ((r.y & 65535u) << 16u));
}

void v_1(uint start_byte_offset, matrix<float16_t, 3, 2> object) {
  w[(start_byte_offset / 4u)] = tint_bitcast_from_f16(object[0u]);
  w[((4u + start_byte_offset) / 4u)] = tint_bitcast_from_f16(object[1u]);
  w[((8u + start_byte_offset) / 4u)] = tint_bitcast_from_f16(object[2u]);
}

void v_2(uint start_byte_offset, S object) {
  w[(start_byte_offset / 4u)] = asuint(object.before);
  v_1((4u + start_byte_offset), object.m);
  w[((64u + start_byte_offset) / 4u)] = asuint(object.after);
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_3 = float16_t(t_low);
  return vector<float16_t, 2>(v_3, float16_t(t_high));
}

matrix<float16_t, 3, 2> v_4(uint start_byte_offset) {
  vector<float16_t, 2> v_5 = tint_bitcast_to_f16(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_6 = tint_bitcast_to_f16(u[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 3, 2>(v_5, v_6, tint_bitcast_to_f16(u[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]));
}

S v_7(uint start_byte_offset) {
  int v_8 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 3, 2> v_9 = v_4((4u + start_byte_offset));
  S v_10 = {v_8, v_9, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_10;
}

void v_11(uint start_byte_offset, S object[4]) {
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      S v_14 = object[v_13];
      v_2((start_byte_offset + (v_13 * 128u)), v_14);
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

void f_inner(uint tint_local_index) {
  {
    uint v_20 = 0u;
    v_20 = tint_local_index;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 4u)) {
        break;
      }
      S v_22 = (S)0;
      v_2((v_21 * 128u), v_22);
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_23[4] = v_15(0u);
  v_11(0u, v_23);
  S v_24 = v_7(256u);
  v_2(128u, v_24);
  v_1(388u, v_4(260u));
  w[33u] = tint_bitcast_from_f16(tint_bitcast_to_f16(u[0u].z).yx);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

