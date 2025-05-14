#version 310 es


struct A {
  uvec2 b;
  uint c;
};

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint q = 0u;
  uint w = A[2](A(uvec2(1u, 2u), 3u), A(uvec2(4u, 5u), 6u))[min(q, 1u)].b.x;
}
