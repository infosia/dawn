groupshared int zero[3];

void tint_zero_workgroup_memory(uint local_idx) {
  if ((local_idx < 3u)) {
    uint i = local_idx;
    int atomic_result = 0;
    InterlockedExchange(zero[i], 0, atomic_result);
  }
  GroupMemoryBarrierWithGroupSync();
}

struct tint_symbol_1 {
  uint local_invocation_index : SV_GroupIndex;
};

void main_inner(uint local_invocation_index) {
  tint_zero_workgroup_memory(local_invocation_index);
  int atomic_result_1 = 0;
  InterlockedOr(zero[0], 0, atomic_result_1);
  int v = atomic_result_1;
}

[numthreads(10, 1, 1)]
void main(tint_symbol_1 tint_symbol) {
  main_inner(tint_symbol.local_invocation_index);
  return;
}
