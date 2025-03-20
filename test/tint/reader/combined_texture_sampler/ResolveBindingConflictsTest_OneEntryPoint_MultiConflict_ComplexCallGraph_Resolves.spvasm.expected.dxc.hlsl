void x_200() {
  return;
}

void x_300() {
  return;
}

void x_400() {
  x_200();
  x_300();
  return;
}

void x_500() {
  x_400();
  x_300();
  return;
}

void x_600() {
  x_300();
  x_500();
  return;
}

void main_1() {
  x_600();
  return;
}

[numthreads(1, 1, 1)]
void main() {
  main_1();
  return;
}
