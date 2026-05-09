#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_d3e37d() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave> res = Matrix<ComponentType::U8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_ro, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_d3e37d().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

