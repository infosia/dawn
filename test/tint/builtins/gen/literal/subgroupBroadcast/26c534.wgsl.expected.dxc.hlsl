RWByteAddressBuffer prevent_dce : register(u0);

int subgroupBroadcast_26c534() {
  int res = WaveReadLaneAt(1, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_26c534()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_26c534()));
  return;
}
