struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint zero[6];
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

typedef int ary_ret_1[2][3];
ary_ret_1 v_5(uint start_byte_offset) {
  int a[2][3] = (int[2][3])0;
  {
    uint v_6 = 0u;
    v_6 = 0u;
    while(true) {
      uint v_7 = v_6;
      if ((v_7 >= 2u)) {
        break;
      }
      int v_8[3] = v_1((start_byte_offset + (v_7 * 12u)));
      a[v_7] = v_8;
      {
        v_6 = (v_7 + 1u);
      }
    }
  }
  int v_9[2][3] = a;
  return v_9;
}

void main_inner(uint tint_local_index) {
  {
    uint v_10 = 0u;
    v_10 = tint_local_index;
    while(true) {
      uint v_11 = v_10;
      if ((v_11 >= 6u)) {
        break;
      }
      zero[((((v_11 / 3u) * 12u) + ((v_11 % 3u) * 4u)) / 4u)] = asuint(int(0));
      {
        v_10 = (v_11 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  int v[2][3] = v_5(0u);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

