struct str {
  int i;
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[1];
int func() {
  return asint(S[0u]);
}

void v(uint start_byte_offset, str object) {
  S[(start_byte_offset / 4u)] = asuint(object.i);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    str v_1 = (str)0;
    v(0u, v_1);
  }
  GroupMemoryBarrierWithGroupSync();
  int r = func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

