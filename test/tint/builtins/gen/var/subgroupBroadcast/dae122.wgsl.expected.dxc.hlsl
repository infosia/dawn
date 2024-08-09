RWByteAddressBuffer prevent_dce : register(u0);

uint subgroupBroadcast_dae122() {
  uint arg_0 = 1u;
  int arg_1 = 1;
  uint res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_dae122()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_dae122()));
  return;
}
