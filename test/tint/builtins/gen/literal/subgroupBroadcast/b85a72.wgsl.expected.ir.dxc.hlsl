
RWByteAddressBuffer prevent_dce : register(u0);
float2 subgroupBroadcast_b85a72() {
  float2 res = WaveReadLaneAt((1.0f).xx, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_b85a72()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_b85a72()));
}

