
float foo() {
  int oob = int(99);
  uint v_1 = min(uint(oob), 3u);
  float b = (0.0f).xxxx[v_1];
  float4 v = (0.0f).xxxx;
  v[min(uint(oob), 3u)] = b;
  return b;
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

