
RWByteAddressBuffer sb_rw0 : register(u0);
RWByteAddressBuffer sb_rw1 : register(u1);
[numthreads(1, 1, 1)]
void compute_main() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = 0uLL;
  sb_rw0.InterlockedMax(0u, ((v << 32u) | uint64_t((1u).xx.y)), v_1);
  uint v_2 = 0u;
  sb_rw1.GetDimensions(v_2);
  uint v_3 = ((v_2 / 8u) - 1u);
  uint v_4 = (min(uint(int(0)), v_3) * 8u);
  uint64_t v_5 = uint64_t((1u).xx.x);
  uint64_t v_6 = 0uLL;
  sb_rw1.InterlockedMax((0u + v_4), ((v_5 << 32u) | uint64_t((1u).xx.y)), v_6);
}

