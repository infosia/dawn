RWByteAddressBuffer prevent_dce : register(u0);

float16_t subgroupBroadcast_7ae924() {
  float16_t res = WaveReadLaneAt(float16_t(1.0h), 1);
  return res;
}

void fragment_main() {
  prevent_dce.Store<float16_t>(0u, subgroupBroadcast_7ae924());
  return;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store<float16_t>(0u, subgroupBroadcast_7ae924());
  return;
}
