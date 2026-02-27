struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[1];
int func() {
  return asint(S[0u]);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    S[0u] = asuint(int(0));
  }
  GroupMemoryBarrierWithGroupSync();
  int r = func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

