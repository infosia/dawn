
RWByteAddressBuffer prevent_dce : register(u0);
uint subgroupBroadcast_92b8ba() {
  uint arg_0 = 1u;
  uint arg_1 = 1u;
  uint res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, subgroupBroadcast_92b8ba());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, subgroupBroadcast_92b8ba());
}

