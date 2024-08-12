
RWByteAddressBuffer prevent_dce : register(u0);
int2 subgroupBroadcast_189e4c() {
  int2 arg_0 = (1).xx;
  uint arg_1 = 1u;
  int2 res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_189e4c()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_189e4c()));
}

