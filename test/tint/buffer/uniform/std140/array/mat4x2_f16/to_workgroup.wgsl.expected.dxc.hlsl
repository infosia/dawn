struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
groupshared uint16_t w[32];
void v_1(uint start_byte_offset, matrix<float16_t, 4, 2> object) {
  w[(start_byte_offset / 2u)] = asuint16(object[0u].x);
  w[((start_byte_offset / 2u) + 1u)] = asuint16(object[0u].y);
  w[((4u + start_byte_offset) / 2u)] = asuint16(object[1u].x);
  w[(((4u + start_byte_offset) / 2u) + 1u)] = asuint16(object[1u].y);
  w[((8u + start_byte_offset) / 2u)] = asuint16(object[2u].x);
  w[(((8u + start_byte_offset) / 2u) + 1u)] = asuint16(object[2u].y);
  w[((12u + start_byte_offset) / 2u)] = asuint16(object[3u].x);
  w[(((12u + start_byte_offset) / 2u) + 1u)] = asuint16(object[3u].y);
}

vector<float16_t, 2> tint_bitcast_to_f16(uint src) {
  uint v = src;
  float t_low = f16tof32((v & 65535u));
  float t_high = f16tof32(((v >> 16u) & 65535u));
  float16_t v_2 = float16_t(t_low);
  return vector<float16_t, 2>(v_2, float16_t(t_high));
}

matrix<float16_t, 4, 2> v_3(uint start_byte_offset) {
  vector<float16_t, 2> v_4 = tint_bitcast_to_f16(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  vector<float16_t, 2> v_5 = tint_bitcast_to_f16(u[((4u + start_byte_offset) / 16u)][(((4u + start_byte_offset) & 15u) >> 2u)]);
  vector<float16_t, 2> v_6 = tint_bitcast_to_f16(u[((8u + start_byte_offset) / 16u)][(((8u + start_byte_offset) & 15u) >> 2u)]);
  return matrix<float16_t, 4, 2>(v_4, v_5, v_6, tint_bitcast_to_f16(u[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)]));
}

void v_7(uint start_byte_offset, matrix<float16_t, 4, 2> object[4]) {
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      v_1((start_byte_offset + (v_9 * 16u)), object[v_9]);
      {
        v_8 = (v_9 + 1u);
      }
    }
  }
}

typedef matrix<float16_t, 4, 2> ary_ret[4];
ary_ret v_10(uint start_byte_offset) {
  matrix<float16_t, 4, 2> a[4] = (matrix<float16_t, 4, 2>[4])0;
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      a[v_12] = v_3((start_byte_offset + (v_12 * 16u)));
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
  matrix<float16_t, 4, 2> v_13[4] = a;
  return v_13;
}

void f_inner(uint tint_local_index) {
  {
    uint v_14 = 0u;
    v_14 = tint_local_index;
    while(true) {
      uint v_15 = v_14;
      if ((v_15 >= 4u)) {
        break;
      }
      v_1((v_15 * 16u), matrix<float16_t, 4, 2>((float16_t(0.0h)).xx, (float16_t(0.0h)).xx, (float16_t(0.0h)).xx, (float16_t(0.0h)).xx));
      {
        v_14 = (v_15 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  matrix<float16_t, 4, 2> v_16[4] = v_10(0u);
  v_7(0u, v_16);
  v_1(16u, v_3(32u));
  vector<float16_t, 2> v_17 = tint_bitcast_to_f16(u[0u].y).yx;
  w[8u] = asuint16(v_17.x);
  w[9u] = asuint16(v_17.y);
  w[8u] = asuint16(tint_bitcast_to_f16(u[0u].y).x);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

