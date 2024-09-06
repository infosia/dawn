
float4 v() {
  return (0.0f).xxxx;
}

void f() {
  float4 a = (1.0f).xxxx;
  float4 b = float4((a).xxxx);
  float4 c = float4((v()).xxxx);
  float4 d = float4(((a * 2.0f)).xxxx);
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

