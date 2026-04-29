//
// fragment_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMin_f5f229() {
  uint2 arg_1 = (1u).xx;
  uint2 v = arg_1;
  uint64_t v_1 = uint64_t(v.x);
  uint64_t v_2 = 0uLL;
  sb_rw.InterlockedMin(0u, (v_1 | (uint64_t(v.y) << 32u)), v_2);
}

void fragment_main() {
  atomicStoreMin_f5f229();
}

//
// compute_main
//

RWByteAddressBuffer sb_rw : register(u0);
void atomicStoreMin_f5f229() {
  uint2 arg_1 = (1u).xx;
  uint2 v = arg_1;
  uint64_t v_1 = uint64_t(v.x);
  uint64_t v_2 = 0uLL;
  sb_rw.InterlockedMin(0u, (v_1 | (uint64_t(v.y) << 32u)), v_2);
}

[numthreads(1, 1, 1)]
void compute_main() {
  atomicStoreMin_f5f229();
}

