#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::U32, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_e49506() {
  Matrix<ComponentType::U32, 8, 8, MatrixUse::B, MatrixScope::Wave> res = Matrix<ComponentType::U32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_ro, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_e49506().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

