
void f() {
  int i = int(1);
  int v = i;
  float2 a = float4x2((0.0f).xx, (0.0f).xx, float2(4.0f, 0.0f), (0.0f).xx)[v];
  int v_1 = i;
  int b = int2(int(0), int(1))[v_1];
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

