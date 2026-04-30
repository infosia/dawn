
RWByteAddressBuffer a : register(u0);
void main() {
  uint64_t v = uint64_t((0u).xx.x);
  uint64_t v_1 = 0uLL;
  a.InterlockedMin(0u, (v | (uint64_t((0u).xx.y) << 32u)), v_1);
}

