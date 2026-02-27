struct F_inputs {
  uint mat2x2 : SV_GroupIndex;
};


groupshared uint2 W[2];
void v(uint start_byte_offset, float2x2 object) {
  W[(start_byte_offset / 8u)] = asuint(object[0u]);
  W[((8u + start_byte_offset) / 8u)] = asuint(object[1u]);
}

void F_inner(uint mat2x2_1) {
  if ((mat2x2_1 < 1u)) {
    v(0u, float2x2((0.0f).xx, (0.0f).xx));
  }
  GroupMemoryBarrierWithGroupSync();
  W[0u] = asuint((asfloat(W[0u]) + 0.0f));
}

[numthreads(1, 1, 1)]
void F(F_inputs inputs) {
  F_inner(inputs.mat2x2);
}

