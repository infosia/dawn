
int tint_mod_i32(int lhs, int rhs) {
  return asint((asuint(lhs) - asuint(asint((asuint((lhs / select(((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1)))), int(1), rhs))) * asuint(select(((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1)))), int(1), rhs)))))));
}

[numthreads(1, 1, 1)]
void f() {
  int a = int(1);
  int b = int(2);
  int r = tint_mod_i32(a, b);
}

