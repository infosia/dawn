
RWByteAddressBuffer sb_rw0 : register(u0);
RWByteAddressBuffer sb_rw1 : register(u1);
[numthreads(1, 1, 1)]
void compute_main() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = uint64_t((1u).xx.y);
  uint64_t v_2 = 0uLL;
  sb_rw0.InterlockedMax(0u, ((v_1 << uint64_t(32u)) | v), v_2);
  uint v_3 = 0u;
  sb_rw1.GetDimensions(v_3);
  uint v_4 = ((v_3 / 8u) - 1u);
  uint v_5 = (min(uint(int(0)), v_4) * 8u);
  uint64_t v_6 = uint64_t((1u).xx.x);
  uint64_t v_7 = uint64_t((1u).xx.y);
  uint64_t v_8 = 0uLL;
  sb_rw1.InterlockedMax((0u + v_5), ((v_7 << uint64_t(32u)) | v_6), v_8);
}

