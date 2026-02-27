struct S {
  int before;
  float3x4 m;
  int after;
};

struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[32];
};
groupshared uint w[128];
void v(uint start_byte_offset, float3x4 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  w[((((start_byte_offset / 4u) + 1u) + 1u) + 1u)] = asuint(object[0u].w);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
  w[(((((16u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object[1u].w);
  w[((32u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((32u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  w[((((32u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[2u].z);
  w[(((((32u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object[2u].w);
}

void v_1(uint start_byte_offset, S object) {
  w[(start_byte_offset / 4u)] = asuint(object.before);
  v((16u + start_byte_offset), object.m);
  w[((64u + start_byte_offset) / 4u)] = asuint(object.after);
}

float3x4 v_2(uint start_byte_offset) {
  return float3x4(asfloat(u[(start_byte_offset / 16u)]), asfloat(u[((16u + start_byte_offset) / 16u)]), asfloat(u[((32u + start_byte_offset) / 16u)]));
}

S v_3(uint start_byte_offset) {
  int v_4 = asint(u[(start_byte_offset / 16u)][((start_byte_offset & 15u) >> 2u)]);
  float3x4 v_5 = v_2((16u + start_byte_offset));
  S v_6 = {v_4, v_5, asint(u[((64u + start_byte_offset) / 16u)][(((64u + start_byte_offset) & 15u) >> 2u)])};
  return v_6;
}

void v_7(uint start_byte_offset, S object[4]) {
  {
    uint v_8 = 0u;
    v_8 = 0u;
    while(true) {
      uint v_9 = v_8;
      if ((v_9 >= 4u)) {
        break;
      }
      S v_10 = object[v_9];
      v_1((start_byte_offset + (v_9 * 128u)), v_10);
      {
        v_8 = (v_9 + 1u);
      }
    }
  }
}

typedef S ary_ret[4];
ary_ret v_11(uint start_byte_offset) {
  S a[4] = (S[4])0;
  {
    uint v_12 = 0u;
    v_12 = 0u;
    while(true) {
      uint v_13 = v_12;
      if ((v_13 >= 4u)) {
        break;
      }
      S v_14 = v_3((start_byte_offset + (v_13 * 128u)));
      a[v_13] = v_14;
      {
        v_12 = (v_13 + 1u);
      }
    }
  }
  S v_15[4] = a;
  return v_15;
}

void f_inner(uint tint_local_index) {
  {
    uint v_16 = 0u;
    v_16 = tint_local_index;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      S v_18 = (S)0;
      v_1((v_17 * 128u), v_18);
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S v_19[4] = v_11(0u);
  v_7(0u, v_19);
  S v_20 = v_3(256u);
  v_1(128u, v_20);
  v(400u, v_2(272u));
  float4 v_21 = asfloat(u[2u]).ywxz;
  w[36u] = asuint(v_21.x);
  w[37u] = asuint(v_21.y);
  w[38u] = asuint(v_21.z);
  w[39u] = asuint(v_21.w);
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

