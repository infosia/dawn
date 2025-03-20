
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

[numthreads(1, 1, 1)]
void main() {
  main_1();
}

