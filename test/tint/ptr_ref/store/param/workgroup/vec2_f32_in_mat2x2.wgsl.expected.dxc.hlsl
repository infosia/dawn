struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint2 S[2];
void func(uint pointer_indices[1]) {
  S[((pointer_indices[0u] * 8u) / 8u)] = asuint((0.0f).xx);
}

void v(uint start_byte_offset, float2x2 object) {
  S[(start_byte_offset / 8u)] = asuint(object[0u]);
  S[((8u + start_byte_offset) / 8u)] = asuint(object[1u]);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v(0u, float2x2((0.0f).xx, (0.0f).xx));
  }
  GroupMemoryBarrierWithGroupSync();
  uint v_1[1] = {1u};
  func(v_1);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

