struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[8];
void v_1(uint start_byte_offset, float2x3 object) {
  v[(start_byte_offset / 4u)] = asuint(object[0u].x);
  v[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  v[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  v[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  v[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
}

float2x3 v_2(uint start_byte_offset) {
  float3 v_3 = asfloat(uint3(v[(start_byte_offset / 4u)], v[((start_byte_offset / 4u) + 1u)], v[(((start_byte_offset / 4u) + 1u) + 1u)]).xyz);
  return float2x3(v_3, asfloat(uint3(v[((16u + start_byte_offset) / 4u)], v[(((16u + start_byte_offset) / 4u) + 1u)], v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)]).xyz));
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v_1(0u, float2x3((0.0f).xxx, (0.0f).xxx));
  }
  GroupMemoryBarrierWithGroupSync();
  v_2(0u);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

