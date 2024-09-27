#version 310 es


struct S {
  int a;
  vec4 b;
  mat2 c;
};

layout(binding = 0, std430)
buffer tint_symbol_1_1_ssbo {
  S tint_symbol;
} v_1;
uint i = 0u;
int idx1() {
  i = (i + 1u);
  return 1;
}
int idx2() {
  i = (i + 2u);
  return 1;
}
int idx3() {
  i = (i + 3u);
  return 1;
}
void foo() {
  float a[4] = float[4](0.0f, 0.0f, 0.0f, 0.0f);
  {
    a[idx1()] = (a[idx1()] * 2.0f);
    while(true) {
      if ((a[idx2()] < 10.0f)) {
      } else {
        break;
      }
      {
        a[idx3()] = (a[idx3()] + 1.0f);
      }
      continue;
    }
  }
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
}
