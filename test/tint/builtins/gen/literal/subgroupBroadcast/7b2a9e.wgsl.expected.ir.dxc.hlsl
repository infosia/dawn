
RWByteAddressBuffer prevent_dce : register(u0);
uint4 subgroupBroadcast_7b2a9e() {
  uint4 res = WaveReadLaneAt((1u).xxxx, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, subgroupBroadcast_7b2a9e());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, subgroupBroadcast_7b2a9e());
}

