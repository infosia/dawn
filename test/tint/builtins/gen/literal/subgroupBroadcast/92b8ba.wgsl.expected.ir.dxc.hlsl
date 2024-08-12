
RWByteAddressBuffer prevent_dce : register(u0);
uint subgroupBroadcast_92b8ba() {
  uint res = WaveReadLaneAt(1u, 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, subgroupBroadcast_92b8ba());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, subgroupBroadcast_92b8ba());
}

