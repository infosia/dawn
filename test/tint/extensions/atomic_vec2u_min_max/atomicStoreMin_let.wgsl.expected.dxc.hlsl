
RWByteAddressBuffer a : register(u0);
void main() {
  uint64_t v = uint64_t((0u).xx.y);
  uint64_t v_1 = uint64_t((0u).xx.x);
  a.InterlockedMin(0u, ((v_1 << uint64_t(32u)) | v));
}

