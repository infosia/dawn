struct str {
  int i;
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[4];
str v(uint start_byte_offset) {
  str v_1 = {asint(S[(start_byte_offset / 4u)])};
  return v_1;
}

str func(uint pointer_indices[1]) {
  str v_2 = v((pointer_indices[0u] * 4u));
  return v_2;
}

void v_3(uint start_byte_offset, str object) {
  S[(start_byte_offset / 4u)] = asuint(object.i);
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
      str v_6 = (str)0;
      v_3((v_5 * 4u), v_6);
      {
        v_4 = (v_5 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  uint v_7[1] = {2u};
  str r = func(v_7);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

