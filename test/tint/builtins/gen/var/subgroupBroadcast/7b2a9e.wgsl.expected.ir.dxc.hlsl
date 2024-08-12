
RWByteAddressBuffer prevent_dce : register(u0);
uint4 subgroupBroadcast_7b2a9e() {
  uint4 arg_0 = (1u).xxxx;
  int arg_1 = 1;
  uint4 res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, subgroupBroadcast_7b2a9e());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, subgroupBroadcast_7b2a9e());
}

