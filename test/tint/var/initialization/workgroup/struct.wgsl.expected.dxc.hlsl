struct S {
  int a;
  float b;
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[2];
void v_1(uint start_byte_offset, S object) {
  v[(start_byte_offset / 4u)] = asuint(object.a);
  v[((4u + start_byte_offset) / 4u)] = asuint(object.b);
}

S v_2(uint start_byte_offset) {
  S v_3 = {asint(v[(start_byte_offset / 4u)]), asfloat(v[((4u + start_byte_offset) / 4u)])};
  return v_3;
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    S v_4 = (S)0;
    v_1(0u, v_4);
  }
  GroupMemoryBarrierWithGroupSync();
  v_2(0u);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

