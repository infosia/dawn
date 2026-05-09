#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_a8e1a3() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0u).Store(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_a8e1a3();
}

