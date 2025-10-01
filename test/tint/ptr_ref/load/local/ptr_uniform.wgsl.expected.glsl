#version 310 es

layout(binding = 0, std140)
uniform v_block_1_ubo {
  uvec4 inner[1];
} v_1;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  uint v_2 = uint(int(v_1.inner[0u].x));
  int u = int((v_2 + uint(1)));
}
