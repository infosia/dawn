struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint v[12];
float3x3 v_1(uint start_byte_offset) {
  float3 v_2 = asfloat(uint3(v[(start_byte_offset / 4u)], v[((start_byte_offset / 4u) + 1u)], v[(((start_byte_offset / 4u) + 1u) + 1u)]).xyz);
  float3 v_3 = asfloat(uint3(v[((16u + start_byte_offset) / 4u)], v[(((16u + start_byte_offset) / 4u) + 1u)], v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)]).xyz);
  return float3x3(v_2, v_3, asfloat(uint3(v[((32u + start_byte_offset) / 4u)], v[(((32u + start_byte_offset) / 4u) + 1u)], v[((((32u + start_byte_offset) / 4u) + 1u) + 1u)]).xyz));
}

float3x3 foo() {
  GroupMemoryBarrierWithGroupSync();
  float3x3 v_4 = v_1(0u);
  GroupMemoryBarrierWithGroupSync();
  return v_4;
}

void v_5(uint start_byte_offset, float3x3 object) {
  v[(start_byte_offset / 4u)] = asuint(object[0u].x);
  v[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  v[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  v[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  v[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  v[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
  v[((32u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  v[(((32u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  v[((((32u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[2u].z);
}

void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v_5(0u, float3x3((0.0f).xxx, (0.0f).xxx, (0.0f).xxx));
  }
  GroupMemoryBarrierWithGroupSync();
  foo();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

