struct S {
  uint3 a;
  uint b;
  uint3 c[4];
};

struct foo_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_ubuffer : register(b0) {
  uint4 ubuffer[5];
};
RWByteAddressBuffer sbuffer : register(u1);
groupshared uint wbuffer[20];
void v(uint start_byte_offset, uint3 object[4]) {
  {
    uint v_1 = 0u;
    v_1 = 0u;
    while(true) {
      uint v_2 = v_1;
      if ((v_2 >= 4u)) {
        break;
      }
      wbuffer[((start_byte_offset + (v_2 * 16u)) / 4u)] = object[v_2].x;
      wbuffer[(((start_byte_offset + (v_2 * 16u)) / 4u) + 1u)] = object[v_2].y;
      wbuffer[((((start_byte_offset + (v_2 * 16u)) / 4u) + 1u) + 1u)] = object[v_2].z;
      {
        v_1 = (v_2 + 1u);
      }
    }
  }
}

void v_3(uint start_byte_offset, S object) {
  wbuffer[(start_byte_offset / 4u)] = object.a.x;
  wbuffer[((start_byte_offset / 4u) + 1u)] = object.a.y;
  wbuffer[(((start_byte_offset / 4u) + 1u) + 1u)] = object.a.z;
  wbuffer[((12u + start_byte_offset) / 4u)] = object.b;
  uint3 v_4[4] = object.c;
  v((16u + start_byte_offset), v_4);
}

void v_5(uint offset, uint3 obj[4]) {
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      sbuffer.Store3((offset + (v_7 * 16u)), obj[v_7]);
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
}

void v_8(uint offset, S obj) {
  sbuffer.Store3((offset + 0u), obj.a);
  sbuffer.Store((offset + 12u), obj.b);
  uint3 v_9[4] = obj.c;
  v_5((offset + 16u), v_9);
}

typedef uint3 ary_ret[4];
ary_ret v_10(uint offset) {
  uint3 a[4] = (uint3[4])0;
  {
    uint v_11 = 0u;
    v_11 = 0u;
    while(true) {
      uint v_12 = v_11;
      if ((v_12 >= 4u)) {
        break;
      }
      a[v_12] = sbuffer.Load3((offset + (v_12 * 16u)));
      {
        v_11 = (v_12 + 1u);
      }
    }
  }
  uint3 v_13[4] = a;
  return v_13;
}

S v_14(uint offset) {
  uint3 v_15 = sbuffer.Load3((offset + 0u));
  uint v_16 = sbuffer.Load((offset + 12u));
  uint3 v_17[4] = v_10((offset + 16u));
  S v_18 = {v_15, v_16, v_17};
  return v_18;
}

typedef uint3 ary_ret_1[4];
ary_ret_1 v_19(uint start_byte_offset) {
  uint3 a[4] = (uint3[4])0;
  {
    uint v_20 = 0u;
    v_20 = 0u;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 4u)) {
        break;
      }
      a[v_21] = ubuffer[((start_byte_offset + (v_21 * 16u)) / 16u)].xyz;
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
  uint3 v_22[4] = a;
  return v_22;
}

S v_23(uint start_byte_offset) {
  uint3 v_24 = ubuffer[(start_byte_offset / 16u)].xyz;
  uint v_25 = ubuffer[((12u + start_byte_offset) / 16u)][(((12u + start_byte_offset) & 15u) >> 2u)];
  uint3 v_26[4] = v_19((16u + start_byte_offset));
  S v_27 = {v_24, v_25, v_26};
  return v_27;
}

void foo_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    wbuffer[0u] = (0u).xxx.x;
    wbuffer[1u] = (0u).xxx.y;
    wbuffer[2u] = (0u).xxx.z;
    wbuffer[3u] = 0u;
  }
  {
    uint v_28 = 0u;
    v_28 = tint_local_index;
    while(true) {
      uint v_29 = v_28;
      if ((v_29 >= 4u)) {
        break;
      }
      wbuffer[((16u + (v_29 * 16u)) / 4u)] = (0u).xxx.x;
      wbuffer[(((16u + (v_29 * 16u)) / 4u) + 1u)] = (0u).xxx.y;
      wbuffer[((((16u + (v_29 * 16u)) / 4u) + 1u) + 1u)] = (0u).xxx.z;
      {
        v_28 = (v_29 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  S u = v_23(0u);
  S s = v_14(0u);
  S w = v_14(0u);
  S v_30 = (S)0;
  v_8(0u, v_30);
  v_3(0u, v_30);
}

[numthreads(1, 1, 1)]
void foo(foo_inputs inputs) {
  foo_inner(inputs.tint_local_index);
}

