struct str {
  int i;
};


static str P[4] = (str[4])0;
void func(inout str pointer) {
  pointer = (str)0;
}

[numthreads(1, 1, 1)]
void main() {
  func(P[2u]);
}

