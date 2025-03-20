#version 310 es

void x_200() {
}
void x_300() {
}
void x_400() {
  x_200();
  x_300();
}
void x_500() {
  x_400();
  x_300();
}
void x_600() {
  x_300();
  x_500();
}
void main_1() {
  x_600();
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  main_1();
}
