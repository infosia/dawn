struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared int zero[3];
void main_inner(uint tint_local_index) {
  int v_1 = int(0);
  InterlockedExchange(zero[(tint_local_index % 3u)], int(0), v_1);
  GroupMemoryBarrierWithGroupSync();
  int v_2 = int(0);
  InterlockedOr(zero[int(0)], int(0), v_2);
  int v = v_2;
}

[numthreads(10, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

