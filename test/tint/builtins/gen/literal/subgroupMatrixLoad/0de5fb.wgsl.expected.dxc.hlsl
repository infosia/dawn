#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::I32, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_0de5fb() {
  Matrix<ComponentType::I32, 8, 8, MatrixUse::A, MatrixScope::Wave> res = Matrix<ComponentType::I32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_0de5fb().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

