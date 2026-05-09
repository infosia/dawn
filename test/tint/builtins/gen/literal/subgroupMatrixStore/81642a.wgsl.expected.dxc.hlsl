#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_81642a() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0u).Store(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_81642a();
}

