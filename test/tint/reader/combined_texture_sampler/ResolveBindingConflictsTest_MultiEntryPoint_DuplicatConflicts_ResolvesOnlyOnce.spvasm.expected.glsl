//
// alpha
//
#version 310 es

void alpha_1() {
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  alpha_1();
}
//
// beta
//
#version 310 es

void beta_1() {
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  beta_1();
}
