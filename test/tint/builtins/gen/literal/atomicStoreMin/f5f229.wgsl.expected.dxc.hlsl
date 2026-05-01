//
// fragment_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMin_f5f229() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = 0uLL;
  sb_rw.InterlockedMin(0u, ((v << 32u) | uint64_t((1u).xx.y)), v_1);
}

void fragment_main() {
  atomicStoreMin_f5f229();
}

//
// compute_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMin_f5f229() {
  uint64_t v = uint64_t((1u).xx.x);
  uint64_t v_1 = 0uLL;
  sb_rw.InterlockedMin(0u, ((v << 32u) | uint64_t((1u).xx.y)), v_1);
}

[numthreads(1, 1, 1)]
void compute_main() {
  atomicStoreMin_f5f229();
}

