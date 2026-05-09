#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_210e2c() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave> res = Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_rw, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_210e2c().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

