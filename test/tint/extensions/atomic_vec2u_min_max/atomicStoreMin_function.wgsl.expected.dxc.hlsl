
RWByteAddressBuffer a : register(u0);
void foo() {
  uint64_t v = uint64_t((0u).xx.x);
  uint64_t v_1 = 0uLL;
  a.InterlockedMin(0u, ((v << 32u) | uint64_t((0u).xx.y)), v_1);
}

void main() {
  foo();
}

