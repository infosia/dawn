struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[4];
void v(uint start_byte_offset, int object[4]) {
  {
    uint v_1 = 0u;
    v_1 = 0u;
    while(true) {
      uint v_2 = v_1;
      if ((v_2 >= 4u)) {
        break;
      }
      S[((start_byte_offset + (v_2 * 4u)) / 4u)] = asuint(object[v_2]);
      {
        v_1 = (v_2 + 1u);
      }
    }
  }
}

void func() {
  int v_3[4] = (int[4])0;
  v(0u, v_3);
}

void main_inner(uint tint_local_index) {
  {
    uint v_4 = 0u;
    v_4 = tint_local_index;
    while(true) {
      uint v_5 = v_4;
      if ((v_5 >= 4u)) {
        break;
      }
      S[((v_5 * 4u) / 4u)] = asuint(int(0));
      {
        v_4 = (v_5 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

