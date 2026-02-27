struct S {
  int before;
  matrix<float16_t, 2, 2> m;
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

void v_1(uint start_byte_offset, matrix<float16_t, 2, 2> object) {
  w[(start_byte_offset / 4u)] = tint_bitcast_from_f16(object[0u]);
  w[((4u + start_byte_offset) / 4u)] = tint_bitcast_from_f16(object[1u]);
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

matrix<float16_t, 2, 2> v_4(uint start_byte_offset) {
  vector<float16_t, 2> v_5 = tint_bitcast_to_f16(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  return matrix<float16_t, 2, 2>(v_5, tint_bitcast_to_f16(u[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]));
}

S v_6(uint start_byte_offset) {
  int v_7 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 2, 2> v_8 = v_4((4u + start_byte_offset));
  S v_9 = {v_7, v_8, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_9;
}

void v_10(uint start_byte_offset, S object[4]) {
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      S v_13 = object[v_12];
      v_2((start_byte_offset + (v_12 * 128u)), v_13);
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_14(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_15 = 0u;
    v_15 = 0u;
    while(true) {
      uint v_16 = v_15;
      if ((v_16 >= 4u)) {
        break;
      }
      S v_17 = v_6((start_byte_offset + (v_16 * 128u)));
      a[v_16] = v_17;
      {
        v_15 = (v_16 + 1u);
      }
    }
  }
  S v_18[4] = a;
  return v_18;
}

void f_inner(uint tint_local_index) {
  {
    uint v_19 = 0u;
    v_19 = tint_local_index;
    while(true) {
      uint v_20 = v_19;
      if ((v_20 >= 4u)) {
        break;
      }
      S v_21 = (S)0;
      v_2((v_20 * 128u), v_21);
      {
        v_19 = (v_20 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_22[4] = v_14(0u);
  v_10(0u, v_22);
  S v_23 = v_6(256u);
  v_2(128u, v_23);
  v_1(388u, v_4(260u));
  w[33u] = tint_bitcast_from_f16(tint_bitcast_to_f16(u[0u].z).yx);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

