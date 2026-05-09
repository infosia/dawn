#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::U8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> subgroupMatrixLoad_bbc2f9() {
  Matrix<ComponentType::U8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> res = Matrix<ComponentType::U8, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Load(sb_ro, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_bbc2f9().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

