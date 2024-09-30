
RWByteAddressBuffer v : register(u0);
static uint i = 0u;
int idx1() {
  i = (i + 1u);
  return int(1);
}

int idx2() {
  i = (i + 2u);
  return int(1);
}

int idx3() {
  i = (i + 3u);
  return int(1);
}

void foo() {
  float a[4] = (float[4])0;
  {
    int v_1 = idx1();
    float v_2 = a[v_1];
    v_2 = (v_2 * 2.0f);
    while(true) {
      int v_3 = idx2();
      if ((a[v_3] < 10.0f)) {
      } else {
        break;
      }
      {
        int v_4 = idx3();
        float v_5 = a[v_4];
        v_5 = (v_5 + 1.0f);
      }
      continue;
    }
  }
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

