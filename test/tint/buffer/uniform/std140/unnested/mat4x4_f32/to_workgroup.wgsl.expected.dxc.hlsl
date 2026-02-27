struct f_inputs {
  uint tint_local_index : SV_GroupIndex;
};


cbuffer cbuffer_u : register(b0) {
  uint4 u[4];
};
groupshared uint w[16];
void v(uint start_byte_offset, float4x4 object) {
  w[(start_byte_offset / 4u)] = asuint(object[0u].x);
  w[((start_byte_offset / 4u) + 1u)] = asuint(object[0u].y);
  w[(((start_byte_offset / 4u) + 1u) + 1u)] = asuint(object[0u].z);
  w[((((start_byte_offset / 4u) + 1u) + 1u) + 1u)] = asuint(object[0u].w);
  w[((16u + start_byte_offset) / 4u)] = asuint(object[1u].x);
  w[(((16u + start_byte_offset) / 4u) + 1u)] = asuint(object[1u].y);
  w[((((16u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[1u].z);
  w[(((((16u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object[1u].w);
  w[((32u + start_byte_offset) / 4u)] = asuint(object[2u].x);
  w[(((32u + start_byte_offset) / 4u) + 1u)] = asuint(object[2u].y);
  w[((((32u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[2u].z);
  w[(((((32u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object[2u].w);
  w[((48u + start_byte_offset) / 4u)] = asuint(object[3u].x);
  w[(((48u + start_byte_offset) / 4u) + 1u)] = asuint(object[3u].y);
  w[((((48u + start_byte_offset) / 4u) + 1u) + 1u)] = asuint(object[3u].z);
  w[(((((48u + start_byte_offset) / 4u) + 1u) + 1u) + 1u)] = asuint(object[3u].w);
}

float4x4 v_1(uint start_byte_offset) {
  return float4x4(asfloat(u[(start_byte_offset / 16u)]), asfloat(u[((16u + start_byte_offset) / 16u)]), asfloat(u[((32u + start_byte_offset) / 16u)]), asfloat(u[((48u + start_byte_offset) / 16u)]));
}

void f_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    v(0u, float4x4((0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx));
  }
  GroupMemoryBarrierWithGroupSync();
  v(0u, v_1(0u));
  float4 v_2 = asfloat(u[0u]);
  w[4u] = asuint(v_2.x);
  w[5u] = asuint(v_2.y);
  w[6u] = asuint(v_2.z);
  w[7u] = asuint(v_2.w);
  float4 v_3 = asfloat(u[0u]).ywxz;
  w[4u] = asuint(v_3.x);
  w[5u] = asuint(v_3.y);
  w[6u] = asuint(v_3.z);
  w[7u] = asuint(v_3.w);
  w[1u] = asuint(asfloat(u[1u].x));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.tint_local_index);
}

