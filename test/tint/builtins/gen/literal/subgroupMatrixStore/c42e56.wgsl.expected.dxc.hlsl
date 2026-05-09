#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_c42e56() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f).Store(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_c42e56();
}

