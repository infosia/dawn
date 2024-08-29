#version 310 es

struct A {
  float a[1000000];
};

int v[1000000];
A b;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
}
