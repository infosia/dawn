
RWByteAddressBuffer prevent_dce : register(u0);
int subgroupBroadcast_26c534() {
  int arg_0 = 1;
  int arg_1 = 1;
  int res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_26c534()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store(0u, asuint(subgroupBroadcast_26c534()));
}

