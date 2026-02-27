struct foo_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared uint a[1];
groupshared uint4 b[1];
groupshared uint2 c[2];
void v(uint start_byte_offset, float2x2 object) {
  c[(start_byte_offset / 8u)] = asuint(object[0u]);
  c[((8u + start_byte_offset) / 8u)] = asuint(object[1u]);
}

float2x2 v_1(uint start_byte_offset) {
  return float2x2(asfloat(c[(start_byte_offset / 8u)]), asfloat(c[((8u + start_byte_offset) / 8u)]));
}

int tint_div_i32(int lhs, int rhs) {
  return (lhs / ((((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1))))) ? (int(1)) : (rhs)));
}

void foo_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    a[0u] = asuint(int(0));
    b[0u] = asuint((0.0f).xxxx);
    v(0u, float2x2((0.0f).xx, (0.0f).xx));
  }
  GroupMemoryBarrierWithGroupSync();
  a[0u] = asuint(tint_div_i32(asint(a[0u]), int(2)));
  b[0u] = asuint(mul(float4x4((0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx, (0.0f).xxxx), asfloat(b[0u])));
  v(0u, (v_1(0u) * 2.0f));
}

[numthreads(1, 1, 1)]
void foo(foo_inputs inputs) {
  foo_inner(inputs.tint_local_index);
}

