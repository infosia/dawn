struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[4];
typedef int ary_ret[4];
ary_ret v_1(uint start_byte_offset) {
  int a[4] = (int[4])0;
  {
    uint v_2 = 0u;
    v_2 = 0u;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 4u)) {
        break;
      }
      a[v_3] = asint(v[((start_byte_offset + (v_3 * 4u)) / 4u)]);
      {
        v_2 = (v_3 + 1u);
      }
    }
  }
  int v_4[4] = a;
  return v_4;
}

typedef int ary_ret_1[4];
ary_ret_1 foo() {
  GroupMemoryBarrierWithGroupSync();
  int v_5[4] = v_1(0u);
  GroupMemoryBarrierWithGroupSync();
  return v_5;
}

void main_inner(uint tint_local_index) {
  {
    uint v_6 = 0u;
    v_6 = tint_local_index;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 4u)) {
        break;
      }
      v[((v_7 * 4u) / 4u)] = asuint(int(0));
      {
        v_6 = (v_7 + 1u);
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

