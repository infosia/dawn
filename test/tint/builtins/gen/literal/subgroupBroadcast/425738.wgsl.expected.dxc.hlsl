RWByteAddressBuffer prevent_dce : register(u0);

uint3 subgroupBroadcast_425738() {
  uint3 res = WaveReadLaneAt((1u).xxx, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store3(0u, asuint(subgroupBroadcast_425738()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store3(0u, asuint(subgroupBroadcast_425738()));
  return;
}
