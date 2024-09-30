
void f() {
  int i = int(0);
  int j = int(0);
  float2x2 m = float2x2(float2(1.0f, 2.0f), float2(3.0f, 4.0f));
  int v = i;
  int v_1 = j;
  float f_1 = m[v][v_1];
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

