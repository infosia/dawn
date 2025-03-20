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
//
// gamma
//
void gamma_1() {
  return;
}

[numthreads(1, 1, 1)]
void gamma() {
  gamma_1();
  return;
}
//
// delta
//
void delta_1() {
  return;
}

[numthreads(1, 1, 1)]
void delta() {
  delta_1();
  return;
}
