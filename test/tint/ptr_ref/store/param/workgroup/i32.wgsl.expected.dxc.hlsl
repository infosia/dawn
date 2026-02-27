struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint S[1];
void func() {
  S[0u] = asuint(int(42));
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    S[0u] = asuint(int(0));
  }
  GroupMemoryBarrierWithGroupSync();
  func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

