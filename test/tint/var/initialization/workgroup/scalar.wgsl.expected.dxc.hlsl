struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[1];
void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v[0u] = asuint(int(0));
  }
  GroupMemoryBarrierWithGroupSync();
  int i = asint(v[0u]);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

