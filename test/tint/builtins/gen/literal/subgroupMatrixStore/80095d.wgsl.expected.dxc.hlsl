#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_80095d() {
  Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Splat(0u).Store(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_80095d();
}

