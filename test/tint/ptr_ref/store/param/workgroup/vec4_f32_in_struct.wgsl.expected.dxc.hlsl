struct str {
  float4 i;
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint4 S[1];
void func() {
  S[0u] = asuint((0.0f).xxxx);
}

void v(uint start_byte_offset, str object) {
  S[(start_byte_offset / 16u)] = asuint(object.i);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    str v_1 = (str)0;
    v(0u, v_1);
  }
  GroupMemoryBarrierWithGroupSync();
  func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

