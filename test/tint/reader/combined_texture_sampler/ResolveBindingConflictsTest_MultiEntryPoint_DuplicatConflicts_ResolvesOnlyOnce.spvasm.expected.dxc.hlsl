//
// alpha
//
void alpha_1() {
  return;
}

[numthreads(1, 1, 1)]
void alpha() {
  alpha_1();
  return;
}
//
// beta
//
void beta_1() {
  return;
}

[numthreads(1, 1, 1)]
void beta() {
  beta_1();
  return;
}
