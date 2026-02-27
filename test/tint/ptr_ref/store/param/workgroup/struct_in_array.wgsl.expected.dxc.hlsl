struct str {
  int i;
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[4];
void v(uint start_byte_offset, str object) {
  S[(start_byte_offset / 4u)] = asuint(object.i);
}

void func(uint pointer_indices[1]) {
  str v_1 = (str)0;
  v((pointer_indices[0u] * 4u), v_1);
}

void main_inner(uint tint_local_index) {
  {
    uint v_2 = 0u;
    v_2 = tint_local_index;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 4u)) {
        break;
      }
      str v_4 = (str)0;
      v((v_3 * 4u), v_4);
      {
        v_2 = (v_3 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  uint v_5[1] = {2u};
  func(v_5);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

