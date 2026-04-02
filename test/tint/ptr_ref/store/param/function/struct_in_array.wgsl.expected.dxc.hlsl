struct str {
  int i;
};


void func(inout str pointer) {
  pointer = (str)0;
}

[numthreads(1, 1, 1)]
void main() {
  str F[4] = (str[4])0;
  func(F[2u]);
}

