RWByteAddressBuffer prevent_dce : register(u0);

int2 subgroupBroadcast_a4c54f() {
  int2 arg_0 = (1).xx;
  int arg_1 = 1;
  int2 res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_a4c54f()));
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupBroadcast_a4c54f()));
  return;
}
