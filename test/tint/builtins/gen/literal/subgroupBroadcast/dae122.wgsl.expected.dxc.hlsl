RWByteAddressBuffer prevent_dce : register(u0);

uint subgroupBroadcast_dae122() {
  uint res = WaveReadLaneAt(1u, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_dae122()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_dae122()));
  return;
}
