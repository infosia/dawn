
RWByteAddressBuffer prevent_dce : register(u0);
int2 subgroupBroadcast_189e4c() {
  int2 res = WaveReadLaneAt((1).xx, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_189e4c()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_189e4c()));
}

