//
// fragment_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMax_707bde() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = 0uLL;
  sb_rw.InterlockedMax(0u, (v | (uint64_t((1u).xx.y) << 32u)), v_1);
}

void fragment_main() {
  atomicStoreMax_707bde();
}

//
// compute_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMax_707bde() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = 0uLL;
  sb_rw.InterlockedMax(0u, (v | (uint64_t((1u).xx.y) << 32u)), v_1);
}

[numthreads(1, 1, 1)]
void compute_main() {
  atomicStoreMax_707bde();
}

