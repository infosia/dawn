#version 310 es

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint q = 0u;
  uint w = uint[2][2][2](uint[2][2](uint[2](0u, 1u), uint[2](2u, 3u)), uint[2][2](uint[2](4u, 5u), uint[2](6u, 7u)))[min(q, 1u)][min(q, 1u)][min(q, 1u)];
}
