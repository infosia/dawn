
RWByteAddressBuffer prevent_dce : register(u0);
float16_t subgroupBroadcast_11675b() {
  float16_t res = WaveReadLaneAt(float16_t(1.0h), 1u);
  return res;
}

void fragment_main() {
  prevent_dce.Store<float16_t>(0u, subgroupBroadcast_11675b());
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store<float16_t>(0u, subgroupBroadcast_11675b());
}

