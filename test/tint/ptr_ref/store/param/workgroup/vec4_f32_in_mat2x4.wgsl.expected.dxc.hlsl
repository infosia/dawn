struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint4 S[2];
void func(uint pointer_indices[1]) {
  S[((pointer_indices[0u] * 16u) / 16u)] = asuint((0.0f).xxxx);
}

void v(uint start_byte_offset, float2x4 object) {
  S[(start_byte_offset / 16u)] = asuint(object[0u]);
  S[((16u + start_byte_offset) / 16u)] = asuint(object[1u]);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v(0u, float2x4((0.0f).xxxx, (0.0f).xxxx));
  }
  GroupMemoryBarrierWithGroupSync();
  uint v_1[1] = {1u};
  func(v_1);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

