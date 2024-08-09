
RWByteAddressBuffer prevent_dce : register(u0);
float4 subgroupBroadcast_a50527() {
  float4 arg_0 = (1.0f).xxxx;
  int arg_1 = 1;
  float4 res = WaveReadLaneAt(arg_0, arg_1);
  return res;
}

void fragment_main() {
  prevent_dce.Store4(0u, asuint(subgroupBroadcast_a50527()));
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store4(0u, asuint(subgroupBroadcast_a50527()));
}

