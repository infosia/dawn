struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[4];
typedef int ary_ret[4];
ary_ret v(uint start_byte_offset) {
  int a[4] = (int[4])0;
  {
    uint v_1 = 0u;
    v_1 = 0u;
    while(true) {
      uint v_2 = v_1;
      if ((v_2 >= 4u)) {
        break;
      }
      a[v_2] = asint(S[((start_byte_offset + (v_2 * 4u)) / 4u)]);
      {
        v_1 = (v_2 + 1u);
      }
    }
  }
  int v_3[4] = a;
  return v_3;
}

typedef int ary_ret_1[4];
ary_ret_1 func() {
  int v_4[4] = v(0u);
  return v_4;
}

void main_inner(uint tint_local_index) {
  {
    uint v_5 = 0u;
    v_5 = tint_local_index;
    while(true) {
      uint v_6 = v_5;
      if ((v_6 >= 4u)) {
        break;
      }
      S[((v_6 * 4u) / 4u)] = asuint(int(0));
      {
        v_5 = (v_6 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  int r[4] = func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

