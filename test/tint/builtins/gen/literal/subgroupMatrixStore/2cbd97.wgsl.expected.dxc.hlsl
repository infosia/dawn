#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_2cbd97() {
  Matrix<ComponentType::I32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(int(0)).Store(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_2cbd97();
}

