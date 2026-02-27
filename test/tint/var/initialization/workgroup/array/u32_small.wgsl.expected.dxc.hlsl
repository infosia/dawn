struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint zero[3];
typedef int ary_ret[3];
ary_ret v_1(uint start_byte_offset) {
  int a[3] = (int[3])0;
  {
    uint v_2 = 0u;
    v_2 = 0u;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 3u)) {
        break;
      }
      a[v_3] = asint(zero[((start_byte_offset + (v_3 * 4u)) / 4u)]);
      {
        v_2 = (v_3 + 1u);
      }
    }
  }
  int v_4[3] = a;
  return v_4;
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 3u)) {
    zero[((tint_local_index * 4u) / 4u)] = asuint(int(0));
  }
  GroupMemoryBarrierWithGroupSync();
  int v[3] = v_1(0u);
}

[numthreads(10, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

