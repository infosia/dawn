struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


RWByteAddressBuffer result : register(u0);
groupshared uint wgvar[3];
void main_inner(uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    wgvar[0u] = uint((false).xxx.x);
    wgvar[1u] = uint((false).xxx.y);
    wgvar[2u] = uint((false).xxx.z);
  }
  GroupMemoryBarrierWithGroupSync();
  bool3 v = bool3(uint3(wgvar[0u], wgvar[1u], wgvar[2u]).xyz);
  wgvar[0u] = uint(v.x);
  wgvar[1u] = uint(v.y);
  wgvar[2u] = uint(v.z);
  bool e = bool(wgvar[0u]);
  wgvar[1u] = uint(e);
  GroupMemoryBarrierWithGroupSync();
  result.Store3(0u, uint3(bool3(uint3(wgvar[0u], wgvar[1u], wgvar[2u]).xyz)));
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

