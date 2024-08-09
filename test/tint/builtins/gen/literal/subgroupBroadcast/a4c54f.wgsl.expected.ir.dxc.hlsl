
RWByteAddressBuffer prevent_dce : register(u0);
int2 subgroupBroadcast_a4c54f() {
  int2 res = WaveReadLaneAt((1).xx, 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_a4c54f()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_a4c54f()));
}

