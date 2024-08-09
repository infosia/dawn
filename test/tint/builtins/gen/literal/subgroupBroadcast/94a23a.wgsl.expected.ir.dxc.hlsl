
RWByteAddressBuffer prevent_dce : register(u0);
float subgroupBroadcast_94a23a() {
  float res = WaveReadLaneAt(1.0f, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_94a23a()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_94a23a()));
}

