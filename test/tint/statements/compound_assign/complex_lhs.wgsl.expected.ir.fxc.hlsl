struct S {
  int4 a[4];
};


static int counter = int(0);
int foo() {
  counter = (counter + int(1));
  return counter;
}

int bar() {
  counter = (counter + int(2));
  return counter;
}

void main() {
  S x = (S)0;
  S p = x;
  int v = foo();
  int4 v_1 = p.a[v];
  int v_2 = bar();
  v_1[v_2] = (v_1[v_2] + int(5));
}

[numthreads(1, 1, 1)]
void unused_entry_point() {
}

