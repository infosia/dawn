#version 310 es

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint q = 0u;
  uint w = uvec2[2](uvec2(1u, 2u), uvec2(3u, 4u))[min(q, 1u)].x;
}
