struct compute_main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


RWByteAddressBuffer prevent_dce : register(u0);
groupshared uint arg_0[1];
float workgroupUniformLoad_7a857c() {
  GroupMemoryBarrierWithGroupSync();
  float v = asfloat(arg_0[0u]);
  GroupMemoryBarrierWithGroupSync();
  float res = v;
  return res;
}

void compute_main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    arg_0[0u] = asuint(0.0f);
  }
  GroupMemoryBarrierWithGroupSync();
  prevent_dce.Store(0u, asuint(workgroupUniformLoad_7a857c()));
}

[numthreads(1, 1, 1)]
void compute_main(compute_main_inputs inputs) {
  compute_main_inner(inputs.tint_local_index);
}

