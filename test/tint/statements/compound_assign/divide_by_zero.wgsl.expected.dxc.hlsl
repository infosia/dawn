
static int a = int(0);
static float b = 0.0f;
int tint_mod_i32(int lhs, int rhs) {
  return asint((asuint(lhs) - asuint(asint((asuint((lhs / select(((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1)))), int(1), rhs))) * asuint(select(((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1)))), int(1), rhs)))))));
}

int tint_div_i32(int lhs, int rhs) {
  return (lhs / select(((rhs == int(0)) | ((lhs == int(-2147483648)) & (rhs == int(-1)))), int(1), rhs));
}

void foo(int maybe_zero) {
  a = tint_div_i32(a, maybe_zero);
  a = tint_mod_i32(a, maybe_zero);
  b = (b / 0.0f);
  float v = b;
  float v_1 = (v / 0.0f);
  b = (v - (trunc(v_1) * 0.0f));
  float v_2 = b;
  b = (v_2 / float(maybe_zero));
  float v_3 = b;
  float v_4 = float(maybe_zero);
  float v_5 = (v_3 / v_4);
  b = (v_3 - (trunc(v_5) * v_4));
}

[numthreads(1, 1, 1)]
void main() {
  foo(int(0));
}

