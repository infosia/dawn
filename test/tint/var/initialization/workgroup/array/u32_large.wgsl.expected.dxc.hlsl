struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint zero[23];
typedef int ary_ret[23];
ary_ret v_1(uint start_byte_offset) {
  int a[23] = (int[23])0;
  {
    uint v_2 = 0u;
    v_2 = 0u;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 23u)) {
        break;
      }
      a[v_3] = asint(zero[((start_byte_offset + (v_3 * 4u)) / 4u)]);
      {
        v_2 = (v_3 + 1u);
      }
    }
  }
  int v_4[23] = a;
  return v_4;
}

void main_inner(uint tint_local_index) {
  {
    uint v_5 = 0u;
    v_5 = tint_local_index;
    while(true) {
      uint v_6 = v_5;
      if ((v_6 >= 23u)) {
        break;
      }
      zero[((v_6 * 4u) / 4u)] = asuint(int(0));
      {
        v_5 = (v_6 + 13u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  int v[23] = v_1(0u);
}

[numthreads(13, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

