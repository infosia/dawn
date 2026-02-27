struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
RWByteAddressBuffer s : register(u1);
groupshared uint16_t w[32];
void v_1(uint start_byte_offset, matrix<float16_t, 2, 3> object) {
  w[(start_byte_offset / 2u)] = asuint16(object[0u].x);
  w[((start_byte_offset / 2u) + 1u)] = asuint16(object[0u].y);
  w[(((start_byte_offset / 2u) + 1u) + 1u)] = asuint16(object[0u].z);
  w[((8u + start_byte_offset) / 2u)] = asuint16(object[1u].x);
  w[(((8u + start_byte_offset) / 2u) + 1u)] = asuint16(object[1u].y);
  w[((((8u + start_byte_offset) / 2u) + 1u) + 1u)] = asuint16(object[1u].z);
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_2 = float16_t(t_low);
  return vector<float16_t, 2>(v_2, float16_t(t_high));
}

vector<float16_t, 4> tint_bitcast_to_f16_1(uint2 src) {
  uint2 v = src;
  uint2 mask = (65535u).xx;
  uint2 shift = (16u).xx;
  float2 t_low = f16tof32((v & mask));
  float2 t_high = f16tof32(((v >> shift) & mask));
  float16_t v_3 = float16_t(t_low.x);
  float16_t v_4 = float16_t(t_high.x);
  float16_t v_5 = float16_t(t_low.y);
  return vector<float16_t, 4>(v_3, v_4, v_5, float16_t(t_high.y));
}

matrix<float16_t, 2, 3> v_6(uint start_byte_offset) {
  uint4 v_7 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_8 = tint_bitcast_to_f16_1((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_7.zw) : (v_7.xy))).xyz;
  uint4 v_9 = u[((8u + start_byte_offset) / 16u)];
  return matrix<float16_t, 2, 3>(v_8, tint_bitcast_to_f16_1(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_9.zw) : (v_9.xy))).xyz);
}

void v_10(uint start_byte_offset, matrix<float16_t, 2, 3> object[4]) {
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      v_1((start_byte_offset + (v_12 * 16u)), object[v_12]);
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
}

typedef matrix<float16_t, 2, 3> ary_ret[4];
ary_ret v_13(uint start_byte_offset) {
  matrix<float16_t, 2, 3> a[4] = (matrix<float16_t, 2, 3>[4])0;
  {
    uint v_14 = 0u;
    v_14 = 0u;
    while(true) {
      uint v_15 = v_14;
      if ((v_15 >= 4u)) {
        break;
      }
      a[v_15] = v_6((start_byte_offset + (v_15 * 16u)));
      {
        v_14 = (v_15 + 1u);
      }
    }
  }
  matrix<float16_t, 2, 3> v_16[4] = a;
  return v_16;
}

void f_inner(uint tint_local_index) {
  {
    uint v_17 = 0u;
    v_17 = tint_local_index;
    while(true) {
      uint v_18 = v_17;
      if ((v_18 >= 4u)) {
        break;
      }
      v_1((v_18 * 16u), matrix<float16_t, 2, 3>((float16_t(0.0h)).xxx, (float16_t(0.0h)).xxx));
      {
        v_17 = (v_18 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  matrix<float16_t, 2, 3> v_19[4] = v_13(0u);
  v_10(0u, v_19);
  v_1(16u, v_6(32u));
  vector<float16_t, 3> v_20 = tint_bitcast_to_f16_1(u[0u].zw).xyz.zxy;
  w[8u] = asuint16(v_20.x);
  w[9u] = asuint16(v_20.y);
  w[10u] = asuint16(v_20.z);
  w[8u] = asuint16(tint_bitcast_to_f16(u[0u].z).x);
  s.Store<float16_t>(0u, asfloat16(w[8u]));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

