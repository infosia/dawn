struct Inner {
  bool b;
  int4 v;
  float3x3 m;
};

struct Outer {
  Inner a[4];
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[80];
float3x3 v_1(uint start_byte_offset) {
  float3 v_2 = asfloat(uint3(v[(start_byte_offset / 4u)], v[((start_byte_offset / 4u) + 1u)], v[(((start_byte_offset / 4u) + 1u) + 1u)]).xyz);
  float3 v_3 = asfloat(uint3(v[((16u + start_byte_offset) / 4u)], v[(((16u + start_byte_offset) / 4u) + 1u)], v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)]).xyz);
  return float3x3(v_2, v_3, asfloat(uint3(v[((32u + start_byte_offset) / 4u)], v[(((32u + start_byte_offset) / 4u) + 1u)], v[((((32u + start_byte_offset) / 4u) + 1u) + 1u)]).xyz));
}

Inner v_4(uint start_byte_offset) {
  bool v_5 = bool(v[(start_byte_offset / 4u)]);
  int4 v_6 = asint(uint4(v[((16u + start_byte_offset) / 4u)], v[(((16u + start_byte_offset) / 4u) + 1u)], v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)], v[(((((16u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)]));
  Inner v_7 = {v_5, v_6, v_1((32u + start_byte_offset))};
  return v_7;
}

typedef Inner ary_ret[4];
ary_ret v_8(uint start_byte_offset) {
  Inner a[4] = (Inner[4])0;
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      Inner v_11 = v_4((start_byte_offset + (v_10 * 80u)));
      a[v_10] = v_11;
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
  Inner v_12[4] = a;
  return v_12;
}

Outer v_13(uint start_byte_offset) {
  Inner v_14[4] = v_8(start_byte_offset);
  Outer v_15 = {v_14};
  return v_15;
}

Outer foo() {
  GroupMemoryBarrierWithGroupSync();
  Outer v_16 = v_13(0u);
  GroupMemoryBarrierWithGroupSync();
  return v_16;
}

void v_17(uint start_byte_offset, float3x3 object) {
  v[(start_byte_offset / 4u)] = asuint(object[0u].x);
  v[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  v[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  v[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  v[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
  v[((32u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  v[(((32u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  v[((((32u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[2u].z);
}

void v_18(uint start_byte_offset, Inner object) {
  v[(start_byte_offset / 4u)] = uint(object.b);
  v[((16u + start_byte_offset) / 4u)] = asuint(object.v.x);
  v[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object.v.y);
  v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object.v.z);
  v[(((((16u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object.v.w);
  v_17((32u + start_byte_offset), object.m);
}

void main_inner(uint tint_local_index) {
  {
    uint v_19 = 0u;
    v_19 = tint_local_index;
    while(true) {
      uint v_20 = v_19;
      if ((v_20 >= 4u)) {
        break;
      }
      Inner v_21 = (Inner)0;
      v_18((v_20 * 80u), v_21);
      {
        v_19 = (v_20 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  foo();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

