
RWByteAddressBuffer prevent_dce : register(u0);
float4 subgroupBroadcast_9a72b2() {
  float4 res = WaveReadLaneAt((1.0f).xxxx, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(subgroupBroadcast_9a72b2()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(subgroupBroadcast_9a72b2()));
}

