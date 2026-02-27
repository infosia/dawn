struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[3];
void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v[0u] = asuint((int(0)).xxx.x);
    v[1u] = asuint((int(0)).xxx.y);
    v[2u] = asuint((int(0)).xxx.z);
  }
  GroupMemoryBarrierWithGroupSync();
  asint(uint3(v[0u], v[1u], v[2u]).xyz);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

