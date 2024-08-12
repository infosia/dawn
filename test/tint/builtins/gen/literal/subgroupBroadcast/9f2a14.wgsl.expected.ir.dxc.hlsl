
RWByteAddressBuffer prevent_dce : register(u0);
float2 subgroupBroadcast_9f2a14() {
  float2 res = WaveReadLaneAt((1.0f).xx, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_9f2a14()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_9f2a14()));
}

