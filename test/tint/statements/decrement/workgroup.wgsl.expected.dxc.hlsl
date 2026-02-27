struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint i[1];
void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    i[0u] = asuint(int(0));
  }
  GroupMemoryBarrierWithGroupSync();
  i[0u] = asuint(asint((asuint(asint(i[0u])) - asuint(int(1)))));
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

