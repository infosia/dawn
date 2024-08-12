
RWByteAddressBuffer prevent_dce : register(u0);
uint3 subgroupBroadcast_a0a167() {
  uint3 res = WaveReadLaneAt((1u).xxx, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store3(0u, subgroupBroadcast_a0a167());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store3(0u, subgroupBroadcast_a0a167());
}

