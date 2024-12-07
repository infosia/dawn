#version 310 es


struct tint_struct {
  int tint_member;
};

layout(binding = 0, std430)
buffer s_block_1_ssbo {
  int inner;
} v;
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_struct c = tint_struct(0);
  int d = c.tint_member;
  v.inner = (c.tint_member + d);
}
