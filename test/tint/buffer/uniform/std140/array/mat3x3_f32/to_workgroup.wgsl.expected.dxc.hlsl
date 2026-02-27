struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[12];
};
groupshared uint w[48];
void v(uint start_byte_offset, float3x3 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
  w[((32u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((32u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  w[((((32u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[2u].z);
}

float3x3 v_1(uint start_byte_offset) {
  return float3x3(asfloat(u[(start_byte_offset / 16u)].xyz), asfloat(u[((16u + start_byte_offset) / 16u)].xyz), asfloat(u[((32u + start_byte_offset) / 16u)].xyz));
}

void v_2(uint start_byte_offset, float3x3 object[4]) {
  {
    uint v_3 = 0u;
    v_3 = 0u;
    while(true) {
      uint v_4 = v_3;
      if ((v_4 >= 4u)) {
        break;
      }
      v((start_byte_offset + (v_4 * 48u)), object[v_4]);
      {
        v_3 = (v_4 + 1u);
      }
    }
  }
}

typedef float3x3 ary_ret[4];
ary_ret v_5(uint start_byte_offset) {
  float3x3 a[4] = (float3x3[4])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      a[v_7] = v_1((start_byte_offset + (v_7 * 48u)));
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  float3x3 v_8[4] = a;
  return v_8;
}

void f_inner(uint tint_local_index) {
  {
    uint v_9 = 0u;
    v_9 = tint_local_index;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      v((v_10 * 48u), float3x3((0.0f).xxx, (0.0f).xxx, (0.0f).xxx));
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  float3x3 v_11[4] = v_5(0u);
  v_2(0u, v_11);
  v(48u, v_1(96u));
  float3 v_12 = asfloat(u[1u].xyz).zxy;
  w[12u] = asuint(v_12.x);
  w[13u] = asuint(v_12.y);
  w[14u] = asuint(v_12.z);
  w[12u] = asuint(asfloat(u[1u].x));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

