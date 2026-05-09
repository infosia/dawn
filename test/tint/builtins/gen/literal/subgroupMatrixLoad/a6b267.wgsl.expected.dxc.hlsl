#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_a6b267() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave> res = Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_a6b267().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

