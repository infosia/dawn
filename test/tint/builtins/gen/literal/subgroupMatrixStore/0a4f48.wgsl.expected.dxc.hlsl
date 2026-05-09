#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer sb_rw : register(u0);
void subgroupMatrixStore_0a4f48() {
  Matrix<ComponentType::I8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Splat(int(0)).Store(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixStore_0a4f48();
}

