struct S {
  int before;
  matrix<float16_t, 4, 3> m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint16_t w[256];
vector<uint16_t, 2> tint_bitcast_to_u16(int src) {
  uint v = asuint(src);
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_1 = float16_t(t_low);
  return asuint16(vector<float16_t, 2>(v_1, float16_t(t_high)));
}

void v_2(uint start_byte_offset, matrix<float16_t, 4, 3> object) {
  w[(start_byte_offset / 2u)] = asuint16(object[0u].x);
  w[((start_byte_offset / 2u) + 1u)] = asuint16(object[0u].y);
  w[(((start_byte_offset / 2u) + 1u) + 1u)] = asuint16(object[0u].z);
  w[((8u + start_byte_offset) / 2u)] = asuint16(object[1u].x);
  w[(((8u + start_byte_offset) / 2u) + 1u)] = asuint16(object[1u].y);
  w[((((8u + start_byte_offset) / 2u) + 1u) + 1u)] = asuint16(object[1u].z);
  w[((16u + start_byte_offset) / 2u)] = asuint16(object[2u].x);
  w[(((16u + start_byte_offset) / 2u) + 1u)] = asuint16(object[2u].y);
  w[((((16u + start_byte_offset) / 2u) + 1u) + 1u)] = asuint16(object[2u].z);
  w[((24u + start_byte_offset) / 2u)] = asuint16(object[3u].x);
  w[(((24u + start_byte_offset) / 2u) + 1u)] = asuint16(object[3u].y);
  w[((((24u + start_byte_offset) / 2u) + 1u) + 1u)] = asuint16(object[3u].z);
}

void v_3(uint start_byte_offset, S object) {
  vector<uint16_t, 2> v_4 = tint_bitcast_to_u16(object.before);
  w[(start_byte_offset / 2u)] = v_4.x;
  w[((start_byte_offset / 2u) + 1u)] = v_4.y;
  v_2((8u + start_byte_offset), object.m);
  vector<uint16_t, 2> v_5 = tint_bitcast_to_u16(object.after);
  w[((64u + start_byte_offset) / 2u)] = v_5.x;
  w[(((64u + start_byte_offset) / 2u) + 1u)] = v_5.y;
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

matrix<float16_t, 4, 3> v_9(uint start_byte_offset) {
  uint4 v_10 = u[(start_byte_offset / 16u)];
  vector<float16_t, 3> v_11 = tint_bitcast_to_f16((((((start_byte_offset & 15u) >> 2u) == 2u)) ? (v_10.zw) : (v_10.xy))).xyz;
  uint4 v_12 = u[((8u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_13 = tint_bitcast_to_f16(((((((8u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_12.zw) : (v_12.xy))).xyz;
  uint4 v_14 = u[((16u + start_byte_offset) / 16u)];
  vector<float16_t, 3> v_15 = tint_bitcast_to_f16(((((((16u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_14.zw) : (v_14.xy))).xyz;
  uint4 v_16 = u[((24u + start_byte_offset) / 16u)];
  return matrix<float16_t, 4, 3>(v_11, v_13, v_15, tint_bitcast_to_f16(((((((24u + start_byte_offset) & 15u) >> 2u) == 2u)) ? (v_16.zw) : (v_16.xy))).xyz);
}

S v_17(uint start_byte_offset) {
  int v_18 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  matrix<float16_t, 4, 3> v_19 = v_9((8u + start_byte_offset));
  S v_20 = {v_18, v_19, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_20;
}

void v_21(uint start_byte_offset, S object[4]) {
  {
    uint v_22 = 0u;
    v_22 = 0u;
    while(true) {
      uint v_23 = v_22;
      if ((v_23 >= 4u)) {
        break;
      }
      S v_24 = object[v_23];
      v_3((start_byte_offset + (v_23 * 128u)), v_24);
      {
        v_22 = (v_23 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_25(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_26 = 0u;
    v_26 = 0u;
    while(true) {
      uint v_27 = v_26;
      if ((v_27 >= 4u)) {
        break;
      }
      S v_28 = v_17((start_byte_offset + (v_27 * 128u)));
      a[v_27] = v_28;
      {
        v_26 = (v_27 + 1u);
      }
    }
  }
  S v_29[4] = a;
  return v_29;
}

void f_inner(uint tint_local_index) {
  {
    uint v_30 = 0u;
    v_30 = tint_local_index;
    while(true) {
      uint v_31 = v_30;
      if ((v_31 >= 4u)) {
        break;
      }
      S v_32 = (S)0;
      v_3((v_31 * 128u), v_32);
      {
        v_30 = (v_31 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_33[4] = v_25(0u);
  v_21(0u, v_33);
  S v_34 = v_17(256u);
  v_3(128u, v_34);
  v_2(392u, v_9(264u));
  vector<float16_t, 3> v_35 = tint_bitcast_to_f16(u[1u].xy).xyz.zxy;
  w[68u] = asuint16(v_35.x);
  w[69u] = asuint16(v_35.y);
  w[70u] = asuint16(v_35.z);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

