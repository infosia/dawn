//
// alpha
//

void alpha_1() {
}

[numthreads(1, 1, 1)]
void alpha() {
  alpha_1();
}

//
// beta
//

void beta_1() {
}

[numthreads(1, 1, 1)]
void beta() {
  beta_1();
}

