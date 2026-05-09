#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::I8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> subgroupMatrixLoad_8c9894() {
  Matrix<ComponentType::I8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> res = Matrix<ComponentType::I8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Load(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_8c9894().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

