//
// fragment_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMax_707bde() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = uint64_t((1u).xx.y);
  uint64_t v_2 = 0uLL;
  sb_rw.InterlockedMax(0u, ((v_1 << uint64_t(32u)) | v), v_2);
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
  uint64_t v_1 = uint64_t((1u).xx.y);
  uint64_t v_2 = 0uLL;
  sb_rw.InterlockedMax(0u, ((v_1 << uint64_t(32u)) | v), v_2);
}

[numthreads(1, 1, 1)]
void compute_main() {
  atomicStoreMax_707bde();
}

