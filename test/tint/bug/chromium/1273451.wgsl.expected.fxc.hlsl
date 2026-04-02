struct B {
  int b;
};

struct A {
  int a;
};


B f(A a) {
  return (B)0;
}

[numthreads(1, 1, 1)]
void main() {
  A v = {int(1)};
  f(v);
}

